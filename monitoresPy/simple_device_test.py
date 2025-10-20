#!/usr/bin/env python3
"""
Script simple para probar conectividad con dispositivos ESP32
"""

import serial
import serial.tools.list_ports
import time
import sys

def test_port(port, timeout=5):
    """Prueba un puerto con timeout limitado"""
    print(f"🔍 Probando {port}...")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"  ✅ Puerto abierto exitosamente")
        
        # Intentar leer datos por un tiempo limitado
        start_time = time.time()
        lines_received = 0
        
        while time.time() - start_time < timeout and lines_received < 5:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        print(f"  📤 {line}")
                        lines_received += 1
                except Exception as e:
                    print(f"  ⚠️ Error leyendo línea: {e}")
                    
            time.sleep(0.1)
        
        ser.close()
        print(f"  📊 Total líneas recibidas: {lines_received}")
        return True
        
    except Exception as e:
        print(f"  ❌ Error: {e}")
        return False

def main():
    print("🔍 Detector de Dispositivos ESP32 Simple")
    print("=" * 50)
    
    # Buscar puertos ESP32
    ports = serial.tools.list_ports.comports()
    esp32_ports = []
    
    for port in ports:
        port_name = port.device
        port_desc = port.description.lower()
        
        print(f"📱 Encontrado: {port_name} - {port.description}")
        
        if any(keyword in port_desc for keyword in ['usb serial', 'jtag', 'silicon labs']) or \
           'usb' in port_name.lower():
            esp32_ports.append(port_name)
    
    if not esp32_ports:
        print("❌ No se encontraron puertos ESP32")
        return
    
    print(f"\n🎯 Puertos ESP32 detectados: {len(esp32_ports)}")
    
    for port in esp32_ports:
        print(f"\n" + "="*30)
        success = test_port(port, timeout=3)
        if not success:
            print(f"  ⚠️ {port} no responde")
    
    print(f"\n✅ Prueba completada")

if __name__ == "__main__":
    main()