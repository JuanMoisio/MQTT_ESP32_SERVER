#!/usr/bin/env python3
"""
Simple ESP32 Dual Monitor
Monitorea ESP32-C3 (Broker) y ESP32-WROOM (Cliente) en terminales separadas
"""

import subprocess
import sys
import time

def main():
    print("🚀 ESP32 Dual Monitor Simple")
    print("=" * 50)
    
    # Puertos conocidos
    esp32c3_port = "/dev/cu.usbmodem31201"  # ESP32-C3 Broker
    esp32_wroom_port = "/dev/cu.usbserial-3110"  # ESP32 WROOM Cliente
    
    print(f"📡 ESP32-C3 Broker: {esp32c3_port}")
    print(f"🔍 ESP32-WROOM Cliente: {esp32_wroom_port}")
    print("=" * 50)
    
    try:
        # Abrir terminal para ESP32-C3 (Broker)
        print("Abriendo terminal para ESP32-C3 (Broker)...")
        broker_cmd = [
            "osascript", "-e", 
            f'tell application "Terminal" to do script "screen {esp32c3_port} 115200"'
        ]
        subprocess.run(broker_cmd)
        
        time.sleep(2)
        
        # Abrir terminal para ESP32-WROOM (Cliente)
        print("Abriendo terminal para ESP32-WROOM (Cliente)...")
        client_cmd = [
            "osascript", "-e",
            f'tell application "Terminal" to do script "screen {esp32_wroom_port} 115200"'
        ]
        subprocess.run(client_cmd)
        
        print("✅ Ambas terminales abiertas!")
        print("\n📋 INSTRUCCIONES:")
        print("- Terminal 1: ESP32-C3 Broker (monitor del servidor)")
        print("- Terminal 2: ESP32-WROOM Cliente (monitor del dispositivo fingerprint)")
        print("- Para salir de screen: Ctrl+A, luego K, luego Y")
        print("- Para conectar el cliente, selecciona una red WiFi en la Terminal 2")
        
    except Exception as e:
        print(f"❌ Error: {e}")
        print("\n🔧 Alternativa manual:")
        print(f"Terminal 1: screen {esp32c3_port} 115200")
        print(f"Terminal 2: screen {esp32_wroom_port} 115200")

if __name__ == "__main__":
    main()