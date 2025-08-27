import serial
import os
from datetime import datetime

SERIAL_PORT = '/dev/cu.usbmodem2101'  # Update your port
BAUD_RATE = 115200
SAVE_FOLDER = os.path.dirname(os.path.abspath(__file__))

ser = serial.Serial(SERIAL_PORT, BAUD_RATE)
print(f"Listening on {SERIAL_PORT} at {BAUD_RATE} baud...")

csv_count = 0

while True:
    line = ser.readline().decode('utf-8', errors='ignore').strip()

    # Detect table header
    if line.startswith("Idx,Raw,Ref,Corrected,EchoIdx,Thickness_mm"):
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = os.path.join(SAVE_FOLDER, f"scan_{timestamp}_{csv_count}.csv")
        csv_count += 1
        print(f"Saving CSV to {filename}")

        with open(filename, 'w') as f:
            f.write(line + "\n")  # write header

            # Read MAX_SAMPLES lines
            for _ in range(50):
                data_line = ser.readline().decode('utf-8', errors='ignore').strip()
                f.write(data_line + "\n")

        # Skip any separator line
        while True:
            sep_line = ser.readline().decode('utf-8', errors='ignore').strip()
            if sep_line.startswith("-----"):
                break

        print("CSV saved successfully.\n")

