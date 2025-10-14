#!/usr/bin/env python3
"""
⚡ Upload Rápido - Carga ambos ESP32 automáticamente
No hace preguntas, solo detecta y carga todo
"""

import serial.tools.list_ports
import subprocess
import sys
import os

# Colores
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

def detect_esp32_devices():
    """Detecta automáticamente los ESP32 conectados"""
    print(f"{Colors.YELLOW}🔍 Detectando dispositivos ESP32...{Colors.END}")
    
    devices = {"ESP32-C3": None, "ESP32-WROOM": None}
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        port_name = port.device
        port_desc = port.description.lower()
        
        if 'jtag' in port_desc or 'debug unit' in port_desc or 'usbmodem' in port_name:
            devices["ESP32-C3"] = port_name
            print(f"{Colors.CYAN}   ✅ ESP32-C3: {port_name}{Colors.END}")
        elif 'usb serial' in port_desc or 'usbserial' in port_name:
            devices["ESP32-WROOM"] = port_name
            print(f"{Colors.GREEN}   ✅ ESP32-WROOM: {port_name}{Colors.END}")
    
    return devices

def upload_to_device(project_path, device_port, device_name):
    """Sube código a un dispositivo específico"""
    print(f"\n{Colors.YELLOW}🚀 Subiendo {device_name}...{Colors.END}")
    
    try:
        original_dir = os.getcwd()
        os.chdir(project_path)
        
        cmd = [
            "/Users/jmoisio/.platformio/penv/bin/platformio",
            "run", "--target", "upload",
            "--upload-port", device_port
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=120)
        os.chdir(original_dir)
        
        if result.returncode == 0:
            print(f"{Colors.GREEN}   ✅ {device_name} - OK!{Colors.END}")
            return True
        else:
            print(f"{Colors.RED}   ❌ {device_name} - Error{Colors.END}")
            return False
            
    except Exception as e:
        print(f"{Colors.RED}   ❌ {device_name} - Error: {e}{Colors.END}")
        os.chdir(original_dir)
        return False

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 50)
    print("    ⚡ UPLOAD RÁPIDO ESP32")
    print("=" * 50)
    print(f"{Colors.END}")
    
    # Detectar dispositivos
    devices = detect_esp32_devices()
    
    if not devices["ESP32-C3"] or not devices["ESP32-WROOM"]:
        print(f"{Colors.RED}❌ Falta algún ESP32. Conecta ambos via USB{Colors.END}")
        return False
    
    # Rutas de proyectos
    projects = {
        "ESP32-C3": "/Users/jmoisio/Documents/PlatformIO/Projects/BorkerMQTT",
        "ESP32-WROOM": "/Users/jmoisio/Documents/PlatformIO/Projects/HuellaDactilar"
    }
    
    print(f"\n{Colors.MAGENTA}🔄 Cargando ambos dispositivos...{Colors.END}")
    
    success_count = 0
    
    # Cargar ESP32-C3 (Broker)
    if upload_to_device(projects["ESP32-C3"], devices["ESP32-C3"], "ESP32-C3 (BROKER)"):
        success_count += 1
    
    # Cargar ESP32-WROOM (Client)  
    if upload_to_device(projects["ESP32-WROOM"], devices["ESP32-WROOM"], "ESP32-WROOM (CLIENT)"):
        success_count += 1
    
    # Resumen
    print(f"\n{Colors.BOLD}📊 RESUMEN:{Colors.END}")
    if success_count == 2:
        print(f"{Colors.GREEN}✅ Ambos ESP32 cargados exitosamente{Colors.END}")
        print(f"{Colors.CYAN}🎉 Sistema listo para usar!{Colors.END}")
        return True
    else:
        print(f"{Colors.RED}❌ Solo {success_count}/2 dispositivos cargados{Colors.END}")
        return False

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Cancelado{Colors.END}")
        sys.exit(1)