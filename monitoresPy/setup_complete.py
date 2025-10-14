#!/usr/bin/env python3
"""
ESP32 Fingerprint System - Setup Completo
Automatiza la configuración completa del sistema de huellas dactilares
"""

import subprocess
import time
import sys
import os

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
    END = '\033[0m'

def print_step(step, description):
    print(f"{Colors.CYAN}{Colors.BOLD}[PASO {step}]{Colors.END} {description}")

def print_success(message):
    print(f"{Colors.GREEN}✅ {message}{Colors.END}")

def print_error(message):
    print(f"{Colors.RED}❌ {message}{Colors.END}")

def print_warning(message):
    print(f"{Colors.YELLOW}⚠️ {message}{Colors.END}")

def print_info(message):
    print(f"{Colors.BLUE}ℹ️ {message}{Colors.END}")

def run_command(cmd, description, check_output=False):
    """Ejecuta un comando y muestra el resultado"""
    print(f"{Colors.YELLOW}Ejecutando: {' '.join(cmd)}{Colors.END}")
    
    try:
        if check_output:
            result = subprocess.run(cmd, capture_output=True, text=True, check=True)
            return result.stdout.strip()
        else:
            result = subprocess.run(cmd, check=True)
            return True
    except subprocess.CalledProcessError as e:
        print_error(f"{description} falló: {e}")
        return False
    except FileNotFoundError:
        print_error(f"Comando no encontrado: {' '.join(cmd)}")
        return False

def check_ports():
    """Verifica que los puertos estén disponibles"""
    print_step(0, "Verificando puertos serie...")
    
    import glob
    ports = glob.glob('/dev/cu.*')
    result = '\n'.join(ports)
    if not ports:
        return False
        
    ports = result.split('\n')
    esp32c3_port = None
    esp32_wroom_port = None
    
    for port in ports:
        if 'usbmodem' in port:
            esp32c3_port = port
        elif 'usbserial' in port:
            esp32_wroom_port = port
    
    if esp32c3_port and esp32_wroom_port:
        print_success(f"ESP32-C3: {esp32c3_port}")
        print_success(f"ESP32-WROOM: {esp32_wroom_port}")
        return esp32c3_port, esp32_wroom_port
    else:
        print_error("No se encontraron ambos dispositivos")
        print_info("Conecta ambos ESP32 y verifica las conexiones USB")
        return False

def upload_broker():
    """Sube el código del broker al ESP32-C3"""
    print_step(1, "Subiendo código al ESP32-C3 (Broker MQTT)...")
    
    broker_path = "/Users/jmoisio/Documents/PlatformIO/Projects/BorkerMQTT"
    pio_path = "/Users/jmoisio/.platformio/penv/bin/platformio"
    
    # Cambiar al directorio del broker
    os.chdir(broker_path)
    
    # Subir código
    cmd = [pio_path, "run", "--target", "upload", "-e", "esp32c3-supermini"]
    if run_command(cmd, "Upload ESP32-C3"):
        print_success("ESP32-C3 programado exitosamente")
        return True
    else:
        print_error("Error programando ESP32-C3")
        return False

def upload_client():
    """Sube el código del cliente al ESP32 WROOM"""
    print_step(2, "Subiendo código al ESP32-WROOM (Cliente Fingerprint)...")
    
    client_path = "/Users/jmoisio/Documents/PlatformIO/Projects/HuellaDactilar"
    pio_path = "/Users/jmoisio/.platformio/penv/bin/platformio"
    
    # Cambiar al directorio del cliente
    os.chdir(client_path)
    
    # Subir código
    cmd = [pio_path, "run", "--target", "upload"]
    if run_command(cmd, "Upload ESP32-WROOM"):
        print_success("ESP32-WROOM programado exitosamente")
        return True
    else:
        print_error("Error programando ESP32-WROOM")
        return False

def open_monitors():
    """Abre los monitores serie para ambos dispositivos"""
    print_step(3, "Abriendo monitores serie...")
    
    monitor_script = "/Users/jmoisio/Documents/PlatformIO/Projects/monitoresPy/simple_monitor.py"
    
    if run_command(["python3", monitor_script], "Abrir monitores"):
        print_success("Monitores serie abiertos")
        return True
    else:
        print_error("Error abriendo monitores")
        return False

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 70)
    print("    🔐 SISTEMA DE HUELLAS DACTILARES ESP32")
    print("    Setup Automático Completo")
    print("=" * 70)
    print(f"{Colors.END}")
    
    print_info("Este script configurará todo el sistema automáticamente:")
    print("  • ESP32-C3 SuperMini → Broker MQTT")
    print("  • ESP32-WROOM → Cliente Fingerprint + R305")
    print("  • Monitores serie duales")
    print()
    
    # Verificar puertos
    ports = check_ports()
    if not ports:
        return 1
    
    esp32c3_port, esp32_wroom_port = ports
    
    print()
    print_step("", "Iniciando configuración automática...")
    print()
    
    # Subir código al broker
    if not upload_broker():
        return 1
    
    print_info("Esperando que el ESP32-C3 se reinicie...")
    time.sleep(3)
    
    # Subir código al cliente
    if not upload_client():
        return 1
    
    print_info("Esperando que el ESP32-WROOM se reinicie...")
    time.sleep(3)
    
    # Abrir monitores
    if not open_monitors():
        return 1
    
    # Instrucciones finales
    print()
    print(f"{Colors.GREEN}{Colors.BOLD}🎉 ¡CONFIGURACIÓN COMPLETADA!{Colors.END}")
    print()
    print(f"{Colors.CYAN}📋 PRÓXIMOS PASOS:{Colors.END}")
    print("1. En la Terminal del ESP32-C3:")
    print(f"   → Verificar: 'Access Point iniciado: DEPOSITO_BROKER'")
    print()
    print("2. En la Terminal del ESP32-WROOM:")
    print(f"   → Buscar 'DEPOSITO_BROKER' en la lista de redes")
    print(f"   → Seleccionar el número correspondiente")
    print(f"   → Introducir contraseña: {Colors.YELLOW}deposito123{Colors.END}")
    print()
    print("3. Una vez conectado, comandos disponibles:")
    print(f"   → {Colors.GREEN}scan{Colors.END} - Escanear huella dactilar")
    print(f"   → {Colors.GREEN}help{Colors.END} - Ver todos los comandos")
    print(f"   → {Colors.GREEN}status{Colors.END} - Estado del sistema")
    print()
    print(f"{Colors.MAGENTA}🔧 Para salir de los monitores: Ctrl+A, luego K, luego Y{Colors.END}")
    
    return 0

if __name__ == "__main__":
    try:
        exit_code = main()
        sys.exit(exit_code)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}Configuración interrumpida por el usuario{Colors.END}")
        sys.exit(1)
    except Exception as e:
        print_error(f"Error inesperado: {e}")
        sys.exit(1)