#!/usr/bin/env python3
"""
Simple ESP32 Dual Monitor
Monitorea ESP32-C3 (Broker) y ESP32-WROOM (Cliente) en terminales separadas
Reintenta abrir el puerto si no está disponible.
"""

import subprocess
import time

def open_terminal(port, label):
    while True:
        try:
            print(f"Intentando abrir terminal para {label} en {port}...")
            cmd = [
                "osascript", "-e",
                f'tell application "Terminal" to do script "screen {port} 115200"'
            ]
            result = subprocess.run(cmd)
            if result.returncode == 0:
                print(f"✅ Terminal abierta para {label}!")
                break
            else:
                print(f"❌ Error abriendo {label}. Reintentando en 5 segundos...")
        except Exception as e:
            print(f"❌ Error: {e}. Reintentando en 5 segundos...")
        time.sleep(5)

def main():
    print("🚀 ESP32 Dual Monitor Simple (con reintentos)")
    print("=" * 50)
    esp32c3_port = "/dev/cu.usbmodem31201"  # ESP32-C3 Broker
    esp32_wroom_port = "/dev/cu.usbserial-3110"  # ESP32 WROOM Cliente
    print(f"📡 ESP32-C3 Broker: {esp32c3_port}")
    print(f"🔍 ESP32-WROOM Cliente: {esp32_wroom_port}")
    print("=" * 50)
    open_terminal(esp32c3_port, "ESP32-C3 Broker")
    open_terminal(esp32_wroom_port, "ESP32-WROOM Cliente")
    print("✅ Ambas terminales abiertas!")
    print("\n📋 INSTRUCCIONES:")
    print("- Terminal 1: ESP32-C3 Broker (monitor del servidor)")
    print("- Terminal 2: ESP32-WROOM Cliente (monitor del dispositivo fingerprint)")
    print("- Para salir de screen: Ctrl+A, luego K, luego Y")
    print("- Para conectar el cliente, selecciona una red WiFi en la Terminal 2")

if __name__ == "__main__":
    main()