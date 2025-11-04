#!/usr/bin/env python3
"""
🧪 Test de Auto-detección de ESP32
Script para probar la detección automática sin ejecutar el monitor completo
"""

import serial.tools.list_ports

def test_detection():
    print("🔍 Detectando puertos serie...")
    
    ports = serial.tools.list_ports.comports()
    
    print(f"\n📋 Puertos disponibles ({len(ports)}):")
    for i, port in enumerate(ports, 1):
        print(f"  {i}. {port.device}")
        print(f"     Descripción: {port.description}")
        print(f"     Hardware ID: {port.hwid}")
        print()
    
    devices = {"ESP32-C3": None, "ESP32-WROOM": None}
    
    print("🎯 Aplicando lógica de detección...")
    
    for port in ports:
        port_name = port.device
        port_desc = port.description.lower()
        
        print(f"   Analizando: {port_name}")
        print(f"   Descripción: {port_desc}")
        
        if 'usbmodem' in port_name:
            devices["ESP32-C3"] = port_name
            print(f"   ✅ Asignado como ESP32-C3")
        elif 'usbserial' in port_name:
            devices["ESP32-WROOM"] = port_name
            print(f"   ✅ Asignado como ESP32-WROOM")
        else:
            print(f"   ⏭️ No coincide con patrones ESP32")
        print()
    
    print("📊 Resultado final:")
    print(f"   ESP32-C3 (SERVER): {devices['ESP32-C3'] or 'NO DETECTADO'}")
    print(f"   ESP32-WROOM (CLIENT): {devices['ESP32-WROOM'] or 'NO DETECTADO'}")

if __name__ == "__main__":
    test_detection()
