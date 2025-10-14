#!/usr/bin/env python3
"""
Dual ESP32 Serial Monitor
Monitorea simultáneamente ESP32-C3 (Broker MQTT) y ESP32 WROOM (Cliente Fingerprint)
"""

import serial
import threading
import time
import sys
import os
from datetime import datetime

# Configuración
BAUDRATE = 115200
TIMEOUT = 1

# Colores ANSI para terminal
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

def get_timestamp():
    """Obtiene timestamp formateado"""
    return datetime.now().strftime("%H:%M:%S.%f")[:-3]

def detect_ports():
    """Detecta puertos serie disponibles"""
    import glob
    
    # Buscar puertos serie en macOS
    ports = []
    
    # Puertos USB comunes
    usb_patterns = [
        '/dev/cu.usbmodem*',
        '/dev/cu.usbserial*',
        '/dev/cu.wchusbserial*'
    ]
    
    for pattern in usb_patterns:
        ports.extend(glob.glob(pattern))
    
    return sorted(ports)

def identify_device(port):
    """
    Intenta identificar qué dispositivo está conectado al puerto
    Retorna: 'broker', 'client', o 'unknown'
    """
    try:
        ser = serial.Serial(port, BAUDRATE, timeout=2)
        time.sleep(2)  # Esperar estabilización
        
        # Leer algunas líneas para identificar el dispositivo
        lines_read = 0
        device_type = 'unknown'
        
        while lines_read < 10:  # Leer máximo 10 líneas
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    lines_read += 1
                    
                    # Identificar por mensajes característicos
                    if 'BROKER MQTT ESP32-C3' in line or 'Access Point iniciado' in line:
                        device_type = 'broker'
                        break
                    elif 'HUELLA DACTILAR' in line or 'R305 detectado' in line:
                        device_type = 'client'
                        break
                    elif 'ESP32-C3' in line or 'DEPOSITO_BROKER' in line:
                        device_type = 'broker'
                        break
                    elif 'fingerprint' in line.lower() or 'scan' in line.lower():
                        device_type = 'client'
                        break
                        
                except UnicodeDecodeError:
                    continue
            else:
                time.sleep(0.1)
        
        ser.close()
        return device_type
        
    except Exception as e:
        print(f"Error identificando dispositivo en {port}: {e}")
        return 'unknown'

def monitor_device(port, device_type, device_name):
    """Monitorea un dispositivo específico"""
    color = Colors.CYAN if device_type == 'broker' else Colors.GREEN
    
    try:
        ser = serial.Serial(port, BAUDRATE, timeout=TIMEOUT)
        print(f"{color}{Colors.BOLD}[{device_name}] Conectado a {port}{Colors.END}")
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = get_timestamp()
                        print(f"{color}[{timestamp}] [{device_name}]{Colors.END} {line}")
                except UnicodeDecodeError:
                    continue
            else:
                time.sleep(0.01)
                
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"{Colors.RED}[{device_name}] Error: {e}{Colors.END}")
    finally:
        try:
            ser.close()
        except:
            pass

