#!/usr/bin/env python3
"""
📺 Monitor ESP32 en Terminal Única
Monitorea ambos ESP32 en la misma terminal con colores
"""

import serial
import threading
import time
import sys
import select
import subprocess
import os
import signal
import serial.tools.list_ports
from datetime import datetime

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

def get_timestamp():
    return datetime.now().strftime("%H:%M:%S")

def detect_esp32_devices():
    """Detecta automáticamente los puertos ESP32 conectados"""
    print(f"{Colors.YELLOW}🔍 Detectando dispositivos ESP32...{Colors.END}")
    
    devices = {"ESP32-C3": None, "ESP32-WROOM": None}
    
    # Listar todos los puertos serie
    ports = serial.tools.list_ports.comports()
    
    for port in ports:
        port_name = port.device
        port_desc = port.description.lower()
        port_hwid = port.hwid.lower() if port.hwid else ""
        
        print(f"{Colors.BLUE}   📱 Analizando: {port_name} - {port.description}{Colors.END}")
        
        # Detectar ESP32-C3 (generalmente aparece como "USB Serial Device" o contiene "CP210x")
        if any(keyword in port_desc for keyword in ['usb serial', 'cp210x', 'silicon labs']):
            if 'usbmodem' in port_name:
                devices["ESP32-C3"] = port_name
                print(f"{Colors.CYAN}   ✅ ESP32-C3 detectado: {port_name}{Colors.END}")
            elif 'usbserial' in port_name:
                devices["ESP32-WROOM"] = port_name
                print(f"{Colors.GREEN}   ✅ ESP32-WROOM detectado: {port_name}{Colors.END}")
        
        # Detectar por VID/PID si está disponible
        elif 'vid:pid' in port_hwid or '10c4:ea60' in port_hwid:  # CP210x
            if 'usbmodem' in port_name:
                devices["ESP32-C3"] = port_name
                print(f"{Colors.CYAN}   ✅ ESP32-C3 detectado por VID/PID: {port_name}{Colors.END}")
            elif 'usbserial' in port_name:
                devices["ESP32-WROOM"] = port_name  
                print(f"{Colors.GREEN}   ✅ ESP32-WROOM detectado por VID/PID: {port_name}{Colors.END}")
    
    # Intentar detección por patrón de puerto si no encontró por descripción
    if not devices["ESP32-C3"] or not devices["ESP32-WROOM"]:
        print(f"{Colors.YELLOW}   🔄 Usando detección por patrón de puerto...{Colors.END}")
        
        for port in ports:
            port_name = port.device
            
            if not devices["ESP32-C3"] and 'usbmodem' in port_name:
                devices["ESP32-C3"] = port_name
                print(f"{Colors.CYAN}   📍 ESP32-C3 asignado: {port_name}{Colors.END}")
            elif not devices["ESP32-WROOM"] and 'usbserial' in port_name:
                devices["ESP32-WROOM"] = port_name
                print(f"{Colors.GREEN}   📍 ESP32-WROOM asignado: {port_name}{Colors.END}")
    
    return devices

def validate_device_connection(port, device_name, timeout=3):
    """Valida que un dispositivo esté realmente conectado y respondiendo"""
    try:
        print(f"{Colors.BLUE}   🔗 Validando {device_name} en {port}...{Colors.END}")
        ser = serial.Serial(port, 115200, timeout=1)
        
        # Dar tiempo para inicializar
        time.sleep(0.5)
        
        # Verificar si hay datos en el buffer
        start_time = time.time()
        has_data = False
        
        while time.time() - start_time < timeout:
            if ser.in_waiting > 0:
                has_data = True
                break
            time.sleep(0.1)
        
        ser.close()
        
        if has_data:
            print(f"{Colors.GREEN}   ✅ {device_name} validated - receiving data{Colors.END}")
            return True
        else:
            print(f"{Colors.YELLOW}   ⚠️ {device_name} connected but no data (may be normal){Colors.END}")
            return True  # Aún es válido, puede estar sin enviar datos
            
    except Exception as e:
        print(f"{Colors.RED}   ❌ {device_name} validation failed: {e}{Colors.END}")
        return False

