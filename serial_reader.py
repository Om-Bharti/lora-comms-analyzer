import serial
import json
import sqlite3
import os
import time
from datetime import datetime

db_path = os.path.expanduser('~/lora_project/data/lora.db')
SERIAL_PORT = '/dev/ttyUSB0'
BAUD_RATE = 9600

def get_db():
    conn = sqlite3.connect(db_path)
    return conn

def insert_packet(data):
    conn = get_db()
    cursor = conn.cursor()
    cursor.execute('''
        INSERT INTO packets 
        (node_id, seq_num, sf, bw, cr, rssi, snr, crc_ok)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    ''', (
        data.get('node', 0),
        data.get('seq', 0),
        data.get('sf', 0),
        data.get('bw', 0),
        data.get('cr', 0),
        data.get('rssi', 0.0),
        data.get('snr', 0.0),
        1 if data.get('crc_ok', False) else 0
    ))
    conn.commit()
    conn.close()

def main():
    print(f"Opening serial port {SERIAL_PORT} at {BAUD_RATE} baud...")
    
    while True:
        try:
            ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
            print("Serial port opened. Waiting for packets...")
            
            while True:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                
                if not line:
                    continue
                
                print(f"[{datetime.now().strftime('%H:%M:%S')}] Raw: {line}")
                
                try:
                    data = json.loads(line)
                    insert_packet(data)
                    print(f"  Saved — RSSI:{data.get('rssi')} SNR:{data.get('snr')} SF:{data.get('sf')} CRC:{'OK' if data.get('crc_ok') else 'FAIL'}")
                except json.JSONDecodeError:
                    print(f"  Skipped (not JSON)")
                    
        except serial.SerialException as e:
            print(f"Serial error: {e}")
            print("Retrying in 3 seconds...")
            time.sleep(3)
        except KeyboardInterrupt:
            print("\nStopped by user")
            break

if __name__ == '__main__':
    main()
