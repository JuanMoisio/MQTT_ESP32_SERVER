#!/usr/bin/env python3
"""
🔐 ESP32 Fingerprint System - Monitor Principal
Sistema de monitoreo para ESP32-C3 (Broker MQTT) + ESP32-WROOM (Cliente Fingerprint)
Autor: Sistema automatizado
Fecha: $(date +%Y-%m-%d)
"""

import subprocess
import sys
import os
import glob
import time
from pathlib import Path

# Colores para terminal
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    WHITE = '\033[97m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

def print_banner():
    print(f"{Colors.BOLD}{Colors.CYAN}")
    print("=" * 70)
    print("    🔐 ESP32 FINGERPRINT SYSTEM MONITOR")
    print("    ESP32-C3 (Broker MQTT) + ESP32-WROOM (Cliente)")
    print("=" * 70)
    print(f"{Colors.END}")

def detect_ports():
    """Detecta automáticamente los puertos de los ESP32"""
    print(f"{Colors.YELLOW}🔍 Detectando puertos ESP32...{Colors.END}")
    
    # Buscar puertos USB
    ports = glob.glob('/dev/cu.*')
    
    esp32c3_port = None
    esp32_wroom_port = None
    
    for port in ports:
        if 'usbmodem' in port:
            esp32c3_port = port  # ESP32-C3 SuperMini (USB CDC)
        elif 'usbserial' in port:
            esp32_wroom_port = port  # ESP32 WROOM (USB Serial)
    
    if esp32c3_port and esp32_wroom_port:
        print(f"{Colors.GREEN}✅ ESP32-C3 (Broker): {esp32c3_port}{Colors.END}")
        print(f"{Colors.GREEN}✅ ESP32-WROOM (Cliente): {esp32_wroom_port}{Colors.END}")
        return esp32c3_port, esp32_wroom_port
    else:
        print(f"{Colors.RED}❌ No se encontraron ambos dispositivos{Colors.END}")
        print(f"{Colors.YELLOW}💡 Conecta ambos ESP32 via USB{Colors.END}")
        return None, None

def show_menu():
    """Muestra el menú principal"""
    print(f"\n{Colors.BOLD}{Colors.BLUE}📋 OPCIONES DISPONIBLES:{Colors.END}")
    print(f"{Colors.CYAN}1.{Colors.END} 🚀 Setup Completo (Subir código + Abrir monitores)")
    print(f"{Colors.CYAN}2.{Colors.END} 📺 Solo Monitores Serie")
    print(f"{Colors.CYAN}3.{Colors.END} ⬆️  Solo Subir Código ESP32-C3 (Broker)")
    print(f"{Colors.CYAN}4.{Colors.END} ⬆️  Solo Subir Código ESP32-WROOM (Cliente)")
    print(f"{Colors.CYAN}5.{Colors.END} 🔧 Monitor Avanzado (Dual Python)")
    print(f"{Colors.CYAN}6.{Colors.END} ❌ Salir")
    print()

def setup_complete():
    """Setup completo automatizado"""
    script_path = Path(__file__).parent / "setup_complete.py"
    subprocess.run(["python3", str(script_path)])

def open_simple_monitors():
    """Abre monitores serie simples"""
    script_path = Path(__file__).parent / "simple_monitor.py"
    subprocess.run(["python3", str(script_path)])

def open_advanced_monitor():
    """Abre monitor avanzado Python"""
    script_path = Path(__file__).parent / "dual_monitor.py"
    subprocess.run(["python3", str(script_path)])

def upload_broker():
    """Sube solo el código del broker"""
    print(f"{Colors.YELLOW}📤 Subiendo código ESP32-C3 (Broker)...{Colors.END}")
    
    broker_path = "/Users/jmoisio/Documents/PlatformIO/Projects/BorkerMQTT"
    pio_path = "/Users/jmoisio/.platformio/penv/bin/platformio"
    
    try:
        os.chdir(broker_path)
        result = subprocess.run([pio_path, "run", "--target", "upload", "-e", "esp32c3-supermini"], 
                               check=True)
        print(f"{Colors.GREEN}✅ ESP32-C3 programado exitosamente{Colors.END}")
    except subprocess.CalledProcessError:
        print(f"{Colors.RED}❌ Error programando ESP32-C3{Colors.END}")

def upload_client():
    """Sube solo el código del cliente"""
    print(f"{Colors.YELLOW}📤 Subiendo código ESP32-WROOM (Cliente)...{Colors.END}")
    
    client_path = "/Users/jmoisio/Documents/PlatformIO/Projects/HuellaDactilar"
    pio_path = "/Users/jmoisio/.platformio/penv/bin/platformio"
    
    try:
        os.chdir(client_path)
        result = subprocess.run([pio_path, "run", "--target", "upload"], check=True)
        print(f"{Colors.GREEN}✅ ESP32-WROOM programado exitosamente{Colors.END}")
    except subprocess.CalledProcessError:
        print(f"{Colors.RED}❌ Error programando ESP32-WROOM{Colors.END}")

def main():
    print_banner()
    
    # Detectar puertos
    esp32c3_port, esp32_wroom_port = detect_ports()
    
    if not (esp32c3_port and esp32_wroom_port):
        print(f"\n{Colors.RED}🚫 Sistema no disponible - Verifica conexiones{Colors.END}")
        return 1
    
    while True:
        show_menu()
        
        try:
            choice = input(f"{Colors.BOLD}Selecciona una opción (1-6): {Colors.END}").strip()
            
            if choice == '1':
                print(f"{Colors.YELLOW}🚀 Iniciando setup completo...{Colors.END}")
                setup_complete()
                print(f"{Colors.GREEN}✅ Setup completado. ¿Deseas hacer algo más?{Colors.END}")
            elif choice == '2':
                print(f"{Colors.YELLOW}📺 Abriendo monitores serie...{Colors.END}")
                open_simple_monitors()
            elif choice == '3':
                upload_broker()
                print(f"{Colors.GREEN}✅ Proceso completado.{Colors.END}")
            elif choice == '4':
                upload_client()
                print(f"{Colors.GREEN}✅ Proceso completado.{Colors.END}")
            elif choice == '5':
                print(f"{Colors.YELLOW}🔧 Iniciando monitor avanzado...{Colors.END}")
                open_advanced_monitor()
            elif choice == '6':
                print(f"{Colors.GREEN}👋 ¡Hasta luego!{Colors.END}")
                break
            else:
                print(f"{Colors.RED}❌ Opción inválida. Selecciona 1-6{Colors.END}")
                
            # Pausa antes de mostrar el menú nuevamente
            if choice != '6':
                input(f"\n{Colors.CYAN}Presiona Enter para volver al menú...{Colors.END}")
                print("\n" + "="*50)
                
        except KeyboardInterrupt:
            print(f"\n{Colors.YELLOW}👋 Saliendo...{Colors.END}")
            break
        except Exception as e:
            print(f"{Colors.RED}❌ Error: {e}{Colors.END}")
    
    return 0

if __name__ == "__main__":
    try:
        sys.exit(main())
    except Exception as e:
        print(f"{Colors.RED}❌ Error fatal: {e}{Colors.END}")
        sys.exit(1)