def cleanup_serial_connections(detected_ports=None):
    """Limpia conexiones serie previas que puedan estar ocupando los puertos"""
    print(f"{Colors.YELLOW}🧹 Limpiando conexiones serie previas...{Colors.END}")
    
    # Usar puertos detectados o todos los puertos serie si no se especifican
    if detected_ports:
        ports = [port for port in detected_ports.values() if port]
    else:
        # Buscar todos los puertos serie USB
        all_ports = serial.tools.list_ports.comports()
        ports = [port.device for port in all_ports if 'usb' in port.device.lower()]
    
    try:
        # 1. Matar sesiones de screen
        print(f"{Colors.BLUE}   • Cerrando sesiones de screen...{Colors.END}")
        subprocess.run(['pkill', '-f', 'screen.*cu.usb'], capture_output=True)
        
        # 2. Matar procesos que usen los puertos
        print(f"{Colors.BLUE}   • Liberando puertos serie...{Colors.END}")
        for port in ports:
            try:
                # Buscar procesos que usen el puerto específico
                result = subprocess.run(['lsof', port], capture_output=True, text=True)
                if result.returncode == 0:
                    # Extraer PIDs de los procesos
                    lines = result.stdout.strip().split('\n')[1:]  # Skip header
                    for line in lines:
                        if line.strip():
                            pid = line.split()[1]
                            print(f"{Colors.CYAN}     Liberando PID {pid} de {port}...{Colors.END}")
                            try:
                                os.kill(int(pid), signal.SIGTERM)
                                time.sleep(0.5)
                                # Si no se cerró con SIGTERM, usar SIGKILL
                                try:
                                    os.kill(int(pid), 0)  # Verificar si aún existe
                                    os.kill(int(pid), signal.SIGKILL)
                                    print(f"{Colors.RED}     Forzado cierre de PID {pid}{Colors.END}")
                                except ProcessLookupError:
                                    pass  # Ya se cerró
                            except (ValueError, ProcessLookupError):
                                pass
            except subprocess.SubprocessError:
                pass
                
        # 3. Pequeña espera para que se liberen los recursos
        time.sleep(2)
        
        print(f"{Colors.GREEN}✅ Puertos serie limpiados{Colors.END}")
        
    except Exception as e:
        print(f"{Colors.YELLOW}⚠️ Advertencia limpiando puertos: {e}{Colors.END}")
        print(f"{Colors.YELLOW}   Continuando de todas formas...{Colors.END}")
    
    print()

def monitor_device(port, device_name, color):
    """Monitorea un dispositivo y muestra output con colores"""
    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
        print(f"{color}[{device_name}] Conectado a {port}{Colors.END}")
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = get_timestamp()
                        print(f"{color}[{timestamp}] [{device_name}]{Colors.END} {line}")
                        sys.stdout.flush()
                except:
                    continue
            else:
                time.sleep(0.01)
                
    except Exception as e:
        print(f"{Colors.RED}[{device_name}] Error: {e}{Colors.END}")

