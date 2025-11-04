#!/usr/bin/env python3
"""
🔐 ESP32 Fingerprint System - Launcher Simple
Launcher directo sin menús complicados
"""

import subprocess
import sys
import os
import glob
from pathlib import Path

# Colores
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

def detect_ports():
    """Detecta puertos ESP32"""
    ports = glob.glob('/dev/cu.*')
    
    esp32c3 = None
    esp32_wroom = None
    
    for port in ports:
        if 'usbmodem' in port:
            esp32c3 = port
        elif 'usbserial' in port:
            esp32_wroom = port
    
    return esp32c3, esp32_wroom

def main():
    print(f"{Colors.BOLD}{Colors.CYAN}🔐 ESP32 Fingerprint System{Colors.END}")
    print(f"{Colors.CYAN}=" * 40 + f"{Colors.END}")
    
    # Detectar dispositivos
    esp32c3, esp32_wroom = detect_ports()
    
    if esp32c3 and esp32_wroom:
        print(f"{Colors.GREEN}✅ ESP32-C3: {esp32c3}{Colors.END}")
        print(f"{Colors.GREEN}✅ ESP32-WROOM: {esp32_wroom}{Colors.END}")
    else:
        print(f"{Colors.RED}❌ Conecta ambos ESP32 via USB{Colors.END}")
        return 1
    
    print(f"\n{Colors.YELLOW}Selecciona una opción:{Colors.END}")
    print(f"{Colors.CYAN}1.{Colors.END} 🚀 Auto-Upload (Detecta y sube código)")
    print(f"{Colors.CYAN}2.{Colors.END} 📺 Monitor con Auto-Detección")
    print(f"{Colors.CYAN}3.{Colors.END} �️ Setup Completo (Upload + Monitor)")
    print(f"{Colors.CYAN}4.{Colors.END} ❌ Salir")
    
    try:
        choice = input(f"\n{Colors.BOLD}Opción (1-4): {Colors.END}").strip()
        
        if choice == '1':
            print(f"{Colors.YELLOW}🚀 Ejecutando auto-upload (setup completo)...{Colors.END}")
            script = Path(__file__).parent / "setup_complete.py"
            subprocess.run(["python3", str(script)])
            
        elif choice == '2':
            print(f"{Colors.YELLOW}📺 Abriendo monitor canónico (monitor_cli)...{Colors.END}")
            script = Path(__file__).parent / "monitor_cli.py"
            subprocess.run(["python3", str(script)])
            
        elif choice == '3':
            print(f"{Colors.YELLOW}🛠️ Ejecutando setup completo...{Colors.END}")
            script = Path(__file__).parent / "setup_complete.py"
            subprocess.run(["python3", str(script)])
            
        elif choice == '4':
            print(f"{Colors.GREEN}👋 ¡Hasta luego!{Colors.END}")
            
        else:
            print(f"{Colors.RED}❌ Opción inválida{Colors.END}")
            
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}👋 Saliendo...{Colors.END}")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())