def send_commands(ports_info):
    """Permite enviar comandos a los dispositivos"""
    print(f"\n{Colors.YELLOW}{Colors.BOLD}=== COMANDOS DISPONIBLES ==={Colors.END}")
    print(f"{Colors.YELLOW}1. Escribir '1:comando' para enviar a ESP32-C3 (Broker){Colors.END}")
    print(f"{Colors.YELLOW}2. Escribir '2:comando' para enviar a ESP32 WROOM (Cliente){Colors.END}")
    print(f"{Colors.YELLOW}3. Escribir 'help' para ver comandos útiles{Colors.END}")
    print(f"{Colors.YELLOW}4. Escribir 'quit' para salir{Colors.END}")
    print(f"{Colors.YELLOW}=============================={Colors.END}\n")
    
    # Abrir conexiones para envío de comandos
    connections = {}
    
    for port, device_type, device_name in ports_info:
        try:
            connections[device_name] = serial.Serial(port, BAUDRATE, timeout=1)
        except Exception as e:
            print(f"{Colors.RED}Error abriendo conexión de comando para {device_name}: {e}{Colors.END}")
    
    try:
        while True:
            try:
                command = input().strip()
                
                if command.lower() == 'quit':
                    break
                elif command.lower() == 'help':
                    print(f"\n{Colors.CYAN}=== COMANDOS ÚTILES ==={Colors.END}")
                    print(f"{Colors.CYAN}Broker (1:):     status, modules, clients{Colors.END}")
                    print(f"{Colors.CYAN}Cliente (2:):    scan, help, status{Colors.END}")
                    print(f"{Colors.CYAN}Ejemplos:{Colors.END}")
                    print(f"{Colors.CYAN}  1:status      - Estado del broker{Colors.END}")
                    print(f"{Colors.CYAN}  2:scan        - Escanear huella{Colors.END}")
                    print(f"{Colors.CYAN}==================={Colors.END}\n")
                    continue
                
                if ':' in command:
                    device_num, cmd = command.split(':', 1)
                    
                    if device_num == '1' and 'ESP32-C3' in connections:
                        connections['ESP32-C3'].write((cmd + '\n').encode())
                        print(f"{Colors.CYAN}[ENVIADO a ESP32-C3] {cmd}{Colors.END}")
                    elif device_num == '2' and 'ESP32-WROOM' in connections:
                        connections['ESP32-WROOM'].write((cmd + '\n').encode())
                        print(f"{Colors.GREEN}[ENVIADO a ESP32-WROOM] {cmd}{Colors.END}")
                    else:
                        print(f"{Colors.RED}Dispositivo no encontrado o comando inválido{Colors.END}")
                else:
                    print(f"{Colors.YELLOW}Formato: dispositivo:comando (ej: 1:status o 2:scan){Colors.END}")
                    
            except KeyboardInterrupt:
                break
            except Exception as e:
                print(f"{Colors.RED}Error enviando comando: {e}{Colors.END}")
                
    finally:
        for conn in connections.values():
            try:
                conn.close()
            except:
                pass

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 60)
    print("    DUAL ESP32 SERIAL MONITOR")
    print("    ESP32-C3 (Broker) + ESP32-WROOM (Cliente)")
    print("=" * 60)
    print(f"{Colors.END}")
    
    # Detectar puertos
    ports = detect_ports()
    
    if not ports:
        print(f"{Colors.RED}❌ No se encontraron puertos serie{Colors.END}")
        return
    
    print(f"{Colors.CYAN}🔍 Puertos detectados: {ports}{Colors.END}")
    
    # Identificar dispositivos
    print(f"{Colors.YELLOW}🔎 Identificando dispositivos...{Colors.END}")
    
    ports_info = []
    for port in ports:
        print(f"   Analizando {port}...")
        device_type = identify_device(port)
        
        if device_type == 'broker':
            device_name = 'ESP32-C3'
            print(f"   ✅ {port} -> {Colors.CYAN}ESP32-C3 (Broker MQTT){Colors.END}")
        elif device_type == 'client':
            device_name = 'ESP32-WROOM'
            print(f"   ✅ {port} -> {Colors.GREEN}ESP32-WROOM (Cliente Fingerprint){Colors.END}")
        else:
            device_name = f'Desconocido-{port.split("/")[-1]}'
            print(f"   ⚠️ {port} -> {Colors.YELLOW}Dispositivo desconocido{Colors.END}")
        
        ports_info.append((port, device_type, device_name))
    
    if not ports_info:
        print(f"{Colors.RED}❌ No se pudieron identificar dispositivos{Colors.END}")
        return
    
    print(f"\n{Colors.BOLD}🚀 Iniciando monitoreo...{Colors.END}")
    print(f"{Colors.YELLOW}Presiona Ctrl+C para salir{Colors.END}\n")
    
    # Crear threads para monitoreo
    threads = []
    
    for port, device_type, device_name in ports_info:
        thread = threading.Thread(
            target=monitor_device, 
            args=(port, device_type, device_name),
            daemon=True
        )
        threads.append(thread)
        thread.start()
    
    # Thread para envío de comandos
    command_thread = threading.Thread(
        target=send_commands,
        args=(ports_info,),
        daemon=True
    )
    command_thread.start()
    
    try:
        # Mantener el programa corriendo
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Cerrando monitor...{Colors.END}")

if __name__ == "__main__":
    try:
        import serial
    except ImportError:
        print(f"{Colors.RED}❌ Instalar pyserial: pip install pyserial{Colors.END}")
        sys.exit(1)
    
    main()