def handle_physical_reset(target, detected_ports):
    """Maneja resets físicos usando señales DTR/RTS"""
    esp32c3_port = detected_ports.get("ESP32-C3")
    esp32_wroom_port = detected_ports.get("ESP32-WROOM")
    
    def reset_device(port, name):
        if not port:
            print(f"{Colors.RED}❌ {name} no detectado - no se puede resetear{Colors.END}")
            return False
            
        try:
            print(f"{Colors.YELLOW}🔄 Reseteando {name} en {port}...{Colors.END}")
            ser = serial.Serial(port, 115200, timeout=1)
            
            # Reset usando DTR (Data Terminal Ready)
            ser.dtr = False
            time.sleep(0.1)
            ser.dtr = True
            time.sleep(0.1)
            ser.dtr = False
            
            # Reset usando RTS (Request To Send) - para ESP32
            ser.rts = False
            time.sleep(0.1)
            ser.rts = True
            time.sleep(0.1)
            ser.rts = False
            
            ser.close()
            print(f"{Colors.GREEN}✅ {name} reseteado{Colors.END}")
            return True
            
        except Exception as e:
            print(f"{Colors.RED}❌ Error reseteando {name}: {e}{Colors.END}")
            return False
    
    if target.lower() == 'server':
        reset_device(esp32c3_port, "ESP32-C3 (SERVER)")
    elif target.lower() == 'client':
        reset_device(esp32_wroom_port, "ESP32-WROOM (CLIENT)")
    elif target.lower() == 'both':
        print(f"{Colors.MAGENTA}🔄 Reseteando ambos dispositivos...{Colors.END}")
        reset_device(esp32c3_port, "ESP32-C3 (SERVER)")
        time.sleep(2)
        reset_device(esp32_wroom_port, "ESP32-WROOM (CLIENT)")
        print(f"{Colors.GREEN}✅ Ambos dispositivos reseteados{Colors.END}")
    else:
        print(f"{Colors.RED}❌ Target inválido. Usa: server, client, o both{Colors.END}")

