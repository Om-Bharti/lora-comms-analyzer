import sqlite3
import os

db_path = os.path.expanduser('~/lora_project/data/lora.db')

conn = sqlite3.connect(db_path)
cursor = conn.cursor()

cursor.execute('''
    CREATE TABLE IF NOT EXISTS packets (
        id        INTEGER PRIMARY KEY AUTOINCREMENT,
        timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
        node_id   INTEGER,
        seq_num   INTEGER,
        sf        INTEGER,
        bw        INTEGER,
        cr        INTEGER,
        rssi      REAL,
        snr       REAL,
        crc_ok    INTEGER,
        distance  REAL DEFAULT 0
    )
''')

conn.commit()
conn.close()
print("Database created successfully")
