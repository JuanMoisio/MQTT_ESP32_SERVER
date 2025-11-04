#!/usr/bin/env python3
"""
Direct command test - Send command directly to ESP32-C3 server
"""
import serial
import time
import sys

def test_server_command():
    try:
        # Connect directly to ESP32-C3 server
        ser = serial.Serial('/dev/cu.usbmodem11201', 115200, timeout=1)
        
        print("🔗 Conectado al ESP32-C3 SERVER")
        print("📤 Enviando comando: server:scan_fingerprint")
        
        # Send the command
        ser.write(b'server:scan_fingerprint\n')
        ser.flush()
        
        print("⏳ Esperando respuesta (10 segundos)...")
        
        # Read response for 10 seconds
        start_time = time.time()
        while time.time() - start_time < 10:
            if ser.in_waiting > 0:
                response = ser.readline().decode('utf-8', errors='ignore').strip()
                if response:
                    print(f"📨 SERVER: {response}")
            time.sleep(0.1)
        
        ser.close()
        print("✅ Test completado")
        
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    test_server_command()
