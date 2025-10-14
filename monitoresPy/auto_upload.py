#!/usr/bin/env python3
"""
🚀 Auto-Upload ESP32
Detecta automáticamente los ESP32 y sube el código correcto a cada uno
"""

import serial.tools.list_ports
import subprocess
import sys
import os
from datetime import datetime

# Colores para output
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
        port_hwid = port.hwid.lower() if port.hwid else ""
        
        print(f"{Colors.BLUE}   📱 {port_name} - {port.description}{Colors.END}")
        
        # ESP32-C3 generalmente aparece como USB JTAG/serial debug unit o usbmodem
        if 'jtag' in port_desc or 'debug unit' in port_desc:
            devices["ESP32-C3"] = port_name
            print(f"{Colors.CYAN}   ✅ ESP32-C3 detectado: {port_name}{Colors.END}")
        elif 'usbmodem' in port_name and not devices["ESP32-C3"]:
            devices["ESP32-C3"] = port_name
            print(f"{Colors.CYAN}   ✅ ESP32-C3 detectado por puerto: {port_name}{Colors.END}")
        
        # ESP32-WROOM generalmente aparece como USB Serial o usbserial
        elif 'usb serial' in port_desc or 'usbserial' in port_name:
            devices["ESP32-WROOM"] = port_name
            print(f"{Colors.GREEN}   ✅ ESP32-WROOM detectado: {port_name}{Colors.END}")
    
    return devices

def upload_to_device(project_path, device_port, device_name):
    """Sube código a un dispositivo específico"""
    print(f"\n{Colors.YELLOW}🚀 Subiendo código a {device_name}...{Colors.END}")
    print(f"{Colors.BLUE}   📁 Proyecto: {project_path}{Colors.END}")
    print(f"{Colors.BLUE}   📱 Puerto: {device_port}{Colors.END}")
    
    try:
        # Cambiar al directorio del proyecto
        original_dir = os.getcwd()
        os.chdir(project_path)
        
        # Comando platformio con puerto específico
        cmd = [
            "/Users/jmoisio/.platformio/penv/bin/platformio",
            "run",
            "--target", "upload",
            "--upload-port", device_port
        ]
        
        print(f"{Colors.CYAN}   ⚡ Ejecutando: {' '.join(cmd)}{Colors.END}")
        
        # Ejecutar comando
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=120  # 2 minutos timeout
        )
        
        # Restaurar directorio original
        os.chdir(original_dir)
        
        if result.returncode == 0:
            print(f"{Colors.GREEN}   ✅ {device_name} - Upload exitoso!{Colors.END}")
            return True
        else:
            print(f"{Colors.RED}   ❌ {device_name} - Error en upload:{Colors.END}")
            print(f"{Colors.RED}      {result.stderr}{Colors.END}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"{Colors.RED}   ❌ {device_name} - Timeout (>2min){Colors.END}")
        os.chdir(original_dir)
        return False
    except Exception as e:
        print(f"{Colors.RED}   ❌ {device_name} - Error: {e}{Colors.END}")
        os.chdir(original_dir)
        return False

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 60)
    print("    🚀 ESP32 AUTO-UPLOAD SYSTEM")
    print("=" * 60)
    print(f"{Colors.END}")
    
    # Detectar dispositivos
    devices = detect_esp32_devices()
    
    if not devices["ESP32-C3"] and not devices["ESP32-WROOM"]:
        print(f"{Colors.RED}❌ No se detectaron dispositivos ESP32{Colors.END}")
        return False
    
    print(f"\n{Colors.GREEN}📋 Dispositivos detectados:{Colors.END}")
    if devices["ESP32-C3"]:
        print(f"{Colors.CYAN}   🔷 ESP32-C3 (BROKER): {devices['ESP32-C3']}{Colors.END}")
    if devices["ESP32-WROOM"]:
        print(f"{Colors.GREEN}   🔶 ESP32-WROOM (CLIENT): {devices['ESP32-WROOM']}{Colors.END}")
    
    # Rutas de los proyectos
    esp32c3_project = "/Users/jmoisio/Documents/PlatformIO/Projects/BorkerMQTT"
    esp32_wroom_project = "/Users/jmoisio/Documents/PlatformIO/Projects/HuellaDactilar"
    
    # Verificar si los proyectos existen
    if not os.path.exists(esp32c3_project):
        print(f"{Colors.RED}❌ Proyecto ESP32-C3 no encontrado: {esp32c3_project}{Colors.END}")
        return False
        
    if not os.path.exists(esp32_wroom_project):
        print(f"{Colors.RED}❌ Proyecto ESP32-WROOM no encontrado: {esp32_wroom_project}{Colors.END}")
        return False
    
    print(f"\n{Colors.YELLOW}🎯 Selecciona qué subir:{Colors.END}")
    print(f"{Colors.CYAN}  1. Solo ESP32-C3 (Broker MQTT){Colors.END}")
    print(f"{Colors.GREEN}  2. Solo ESP32-WROOM (Cliente Fingerprint){Colors.END}")
    print(f"{Colors.MAGENTA}  3. Ambos dispositivos{Colors.END}")
    print(f"{Colors.BLUE}  4. Cancelar{Colors.END}")
    
    choice = input(f"\n{Colors.YELLOW}Elige opción (1-4): {Colors.END}")
    
    success_count = 0
    
    if choice == "1" and devices["ESP32-C3"]:
        if upload_to_device(esp32c3_project, devices["ESP32-C3"], "ESP32-C3 (BROKER)"):
            success_count += 1
            
    elif choice == "2" and devices["ESP32-WROOM"]:
        if upload_to_device(esp32_wroom_project, devices["ESP32-WROOM"], "ESP32-WROOM (CLIENT)"):
            success_count += 1
            
    elif choice == "3":
        print(f"\n{Colors.MAGENTA}🔄 Subiendo a ambos dispositivos...{Colors.END}")
        
        if devices["ESP32-C3"]:
            if upload_to_device(esp32c3_project, devices["ESP32-C3"], "ESP32-C3 (BROKER)"):
                success_count += 1
        else:
            print(f"{Colors.YELLOW}⚠️ ESP32-C3 no detectado, saltando...{Colors.END}")
        
        if devices["ESP32-WROOM"]:
            if upload_to_device(esp32_wroom_project, devices["ESP32-WROOM"], "ESP32-WROOM (CLIENT)"):
                success_count += 1
        else:
            print(f"{Colors.YELLOW}⚠️ ESP32-WROOM no detectado, saltando...{Colors.END}")
            
    elif choice == "4":
        print(f"{Colors.BLUE}🚪 Operación cancelada{Colors.END}")
        return True
    else:
        print(f"{Colors.RED}❌ Opción inválida o dispositivo no detectado{Colors.END}")
        return False
    
    # Resumen final
    print(f"\n{Colors.BOLD}📊 RESUMEN:{Colors.END}")
    if success_count > 0:
        print(f"{Colors.GREEN}✅ Uploads exitosos: {success_count}{Colors.END}")
        print(f"{Colors.CYAN}🎉 ¡Listo para usar!{Colors.END}")
    else:
        print(f"{Colors.RED}❌ No se completaron uploads exitosos{Colors.END}")
    
    return success_count > 0

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Operación interrumpida{Colors.END}")
        sys.exit(1)
    except Exception as e:
        print(f"{Colors.RED}❌ Error inesperado: {e}{Colors.END}")
        sys.exit(1)