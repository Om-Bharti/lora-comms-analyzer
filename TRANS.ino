#include <SPI.h>
#include <LoRa.h>

#define NSS    D8
#define RST    D0
#define DIO0   D2
#define BUTTON D3

int spreadingFactors[]  = {7, 8, 9, 10, 11, 12};
int sequenceNumber = 0;
int packetsPerBurst = 20;

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

void sendSyncPacket(int nextSF) {
  LoRa.setSpreadingFactor(7);
  delay(300);

  uint8_t sync[4];
  sync[0] = 0x53;
  sync[1] = 0x4E;
  sync[2] = nextSF;
  sync[3] = packetsPerBurst;

  // Send sync THREE times so receiver cannot miss it
  for (int i = 0; i < 3; i++) {
    LoRa.beginPacket();
    LoRa.write(sync, 4);
    LoRa.endPacket();
    delay(200);
  }

  Serial.print(">>> Sync sent 3x for SF");
  Serial.println(nextSF);

  // SF-specific wait — tuned for each SF
  int syncDelays[] = {800, 1000, 1800, 2000, 2500, 3000};
  delay(syncDelays[nextSF - 7]);
}
void sendBurst(int sf) {
  LoRa.setSpreadingFactor(sf);
  delay(300);

  Serial.println("─────────────────────────────");
  Serial.print("Burst start — SF");
  Serial.println(sf);

  // Time on air per SF (ms) — must be longer than actual ToA
  int waitTimes[] = {600, 800, 1100, 1500, 2000, 2800};
  int wait = waitTimes[sf - 7];

  for (int i = 0; i < packetsPerBurst; i++) {
    sequenceNumber++;

    uint8_t payload[8] = {0xDE, 0xAD, 0xBE, 0xEF,
                          0xCA, 0xFE, 0xBA, 0xBE};
    uint8_t packet[16];
    packet[0] = 0x01;
    packet[1] = sequenceNumber & 0xFF;
    packet[2] = (sequenceNumber >> 8) & 0xFF;
    packet[3] = sf;
    packet[4] = 125;
    packet[5] = 5;
    memcpy(&packet[6], payload, 8);

    uint16_t crc = crc16(packet, 14);
    packet[14] = crc & 0xFF;
    packet[15] = (crc >> 8) & 0xFF;

    LoRa.beginPacket();
    LoRa.write(packet, 16);
    LoRa.endPacket();

    Serial.print("  #");
    Serial.print(sequenceNumber);
    Serial.print(" SF");
    Serial.println(sf);

    delay(wait);
  }

  Serial.print("Burst done — SF");
  Serial.println(sf);
}

void runFullSweep() {
  Serial.println("=== FULL SWEEP START ===");
  Serial.println("SF7 → SF8 → SF9 → SF10 → SF11 → SF12");

  for (int i = 0; i < 6; i++) {
    int sf = spreadingFactors[i];
    sendSyncPacket(sf);
    sendBurst(sf);
    // Small gap between bursts — Node 2 goes back to SF7
    // and waits for next sync
    delay(1000);
  }

  Serial.println("=== FULL SWEEP COMPLETE ===");
  Serial.println("Press button to run again");
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  pinMode(BUTTON, INPUT_PULLUP);

  LoRa.setPins(NSS, RST, DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("FAIL — LoRa not found");
    while (1);
  }

  LoRa.setTxPower(17);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSpreadingFactor(7);

  Serial.println("Node 1 ready");
  Serial.println("Press button — runs full sweep SF7 to SF12 automatically");
}

void loop() {
  if (digitalRead(BUTTON) == LOW) {
    delay(50);
    if (digitalRead(BUTTON) == LOW) {
      runFullSweep();
      while (digitalRead(BUTTON) == LOW);
    }
  }
}