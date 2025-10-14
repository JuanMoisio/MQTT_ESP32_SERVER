#!/usr/bin/env python3
"""
🔍 ESP32 System Diagnostic
Diagnostica el estado de ambos ESP32 para resolver problemas
"""

import serial
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

def read_device_output(port, device_name, duration=5):
    """Lee la salida de un dispositivo por un tiempo determinado"""
    print(f"\n{Colors.CYAN}📡 Leyendo {device_name} ({port}) por {duration}s...{Colors.END}")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1)
        
        lines = []
        start_time = time.time()
        
        while time.time() - start_time < duration:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        lines.append(line)
                        print(f"{Colors.YELLOW}[{device_name}]{Colors.END} {line}")
                except:
                    continue
            else:
                time.sleep(0.1)
        
        ser.close()
        return lines
        
    except Exception as e:
        print(f"{Colors.RED}❌ Error conectando a {device_name}: {e}{Colors.END}")
        return []

def send_command_to_device(port, command, device_name):
    """Envía un comando a un dispositivo"""
    print(f"\n{Colors.BLUE}📤 Enviando '{command}' a {device_name}...{Colors.END}")
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(1)
        
        ser.write((command + '\n').encode())
        time.sleep(2)
        
        # Leer respuesta
        response_lines = []
        for _ in range(10):  # Leer hasta 10 líneas de respuesta
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        response_lines.append(line)
                        print(f"{Colors.GREEN}[{device_name} RESP]{Colors.END} {line}")
                except:
                    continue
            else:
                time.sleep(0.1)
        
        ser.close()
        return response_lines
        
    except Exception as e:
        print(f"{Colors.RED}❌ Error enviando comando a {device_name}: {e}{Colors.END}")
        return []

def main():
    print(f"{Colors.BOLD}{Colors.CYAN}🔍 ESP32 SYSTEM DIAGNOSTIC{Colors.END}")
    print("=" * 50)
    
    esp32c3_port = "/dev/cu.usbmodem31201"
    esp32_wroom_port = "/dev/cu.usbserial-3110"
    
    print(f"{Colors.YELLOW}Diagnosticando sistema de huellas dactilares...{Colors.END}")
    
    # 1. Leer estado del ESP32-C3 (Broker)
    print(f"\n{Colors.BOLD}🔸 PASO 1: Verificar ESP32-C3 (Broker MQTT){Colors.END}")
    esp32c3_output = read_device_output(esp32c3_port, "ESP32-C3", 3)
    
    # 2. Enviar comando 'status' al broker
    broker_status = send_command_to_device(esp32c3_port, "status", "ESP32-C3")
    
    # 3. Leer estado del ESP32-WROOM (Cliente)
    print(f"\n{Colors.BOLD}🔸 PASO 2: Verificar ESP32-WROOM (Cliente Fingerprint){Colors.END}")
    esp32_wroom_output = read_device_output(esp32_wroom_port, "ESP32-WROOM", 3)
    
    # 4. Análisis de problemas
    print(f"\n{Colors.BOLD}🔸 PASO 3: Análisis de problemas{Colors.END}")
    
    # Verificar si el broker está funcionando
    broker_working = False
    ap_created = False
    
    all_broker_lines = esp32c3_output + broker_status
    
    for line in all_broker_lines:
        if "Access Point iniciado" in line or "DEPOSITO_BROKER" in line:
            ap_created = True
            broker_working = True
            break
        elif "Sistema listo" in line or "Esperando conexiones" in line:
            broker_working = True
    
    # Verificar estado del cliente
    client_scanning = False
    client_stuck = False
    
    for line in esp32_wroom_output:
        if "Escaneando redes WiFi" in line or "scanning wifi" in line.lower():
            client_scanning = True
        elif "Setup completado" in line:
            client_stuck = True
    
    # Reporte de diagnóstico
    print(f"\n{Colors.BOLD}📋 REPORTE DE DIAGNÓSTICO:{Colors.END}")
    
    if ap_created:
        print(f"{Colors.GREEN}✅ ESP32-C3: Access Point 'DEPOSITO_BROKER' creado{Colors.END}")
    elif broker_working:
        print(f"{Colors.YELLOW}⚠️ ESP32-C3: Funcionando pero AP no confirmado{Colors.END}")
    else:
        print(f"{Colors.RED}❌ ESP32-C3: No responde o no funciona correctamente{Colors.END}")
    
    if client_scanning:
        print(f"{Colors.YELLOW}⚠️ ESP32-WROOM: Atascado escaneando WiFi{Colors.END}")
    elif client_stuck:
        print(f"{Colors.RED}❌ ESP32-WROOM: Setup completado pero sin progreso{Colors.END}")
    else:
        print(f"{Colors.BLUE}ℹ️ ESP32-WROOM: Estado no claro, revisar terminal{Colors.END}")
    
    # Recomendaciones
    print(f"\n{Colors.BOLD}🔧 RECOMENDACIONES:{Colors.END}")
    
    if not ap_created:
        print(f"{Colors.YELLOW}1. Reiniciar ESP32-C3 (presiona botón RESET){Colors.END}")
        print(f"{Colors.YELLOW}2. Verificar que el código del broker se subió correctamente{Colors.END}")
    
    if client_scanning:
        print(f"{Colors.YELLOW}3. Reiniciar ESP32-WROOM (presiona botón RESET){Colors.END}")
        print(f"{Colors.YELLOW}4. Esperar a que aparezca 'DEPOSITO_BROKER' en la lista{Colors.END}")
    
    print(f"\n{Colors.CYAN}💡 Si el problema persiste:{Colors.END}")
    print(f"   - Desconectar y reconectar ambos ESP32")
    print(f"   - Ejecutar el setup completo nuevamente")
    print(f"   - Verificar conexiones del hardware")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}👋 Diagnóstico interrumpido{Colors.END}")
    except Exception as e:
        print(f"{Colors.RED}❌ Error: {e}{Colors.END}")