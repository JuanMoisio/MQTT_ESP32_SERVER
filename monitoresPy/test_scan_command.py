#!/usr/bin/env python3
"""
Test script to send scan_fingerprint command directly to ESP32-C3 server
"""

import serial
import time
import sys

def test_scan_command():
    print("🔍 Testing server:scan_fingerprint command...")
    
    try:
        # Connect to ESP32-C3 server
        server_port = "/dev/cu.usbmodem11201"
        print(f"📡 Connecting to ESP32-C3 server at {server_port}")
        
        ser = serial.Serial(server_port, 115200, timeout=2)
        time.sleep(1)  # Wait for connection to stabilize
        
        print("✅ Connected to ESP32-C3 server")
        print("📤 Sending 'scan_fingerprint' command...")
        
        # Send the scan_fingerprint command
        command = "scan_fingerprint\n"
        ser.write(command.encode())
        
        print("📨 Command sent! Waiting for response...")
        
        # Wait for response
        start_time = time.time()
        while time.time() - start_time < 10:  # Wait up to 10 seconds
            if ser.in_waiting > 0:
                response = ser.readline().decode('utf-8', errors='ignore').strip()
                if response:
                    print(f"📥 Server Response: {response}")
                    
                    # Check if we get any indication the command was processed
                    if "scan" in response.lower() or "command" in response.lower():
                        print("✅ Server appears to have processed the scan command!")
                        break
            time.sleep(0.1)
        
        ser.close()
        print("🔌 Connection closed")
        
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    test_scan_command()