import serial
import os
import time
from datetime import datetime

# ----------------- CONFIG -----------------
SERIAL_PORT = '/dev/cu.usbmodem2101'       # Replace with your port (e.g., '/dev/ttyACM0' on Linux)
BAUD_RATE = 115200

# Folder to save CSV files (same as script)
SAVE_FOLDER = os.path.dirname(os.path.abspath(__file__))

# ----------------- SERIAL CONNECTION -----------------
ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud...")

# ----------------- MAIN LOOP -----------------
while True:
    line = ser.readline().decode('utf-8', errors='ignore').strip()
    
    # Detect start of CSV block
    if line.startswith("=== Sleep Detected"):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = os.path.join(SAVE_FOLDER, f"scan_{timestamp}.csv")
        print(f"Detected sleep switch — saving CSV to {filename}")
        
        with open(filename, 'w', newline='') as f:
            # Read CSV header
            header = ser.readline().decode('utf-8', errors='ignore').strip()
            f.write(header + "\n")
            
            # Read CSV lines until end marker
            while True:
                data_line = ser.readline().decode('utf-8', errors='ignore').strip()
                if data_line.startswith("=== End CSV"):
                    print("CSV saved successfully.\n")
                    break
                f.write(data_line + "\n")

