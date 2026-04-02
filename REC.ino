#include <SPI.h>
#include <LoRa.h>
#include <ArduinoJson.h>

#define NSS       D8
#define RST       D0
#define DIO0      D2
#define LED_GREEN D4
#define LED_RED   D3

int totalPackets = 0;
int errorPackets = 0;
int expectedSeq  = 0;
int currentSF    = 7;
unsigned long lastActivity = 0;

// Timeout — 30s to handle slow SF12 bursts safely
#define TIMEOUT_MS 30000

uint16_t crc16(uint8_t *data, size_t length) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < length; i++) {
    crc ^= (uint16_t)data[i] << 8;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x8000) crc = (crc << 1) ^ 0x1021;
      else crc <<= 1;
    }
  }
  return crc;
}

void switchSF(int sf) {
  currentSF = sf;
  LoRa.idle();
  LoRa.setSpreadingFactor(sf);
  LoRa.receive();
  // CRITICAL — reset timeout the moment we switch
  lastActivity = millis();
  Serial.print("Switched to SF");
  Serial.println(sf);
}

void handleSyncPacket() {
  uint8_t sync[4];
  for (int i = 0; i < 4; i++) sync[i] = LoRa.read();

  if (sync[0] == 0x53 && sync[1] == 0x4E) {
    int nextSF = sync[2];
    Serial.print("Sync received — next SF: ");
    Serial.println(nextSF);
    // Reset timeout immediately on sync received
    lastActivity = millis();
    delay(100);
    switchSF(nextSF);
  }
}

void handleDataPacket() {
  uint8_t packet[16];
  for (int i = 0; i < 16; i++) packet[i] = LoRa.read();

  int nodeId = packet[0];
  int seqNum = packet[1] | (packet[2] << 8);
  int sf     = packet[3];
  int bw     = packet[4];
  int cr     = packet[5];

  uint16_t rxCRC   = packet[14] | (packet[15] << 8);
  uint16_t calcCRC = crc16(packet, 14);
  bool crcOk = (rxCRC == calcCRC);

  uint8_t expected[8] = {0xDE, 0xAD, 0xBE, 0xEF,
                         0xCA, 0xFE, 0xBA, 0xBE};
  bool payloadOk = true;
  for (int i = 0; i < 8; i++) {
    if (packet[6 + i] != expected[i]) {
      payloadOk = false;
      break;
    }
  }

  // Missed packet detection
  if (expectedSeq > 0 && seqNum > expectedSeq + 1) {
    int missed = seqNum - expectedSeq - 1;
    errorPackets += missed;
    totalPackets += missed;
  }
  expectedSeq = seqNum;
  totalPackets++;
  if (!crcOk || !payloadOk) errorPackets++;

  float rssi = LoRa.packetRssi();
  float snr  = LoRa.packetSnr();
  float ber  = totalPackets > 0
               ? (float)errorPackets / totalPackets : 0.0;

  // Reset timeout on every received packet
  lastActivity = millis();

  // LED feedback
  if (crcOk && payloadOk) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
  } else {
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, HIGH);
  }
  delay(80);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);

  // JSON to Pi
  StaticJsonDocument<256> doc;
  doc["node"]   = nodeId;
  doc["seq"]    = seqNum;
  doc["sf"]     = sf;
  doc["bw"]     = bw;
  doc["cr"]     = cr;
  doc["rssi"]   = rssi;
  doc["snr"]    = snr;
  doc["crc_ok"] = crcOk && payloadOk;
  doc["ber"]    = ber;
  doc["total"]  = totalPackets;
  doc["errors"] = errorPackets;

  serializeJson(doc, Serial);
  Serial.println();
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED,   OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED,   LOW);

  LoRa.setPins(NSS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("FAIL — LoRa not found");
    while (1);
  }

  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSpreadingFactor(7);
  LoRa.receive();

  lastActivity = millis();
  Serial.println("Node 2 ready — listening SF7");
}

void loop() {
  int packetSize = LoRa.parsePacket();

  if (packetSize == 4) {
    handleSyncPacket();
  } else if (packetSize == 16) {
    handleDataPacket();
  } else if (packetSize > 0) {
    // Unknown — drain buffer
    while (LoRa.available()) LoRa.read();
  }

  // Timeout — return to SF7 only if inactive for 30s
  if (currentSF != 7 &&
      (millis() - lastActivity > TIMEOUT_MS)) {
    Serial.println("Timeout 30s — returning to SF7");
    switchSF(7);
  }
}