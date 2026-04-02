# 📡 lora-comms-analyzer

A real-time **LoRa Digital Communications Analyzer** built with ESP8266 NodeMCU, Ra-02 LoRa modules, and Raspberry Pi 3B. Demonstrates core digital communications theory — RSSI, SNR, BER, link budget, spreading factor sweep, and Friis path loss — through a live web dashboard.

![Dashboard](https://img.shields.io/badge/Dashboard-Live-3fb950?style=flat-square)
![Platform](https://img.shields.io/badge/Platform-ESP8266%20%7C%20RPi%203B-58a6ff?style=flat-square)
![Frequency](https://img.shields.io/badge/LoRa-433%20MHz-d29922?style=flat-square)
![License](https://img.shields.io/badge/License-MIT-bc8cff?style=flat-square)

---

## 📸 Dashboard Preview

> Live charts showing RSSI/SNR over time, BER vs Spreading Factor, noise floor estimate, link budget calculator, and packet inspector.

---

## 🎯 What This Project Demonstrates

| Concept | Implementation |
|---|---|
| **RSSI** | Measured per packet, plotted live vs time |
| **SNR** | Per-SF average, distribution histogram |
| **BER** | CRC16 + sequence number tracking across SF7–SF12 |
| **Spreading Factor** | Automated sweep SF7→SF12, effect on SNR/BER measured |
| **Link Budget** | Friis path loss equation, predicted vs measured RSSI |
| **Noise Floor** | Estimated as RSSI − SNR averaged across all packets |
| **Time on Air** | SF7 (72ms) vs SF12 (2.5s) tradeoff demonstrated |
| **Forward Error Correction** | Coding Rate 4/5 on all transmissions |
| **Packet Framing** | Custom 16-byte frame with preamble, seq num, CRC16, ACK |

---

## 🧰 Hardware Required

| Component | Quantity | Purpose |
|---|---|---|
| ESP8266 NodeMCU (CP2102) | 2 | Transmitter + Receiver nodes |
| LoRa Ra-02 (SX1278, 433 MHz) | 2 | RF radio modules |
| Raspberry Pi 3B | 1 | Backend server + dashboard |
| MicroSD card (16GB+) | 1 | Raspberry Pi OS |
| Breadboard | 2 | Wiring |
| Jumper wires (M-F) | 20 | Connections |
| USB cable (micro-USB) | 2 | Power + serial data |
| Power bank (5V 2A) | 1 | Portable power for range test |

### Wiring — ESP8266 to Ra-02 (both nodes identical)

| Ra-02 Pin | NodeMCU Pin |
|---|---|
| 3.3V | 3V3 ⚠️ never 5V |
| GND | GND |
| MOSI | D7 |
| MISO | D6 |
| SCK | D5 |
| NSS | D8 |
| RST | D0 |
| DIO0 | D2 |

> ⚠️ **Ra-02 is 3.3V only.** Connecting to 5V will permanently damage the module.

---

## 🗂️ Project Structure

```
lora-comms-analyzer/
│
├── node1_transmitter/
│   └── node1_transmitter.ino    # ESP8266 transmitter firmware
│
├── node2_receiver/
│   └── node2_receiver.ino       # ESP8266 receiver firmware
│
├── raspberry_pi/
│   ├── app.py                   # Flask REST API
│   ├── serial_reader.py         # USB serial ingest → SQLite
│   ├── init_db.py               # Database setup script
│   └── templates/
│       └── index.html           # Live web dashboard
│
└── README.md
```

---

## ⚙️ System Architecture

```
ESP8266 Node 1                      ESP8266 Node 2         Raspberry Pi 3B
(Transmitter)                       (Receiver)             (Server)

Button pressed
    ↓
Sync packet @ SF7 ──── RF 433MHz ──→ Detect sync
    ↓                                Switch SF
Burst @ target SF ──── RF 433MHz ──→ Receive packets
SF7→SF8→SF9→SF10→SF11→SF12          CRC16 check
                                     Measure RSSI, SNR
                                     Count BER
                                     Build JSON
                                         ↓
                                     USB Serial (9600)
                                         ↓
                                                     serial_reader.py
                                                     → SQLite DB
                                                     → Flask API
                                                     → Browser dashboard
                                                        192.168.x.x:5000
```

---

## 🚀 Setup Guide

### Step 1 — Flash Raspberry Pi OS

1. Download [Raspberry Pi Imager](https://www.raspberrypi.com/software/)
2. Flash **Raspberry Pi OS Lite (64-bit)** to microSD
3. In imager settings: enable SSH, set Wi-Fi credentials, set hostname `lorapi`
4. Insert SD card, power on Pi, wait 90 seconds
5. SSH in: `ssh pi@lorapi.local`

### Step 2 — Pi Software Setup

```bash
sudo apt update && sudo apt upgrade -y
sudo apt install python3-pip sqlite3 -y
pip3 install pyserial flask
sudo usermod -a -G dialout pi
sudo reboot
```

### Step 3 — Create Project

```bash
mkdir ~/lora_project
mkdir ~/lora_project/templates
mkdir ~/lora_project/data
```

Copy `app.py`, `serial_reader.py`, `init_db.py` into `~/lora_project/`
Copy `index.html` into `~/lora_project/templates/`

```bash
python3 ~/lora_project/init_db.py
```

Expected: `Database created successfully`

### Step 4 — Arduino IDE Setup

1. Install [Arduino IDE 2.x](https://www.arduino.cc/en/software)
2. Add ESP8266 board URL in Preferences:
   ```
   http://arduino.esp8266.com/stable/package_esp8266com_index.json
   ```
3. Install **ESP8266 by ESP8266 Community** via Boards Manager
4. Install libraries via Library Manager:
   - `LoRa` by Sandeep Mistry
   - `ArduinoJson` by Benoit Blanchon (v6.x)
5. Board: `NodeMCU 1.0 (ESP-12E Module)`
6. Upload Speed: `9600`

### Step 5 — Upload Firmware

1. Upload `node1_transmitter.ino` to NodeMCU #1
2. Upload `node2_receiver.ino` to NodeMCU #2
3. Plug NodeMCU #2 into Pi via USB

### Step 6 — Run

On Raspberry Pi, open two terminals:

**Terminal 1:**
```bash
python3 ~/lora_project/serial_reader.py
```

**Terminal 2:**
```bash
python3 ~/lora_project/app.py
```

Open browser → `http://lorapi.local:5000`

Press button on Node 1 — full SF7→SF12 sweep runs automatically.

---

## 📊 How the Experiment Works

Node 1 automatically cycles through all six spreading factors on a single button press:

```
Button press → SF7 burst (20 packets) → SF8 burst → SF9 burst
            → SF10 burst → SF11 burst → SF12 burst
```

Each burst is preceded by a **sync packet at SF7** so Node 2 knows which SF to switch to. The receiver measures RSSI and SNR for each packet, validates CRC16, tracks sequence numbers for dropped packet detection, and streams everything as JSON to the Pi over USB serial.

### Packet Frame Format

```
Byte 0      : Node ID (0x01)
Bytes 1–2   : Sequence number (uint16)
Byte 3      : Spreading Factor (7–12)
Byte 4      : Bandwidth (125 kHz)
Byte 5      : Coding Rate (5 = 4/5)
Bytes 6–13  : Test payload (0xDEADBEEF CAFEBABE)
Bytes 14–15 : CRC16 checksum
```

---

## 📈 Dashboard Features

| Panel | What it shows |
|---|---|
| **Stats strip** | Total packets, errors, BER, avg RSSI, avg SNR, noise floor, best SF, link margin |
| **RSSI over time** | Live line chart of RSSI + SNR per packet |
| **SNR per SF** | Bar chart — average SNR for each spreading factor |
| **BER vs SF** | Bar chart — bit error rate per spreading factor |
| **Packets per SF** | Count of packets received at each SF |
| **Noise floor** | Estimated as RSSI − SNR, with visual bar meter |
| **SNR distribution** | Histogram of all SNR values received |
| **Predictions** | Max range SF7/SF12, recommended SF, BER at 500m — calculated from your measured data |
| **Link budget** | Friis path loss calculator — enter distance, get predicted RSSI vs sensitivity |
| **Packet inspector** | Last 30 packets decoded field by field with CRC status |

---

## 🔬 Key Results

Running the SF sweep with nodes ~5m apart indoors:

| SF | Avg RSSI | Avg SNR | BER | Time on Air |
|---|---|---|---|---|
| SF7 | ~−48 dBm | ~9.5 dB | 0.0000 | 72 ms |
| SF8 | ~−49 dBm | ~11 dB | 0.0000 | 128 ms |
| SF9 | ~−49 dBm | ~11 dB | 0.0000 | 246 ms |
| SF10 | ~−50 dBm | ~11 dB | 0.0000 | 493 ms |
| SF11 | ~−50 dBm | ~12 dB | 0.0000 | 986 ms |
| SF12 | ~−49 dBm | ~12 dB | 0.0000 | 2.5 s |

> At close range, all SFs achieve near-zero BER. The meaningful comparison is **Time on Air vs range capability** — SF12 is 35× slower than SF7 but can decode signals 10+ dB weaker, translating to significantly greater range.

---

## 📡 Digital Communications Theory

### Friis Path Loss Equation
```
FSPL (dB) = 20·log₁₀(d) + 20·log₁₀(f) + 20·log₁₀(4π/c)

Where:
  d = distance (m)
  f = frequency (433 × 10⁶ Hz)
  c = speed of light (3 × 10⁸ m/s)
```

### Link Budget
```
Link Margin = Tx Power − Path Loss − Rx Sensitivity
           = 17 dBm − FSPL − (−137 dBm)
```

### Noise Floor Estimate
```
Noise Floor = RSSI − SNR   (per packet, then averaged)
```

### LoRa SNR Sensitivity Thresholds (SX1278)
| SF | Min SNR |
|---|---|
| SF7 | −7.5 dB |
| SF8 | −10.0 dB |
| SF9 | −12.5 dB |
| SF10 | −15.0 dB |
| SF11 | −17.5 dB |
| SF12 | −20.0 dB |

---

## 🗺️ Outdoor Range Test

1. Configure Pi as Wi-Fi hotspot (optional) or keep on router
2. Power Node 1 from a power bank
3. Keep Node 2 + Pi at fixed location
4. Walk away in 20m steps up to 300–500m
5. Press button at each stop — 20-packet burst
6. Watch RSSI drop live on browser dashboard
7. Compare measured path loss curve to Friis model

---

## 🛠️ Dependencies

### Raspberry Pi
- Python 3.x
- Flask
- PySerial
- SQLite3 (built-in)

### Arduino
- ESP8266 Arduino Core
- LoRa by Sandeep Mistry
- ArduinoJson v6 by Benoit Blanchon

---

## 📝 License

MIT License — free to use, modify, and distribute.

---

## 🙏 Acknowledgements

- [LoRa library](https://github.com/sandeepmistry/arduino-LoRa) by Sandeep Mistry
- [ArduinoJson](https://arduinojson.org/) by Benoit Blanchon
- [Chart.js](https://www.chartjs.org/) for dashboard visualizations
- Semtech SX1278 datasheet for SNR sensitivity thresholds

---

*Built as a mini project demonstrating digital communications concepts — RSSI, SNR, BER, link budget, and LoRa physical layer parameters.*
