#!/usr/bin/env python3
"""
🔄 ESP32 Reset Helper
Ayuda a resetear y reconfigurar el sistema cuando se queda atascado
"""

import subprocess
import time
import sys

# Colores
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

def kill_screen_sessions():
    """Mata todas las sesiones de screen activas"""
    print(f"{Colors.YELLOW}🔄 Cerrando sesiones de screen activas...{Colors.END}")
    
    try:
        # Listar sesiones de screen
        result = subprocess.run(['screen', '-ls'], capture_output=True, text=True)
        
        if "There is a screen on" in result.stdout or "There are screens on" in result.stdout:
            # Matar todas las sesiones
            subprocess.run(['pkill', '-f', 'screen'], capture_output=True)
            time.sleep(2)
            print(f"{Colors.GREEN}✅ Sesiones de screen cerradas{Colors.END}")
        else:
            print(f"{Colors.BLUE}ℹ️ No hay sesiones de screen activas{Colors.END}")
            
    except Exception as e:
        print(f"{Colors.RED}❌ Error cerrando sesiones: {e}{Colors.END}")

def reset_system():
    """Resetea completamente el sistema"""
    print(f"{Colors.BOLD}{Colors.CYAN}🔄 ESP32 SYSTEM RESET{Colors.END}")
    print("=" * 40)
    
    # 1. Cerrar sesiones de terminal
    kill_screen_sessions()
    
    # 2. Instrucciones para reseteo físico
    print(f"\n{Colors.BOLD}📋 PASOS PARA RESETEAR:{Colors.END}")
    print(f"{Colors.YELLOW}1. Presiona el botón RESET en ambos ESP32{Colors.END}")
    print(f"   • ESP32-C3 (el más pequeño)")
    print(f"   • ESP32-WROOM (el más grande)")
    
    print(f"\n{Colors.YELLOW}2. Espera 5 segundos...{Colors.END}")
    for i in range(5, 0, -1):
        print(f"   ⏱️ {i}...")
        time.sleep(1)
    
    print(f"\n{Colors.GREEN}✅ Ahora abriendo nuevos monitores...{Colors.END}")
    
    # 3. Abrir nuevos monitores
    time.sleep(2)
    
    monitor_script = "/Users/jmoisio/Documents/PlatformIO/Projects/monitoresPy/simple_monitor.py"
    subprocess.run(["python3", monitor_script])
    
    print(f"\n{Colors.CYAN}📋 QUÉ BUSCAR EN LAS TERMINALES:{Colors.END}")
    print(f"{Colors.GREEN}Terminal ESP32-C3:{Colors.END}")
    print(f"   → 'Access Point iniciado: DEPOSITO_BROKER'")
    print(f"   → 'IP del broker: 192.168.4.1'")
    
    print(f"\n{Colors.GREEN}Terminal ESP32-WROOM:{Colors.END}")
    print(f"   → Lista de redes WiFi")
    print(f"   → 'DEPOSITO_BROKER' debe aparecer en la lista")
    
    print(f"\n{Colors.YELLOW}Si 'DEPOSITO_BROKER' aparece:{Colors.END}")
    print(f"   1. Anota el número de la red")
    print(f"   2. Escribe ese número y presiona Enter")
    print(f"   3. Escribe la contraseña: {Colors.BOLD}deposito123{Colors.END}")

def main():
    try:
        reset_system()
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}👋 Reset cancelado{Colors.END}")
    except Exception as e:
        print(f"{Colors.RED}❌ Error: {e}{Colors.END}")

if __name__ == "__main__":
    main()