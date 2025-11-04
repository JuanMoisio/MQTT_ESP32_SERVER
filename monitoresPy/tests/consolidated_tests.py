import serial
import time

# Configuration
SERIAL_PORT = '/dev/ttyUSB0'  # Adjust as needed
BAUD_RATE = 57600

def connect_sensor():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print("Sensor connected.")
        return ser
    except Exception as e:
        print(f"Connection failed: {e}")
        return None

def diagnostic_test(ser):
    print("Running diagnostic test...")
    # Send diagnostic command (example: check sensor status)
    ser.write(b'\xEF\x01\xFF\xFF\xFF\xFF\x01\x00\x03\x0A\x00\x0E')  # Placeholder command
    response = ser.read(12)
    if response:
        print(f"Diagnostic response: {response.hex()}")
    else:
        print("No response from sensor.")

def scan_test(ser):
    print("Running scan test...")
    # Send scan command (example: capture fingerprint)
    ser.write(b'\xEF\x01\xFF\xFF\xFF\xFF\x01\x00\x03\x01\x00\x05')  # Placeholder command
    response = ser.read(12)
    if response:
        print(f"Scan response: {response.hex()}")
    else:
        print("No response from sensor.")

def finger_commands_test(ser):
    print("Running finger commands test...")
    # Example: Enroll finger
    ser.write(b'\xEF\x01\xFF\xFF\xFF\xFF\x01\x00\x04\x02\x01\x00\x08')  # Placeholder command
    response = ser.read(12)
    if response:
        print(f"Enroll response: {response.hex()}")
    else:
        print("No response from sensor.")
    # Add more commands as needed (e.g., match, delete)

def main():
    ser = connect_sensor()
    if not ser:
        return
    try:
        diagnostic_test(ser)
        time.sleep(1)
        scan_test(ser)
        time.sleep(1)
        finger_commands_test(ser)
    finally:
        ser.close()
        print("Tests completed.")

if __name__ == "__main__":
    main()
