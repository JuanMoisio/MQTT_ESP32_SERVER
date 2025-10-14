#!/usr/bin/env python3
"""
Simple script to send scan_fingerprint command directly to ESP32-C3
"""

import serial
import time

def send_scan_command():
    print("🔍 Testing server:scan_fingerprint command...")
    
    try:
        # Connect to ESP32-C3 server
        server_port = "/dev/cu.usbmodem11201"
        print(f"📡 Connecting to ESP32-C3 server at {server_port}")
        
        ser = serial.Serial(server_port, 115200, timeout=1)
        time.sleep(0.5)  # Brief pause for connection
        
        print("✅ Connected to ESP32-C3")
        print("📤 Sending 'server:scan_fingerprint' command...")
        
        # Send the command in the exact format the server expects
        command = "server:scan_fingerprint\n"
        ser.write(command.encode())
        ser.flush()  # Ensure command is sent immediately
        
        print("📨 Command sent!")
        print("✅ Check the monitor terminal for debug output and client response!")
        print("   Expected to see:")
        print("   - DEBUG: Comando server recibido: scan_fingerprint")
        print("   - DEBUG: Buscando módulo fingerprint_scanner...")
        print("   - DEBUG: Módulo encontrado: fingerprint_4b224f7c630")
        print("   - DEBUG: Enviando comando scan_fingerprint...")
        print("   - Initiando escaneo de huella...")
        print("   - Client should start fingerprint scanning")
        
        # Brief pause to allow processing
        time.sleep(1)
        
        ser.close()
        print("🔌 Connection closed")
        
    except Exception as e:
        print(f"❌ Error sending command: {e}")

if __name__ == "__main__":
    send_scan_command()