def send_commands(detected_ports):
    """Permite enviar comandos a los dispositivos"""
    esp32c3_port = detected_ports.get("ESP32-C3")
    esp32_wroom_port = detected_ports.get("ESP32-WROOM")
    
    connections = {}
    
    try:
        if esp32c3_port:
            connections['SERVER'] = serial.Serial(esp32c3_port, 115200, timeout=1)
        if esp32_wroom_port:
            connections['CLIENT'] = serial.Serial(esp32_wroom_port, 115200, timeout=1)
        
        print(f"\n{Colors.YELLOW}📤 Comandos disponibles:{Colors.END}")
        print(f"{Colors.CYAN}  server:comando  - Enviar a SERVER (Broker MQTT){Colors.END}")
        print(f"{Colors.CYAN}  client:comando  - Enviar a CLIENT (Fingerprint){Colors.END}")
        print(f"{Colors.CYAN}  help            - Ver ayuda{Colors.END}")
        print(f"{Colors.CYAN}  quit            - Salir{Colors.END}\n")
        
        while True:
            try:
                # Usar select para input no bloqueante en macOS
                if sys.stdin in select.select([sys.stdin], [], [], 0.1)[0]:
                    command = sys.stdin.readline().strip()
                    
                    if command.lower() == 'quit':
                        break
                    elif command.lower() == 'help':
                        print(f"\n{Colors.CYAN}Comandos útiles:{Colors.END}")
                        print(f"{Colors.CYAN}  server:status  - Estado del broker MQTT{Colors.END}")
                        print(f"{Colors.CYAN}  server:reset   - Resetear ESP32-C3 (software){Colors.END}")
                        print(f"{Colors.CYAN}  client:scan    - Escanear huella dactilar{Colors.END}")
                        print(f"{Colors.CYAN}  client:help    - Ayuda del cliente{Colors.END}")
                        print(f"{Colors.CYAN}  client:reset   - Resetear ESP32-WROOM (software){Colors.END}")
                        print(f"{Colors.CYAN}  client:2       - Seleccionar red #2 (ejemplo){Colors.END}")
                        print(f"{Colors.CYAN}  reset:server   - Reset físico ESP32-C3{Colors.END}")
                        print(f"{Colors.CYAN}  reset:client   - Reset físico ESP32-WROOM{Colors.END}")
                        print(f"{Colors.CYAN}  reset:both     - Reset físico ambos ESP32{Colors.END}\n")
                    elif ':' in command:
                        device, cmd = command.split(':', 1)
                        
                        if device.lower() == 'server' and 'SERVER' in connections:
                            connections['SERVER'].write((command + '\n').encode())  # Envía comando completo
                            print(f"{Colors.CYAN}[ENVIADO a SERVER] {command}{Colors.END}")
                        elif device.lower() == 'client' and 'CLIENT' in connections:
                            connections['CLIENT'].write((cmd + '\n').encode())
                            print(f"{Colors.GREEN}[ENVIADO a CLIENT] {cmd}{Colors.END}")
                        elif device.lower() == 'reset':
                            handle_physical_reset(cmd, detected_ports)
                        else:
                            print(f"{Colors.RED}Dispositivo no válido. Usa: server:comando, client:comando o reset:dispositivo{Colors.END}")
                    else:
                        print(f"{Colors.YELLOW}Formato: dispositivo:comando (ej: server:status, client:scan, reset:both){Colors.END}")
                        
            except KeyboardInterrupt:
                break
            except:
                continue
                
    except Exception as e:
        print(f"{Colors.RED}Error con conexiones de comando: {e}{Colors.END}")
    finally:
        for conn in connections.values():
            try:
                conn.close()
            except:
                pass

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 60)
    print("    📺 ESP32 DUAL MONITOR - AUTO DETECTION")
    print("=" * 60)
    print(f"{Colors.END}")
    
    # Detectar dispositivos automáticamente
    detected_ports = detect_esp32_devices()
    
    # Verificar que se detectaron ambos dispositivos
    if not detected_ports["ESP32-C3"]:
        print(f"{Colors.RED}❌ ESP32-C3 no detectado. Verifica conexión USB.{Colors.END}")
        return False
        
    if not detected_ports["ESP32-WROOM"]:
        print(f"{Colors.RED}❌ ESP32-WROOM no detectado. Verifica conexión USB.{Colors.END}")
        return False
    
    print(f"\n{Colors.GREEN}✅ Dispositivos detectados:{Colors.END}")
    print(f"{Colors.CYAN}   🔷 ESP32-C3 (SERVER): {detected_ports['ESP32-C3']}{Colors.END}")
    print(f"{Colors.GREEN}   🔶 ESP32-WROOM (CLIENT): {detected_ports['ESP32-WROOM']}{Colors.END}")
    
    # Limpiar conexiones previas con los puertos detectados
    cleanup_serial_connections(detected_ports)
    
    # Validar conexiones
    print(f"\n{Colors.YELLOW}🔍 Validando conexiones...{Colors.END}")
    if not validate_device_connection(detected_ports["ESP32-C3"], "ESP32-C3"):
        print(f"{Colors.YELLOW}⚠️ Continuando sin validación completa...{Colors.END}")
    if not validate_device_connection(detected_ports["ESP32-WROOM"], "ESP32-WROOM"):
        print(f"{Colors.YELLOW}⚠️ Continuando sin validación completa...{Colors.END}")
    
    print(f"\n{Colors.CYAN}🚀 Iniciando monitores...{Colors.END}")
    print(f"{Colors.YELLOW}Presiona Ctrl+C para salir{Colors.END}\n")
    
    # Crear threads para monitoreo con puertos detectados
    thread1 = threading.Thread(
        target=monitor_device,
        args=(detected_ports["ESP32-C3"], "SERVER", Colors.CYAN),
        daemon=True
    )
    
    thread2 = threading.Thread(
        target=monitor_device, 
        args=(detected_ports["ESP32-WROOM"], "CLIENT", Colors.GREEN),
        daemon=True
    )
    
    # Iniciar threads
    thread1.start()
    thread2.start()
    
    # Esperar un poco para que se estabilicen
    time.sleep(2)
    
    # Manejar comandos con puertos detectados
    try:
        send_commands(detected_ports)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Cerrando monitor...{Colors.END}")
        print(f"{Colors.BLUE}🧹 Limpiando recursos...{Colors.END}")
        cleanup_serial_connections(detected_ports)
        
    return True

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Programa interrumpido{Colors.END}")
        cleanup_serial_connections()
    except Exception as e:
        print(f"{Colors.RED}❌ Error: {e}{Colors.END}")
        cleanup_serial_connections()
        sys.exit(1)