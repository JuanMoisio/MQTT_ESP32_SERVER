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
import termios
import tty
from datetime import datetime

# Configuración
MAX_HISTORY = 100  # Máximo de comandos en historial

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

class CommandHistory:
    """Maneja el historial de comandos con navegación"""
    
    def __init__(self, max_size=MAX_HISTORY):
        self.history = []
        self.max_size = max_size
        self.current_index = 0
        self.temp_command = ""  # Para guardar el comando actual mientras navegas
        
    def add_command(self, command):
        """Agrega un comando al historial"""
        if command.strip() and (not self.history or self.history[-1] != command):
            self.history.append(command)
            if len(self.history) > self.max_size:
                self.history.pop(0)
        self.reset_navigation()
    
    def reset_navigation(self):
        """Resetea la navegación al final del historial"""
        self.current_index = len(self.history)
        self.temp_command = ""
    
    def get_previous(self, current_command=""):
        """Obtiene el comando anterior (flecha arriba)"""
        if self.current_index == len(self.history):
            self.temp_command = current_command
        
        if self.current_index > 0:
            self.current_index -= 1
            return self.history[self.current_index]
        return current_command
    
    def get_next(self):
        """Obtiene el comando siguiente (flecha abajo)"""
        if self.current_index < len(self.history) - 1:
            self.current_index += 1
            return self.history[self.current_index]
        elif self.current_index == len(self.history) - 1:
            self.current_index += 1
            return self.temp_command
        return ""
    
    def get_stats(self):
        """Obtiene estadísticas del historial"""
        return len(self.history), self.current_index

class InputHandler:
    """Maneja la entrada de comandos con historial"""
    
    def __init__(self):
        self.history = CommandHistory()
        self.current_command = ""
        self.cursor_pos = 0
        
    def handle_special_key(self, key):
        """Maneja teclas especiales (flechas, etc.)"""
        if key == '\x1b[A':  # Flecha arriba
            new_command = self.history.get_previous(self.current_command)
            self.set_command(new_command)
            return True
        elif key == '\x1b[B':  # Flecha abajo
            new_command = self.history.get_next()
            self.set_command(new_command)
            return True
        elif key == '\x1b[C':  # Flecha derecha
            if self.cursor_pos < len(self.current_command):
                self.cursor_pos += 1
                self.move_cursor_right()
            return True
        elif key == '\x1b[D':  # Flecha izquierda
            if self.cursor_pos > 0:
                self.cursor_pos -= 1
                self.move_cursor_left()
            return True
        return False
    
    def set_command(self, command):
        """Establece el comando actual y actualiza la pantalla"""
        # Limpiar línea actual
        print(f'\r{" " * (len(self.current_command) + 10)}\r', end='')
        
        self.current_command = command
        self.cursor_pos = len(command)
        
        # Mostrar nuevo comando
        print(f'{Colors.YELLOW}> {command}{Colors.END}', end='', flush=True)
    
    def add_char(self, char):
        """Agrega un carácter en la posición del cursor"""
        if char == '\x7f' or char == '\x08':  # Backspace
            if self.cursor_pos > 0:
                self.current_command = (self.current_command[:self.cursor_pos-1] + 
                                      self.current_command[self.cursor_pos:])
                self.cursor_pos -= 1
                self.refresh_line()
        elif char == '\n' or char == '\r':  # Enter
            return 'enter'
        elif char.isprintable():
            self.current_command = (self.current_command[:self.cursor_pos] + char + 
                                  self.current_command[self.cursor_pos:])
            self.cursor_pos += 1
            self.refresh_line()
        return 'continue'
    
    def refresh_line(self):
        """Refresca la línea de comando completa"""
        print(f'\r{" " * 100}\r', end='')  # Limpiar línea
        print(f'{Colors.YELLOW}> {self.current_command}{Colors.END}', end='', flush=True)
        
        # Mover cursor a la posición correcta
        if self.cursor_pos < len(self.current_command):
            move_back = len(self.current_command) - self.cursor_pos
            print(f'\x1b[{move_back}D', end='', flush=True)
    
    def move_cursor_left(self):
        """Mueve el cursor una posición a la izquierda"""
        print('\x1b[D', end='', flush=True)
    
    def move_cursor_right(self):
        """Mueve el cursor una posición a la derecha"""
        print('\x1b[C', end='', flush=True)
    
    def get_command(self):
        """Obtiene el comando actual y lo agrega al historial"""
        command = self.current_command
        if command.strip():
            self.history.add_command(command)
        self.current_command = ""
        self.cursor_pos = 0
        return command

def get_timestamp():
    return datetime.now().strftime("%H:%M:%S")

def get_char():
    """Obtiene un carácter sin presionar Enter"""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        
        # Verificar si hay entrada disponible
        if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
            ch = sys.stdin.read(1)
            
            # Manejar secuencias de escape (flechas)
            if ch == '\x1b':
                # Leer el resto de la secuencia con timeout corto
                if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                    ch2 = sys.stdin.read(1)
                    if ch2 == '[':
                        if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                            ch3 = sys.stdin.read(1)
                            return f'\x1b[{ch3}'
                return ch
            return ch
        return None
    except:
        return None
    finally:
        try:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        except:
            pass

def identify_device_type(port):
    """Identifica qué tipo de dispositivo está conectado al puerto"""
    try:
        ser = serial.Serial(port, 115200, timeout=3)
        time.sleep(2)  # Esperar estabilización
        
        lines_read = 0
        device_type = 'unknown'
        
        # Enviar un comando de reset suave para obtener mensajes de inicio
        ser.write(b'\r\n')
        time.sleep(1)
        
        while lines_read < 20:  # Leer más líneas para mejor identificación
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    lines_read += 1
                    
                    # Identificar por mensajes específicos de inicio
                    if any(keyword in line for keyword in ['BROKER MQTT', 'Access Point iniciado', 'Broker MQTT']):
                        device_type = 'broker'
                        break
                    elif any(keyword in line for keyword in ['HUELLA DACTILAR', 'FINGERPRINT', 'fingerprint_scanner']):
                        device_type = 'fingerprint'
                        break
                    elif any(keyword in line for keyword in ['LECTOR RFID', 'RFID', 'rfid_reader']):
                        device_type = 'rfid'
                        break
                        
                except UnicodeDecodeError:
                    continue
            else:
                time.sleep(0.1)
        
        ser.close()
        return device_type
        
    except Exception as e:
        print(f"   ⚠️ Error identificando {port}: {e}")
        return 'unknown'

def detect_esp32_devices():
    """Detecta automáticamente los puertos ESP32 conectados y los identifica"""
    print(f"{Colors.YELLOW}🔍 Detectando dispositivos ESP32...{Colors.END}")
    
    # Listar todos los puertos serie
    ports = serial.tools.list_ports.comports()
    esp32_ports = []
    
    # Filtrar puertos que parecen ser ESP32
    for port in ports:
        port_name = port.device
        port_desc = port.description.lower()
        port_hwid = port.hwid.lower() if port.hwid else ""
        
        print(f"{Colors.BLUE}   📱 Analizando: {port_name} - {port.description}{Colors.END}")
        
        # Verificar si es un puerto ESP32 potencial
        if any(keyword in port_desc for keyword in ['usb serial', 'cp210x', 'silicon labs', 'jtag']) or \
           'usbmodem' in port_name or 'usbserial' in port_name:
            esp32_ports.append(port_name)
    
    if not esp32_ports:
        print(f"{Colors.RED}   ❌ No se encontraron puertos ESP32{Colors.END}")
        return {}
    
    print(f"{Colors.YELLOW}🔍 Identificando dispositivos por chip ID...{Colors.END}")
    
    # Identificar cada dispositivo
    devices = {}
    device_counter = 1
    
    for port_name in esp32_ports:
        print(f"   🔍 Identificando dispositivo en {port_name}...")
        device_type = identify_device_type(port_name)
        
        if device_type == 'broker':
            devices["BROKER"] = port_name
            print(f"{Colors.CYAN}   ✅ BROKER (ESP32-C3) detectado: {port_name}{Colors.END}")
        elif device_type == 'fingerprint':
            devices["FINGERPRINT"] = port_name
            print(f"{Colors.GREEN}   ✅ FINGERPRINT (ESP32-WROOM) detectado: {port_name}{Colors.END}")
        elif device_type == 'rfid':
            devices["RFID"] = port_name
            print(f"{Colors.MAGENTA}   ✅ RFID (ESP32-WROOM) detectado: {port_name}{Colors.END}")
        else:
            # Si no se puede identificar, asignar por patrón de puerto
            if 'usbmodem' in port_name and "BROKER" not in devices:
                devices["BROKER"] = port_name
                print(f"{Colors.CYAN}   🔶 BROKER (ESP32-C3): {port_name}{Colors.END}")
            elif 'usbserial' in port_name:
                if "FINGERPRINT" not in devices:
                    devices["FINGERPRINT"] = port_name
                    print(f"{Colors.GREEN}   🔶 FINGERPRINT (ESP32-WROOM): {port_name}{Colors.END}")
                elif "RFID" not in devices:
                    devices["RFID"] = port_name
                    print(f"{Colors.MAGENTA}   🏷️ RFID (ESP32-WROOM): {port_name}{Colors.END}")
    
    print(f"{Colors.YELLOW}⚠️ Asignando puertos por tipo de hardware...{Colors.END}")
    # Mostrar resumen final
    for device_name, port in devices.items():
        if device_name == "BROKER":
            print(f"{Colors.CYAN}   � {device_name} (ESP32-C3): {port}{Colors.END}")
        elif device_name == "FINGERPRINT":
            print(f"{Colors.GREEN}   🔶 {device_name} (ESP32-WROOM): {port}{Colors.END}")
        elif device_name == "RFID":
            print(f"{Colors.MAGENTA}   🏷️ {device_name} (ESP32-WROOM): {port}{Colors.END}")
    
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

def handle_software_reset(target, connections):
    """Maneja resets por comando serie"""
    def reset_device(connection, name):
        if not connection:
            print(f"{Colors.RED}❌ {name} no conectado - no se puede resetear{Colors.END}")
            return False
            
        try:
            print(f"{Colors.YELLOW}🔄 Enviando comando reset a {name}...{Colors.END}")
            connection.write(b'reset\n')
            time.sleep(0.5)  # Dar tiempo para que el comando sea procesado
            print(f"{Colors.GREEN}✅ {name} comando reset enviado{Colors.END}")
            return True
            
        except Exception as e:
            print(f"{Colors.RED}❌ Error enviando reset a {name}: {e}{Colors.END}")
            return False
    
    if target.lower() == 'server':
        reset_device(connections.get('SERVER'), "ESP32-C3 (SERVER)")
    elif target.lower() == 'client':
        reset_device(connections.get('CLIENT'), "ESP32-WROOM (CLIENT)")
    elif target.lower() == 'both':
        print(f"{Colors.MAGENTA}🔄 Reseteando ambos dispositivos...{Colors.END}")
        reset_device(connections.get('SERVER'), "ESP32-C3 (SERVER)")
        time.sleep(1)
        reset_device(connections.get('CLIENT'), "ESP32-WROOM (CLIENT)")
        print(f"{Colors.GREEN}✅ Ambos dispositivos reseteados{Colors.END}")
    else:
        print(f"{Colors.RED}❌ Target inválido. Usa: server, client, o both{Colors.END}")

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

def show_help():
    """Muestra la ayuda completa"""
    print(f"\n{Colors.CYAN}=== COMANDOS CON HISTORIAL NAVEGABLE ==={Colors.END}")
    print(f"{Colors.CYAN}Navegación:{Colors.END}")
    print(f"{Colors.CYAN}  ↑ ↓     - Navegar historial de comandos{Colors.END}")
    print(f"{Colors.CYAN}  ← →     - Mover cursor en línea actual{Colors.END}")
    print(f"{Colors.CYAN}  Enter   - Ejecutar comando{Colors.END}")
    print(f"{Colors.CYAN}  Backspace - Borrar carácter{Colors.END}")
    print()
    print(f"{Colors.CYAN}Comandos del sistema:{Colors.END}")
    print(f"{Colors.CYAN}  help    - Mostrar esta ayuda{Colors.END}")
    print(f"{Colors.CYAN}  history - Mostrar historial de comandos{Colors.END}")
    print(f"{Colors.CYAN}  quit    - Salir del programa{Colors.END}")
    print()
    print(f"{Colors.CYAN}Comandos de dispositivos:{Colors.END}")
    print(f"{Colors.CYAN}  server:comando  - Enviar a ESP32-C3 (Broker){Colors.END}")
    print(f"{Colors.CYAN}  client:comando  - Enviar a ESP32-WROOM (Cliente){Colors.END}")
    print(f"{Colors.CYAN}  reset:dispositivo - Reset físico{Colors.END}")
    print()
    print(f"{Colors.CYAN}Ejemplos útiles:{Colors.END}")
    print(f"{Colors.CYAN}  server:server:list_fingerprints    - Listar huellas{Colors.END}")
    print(f"{Colors.CYAN}  server:server:finger_cut:5         - Eliminar huella ID 5{Colors.END}")
    print(f"{Colors.CYAN}  server:server:enroll:Juan          - Enrolar usuario Juan{Colors.END}")
    print(f"{Colors.CYAN}  server:status                      - Estado del broker{Colors.END}")
    print(f"{Colors.CYAN}  client:scan                        - Escanear huella{Colors.END}")
    print(f"{Colors.CYAN}  reset:both                         - Reset ambos ESP32{Colors.END}")
    print(f"{Colors.CYAN}======================================={Colors.END}")

def show_history(history):
    """Muestra el historial de comandos"""
    print(f"\n{Colors.MAGENTA}=== HISTORIAL DE COMANDOS ==={Colors.END}")
    if not history.history:
        print(f"{Colors.YELLOW}No hay comandos en el historial{Colors.END}")
    else:
        for i, cmd in enumerate(history.history[-20:], 1):  # Últimos 20
            print(f"{Colors.MAGENTA}{i:2d}. {cmd}{Colors.END}")
        
        total, current = history.get_stats()
        print(f"\n{Colors.MAGENTA}Total: {total} comandos, Posición actual: {current}{Colors.END}")
    print(f"{Colors.MAGENTA}============================{Colors.END}")

def process_device_command(command, connections):
    """Procesa un comando de dispositivo"""
    try:
        device, cmd = command.split(':', 1)
        
        if device.lower() == 'server' and 'SERVER' in connections:
            # Para server, enviar el comando completo con prefijo server:
            full_command = f"server:{cmd}"
            connections['SERVER'].write((full_command + '\n').encode())
            print(f"\n{Colors.CYAN}[ENVIADO a SERVER] {full_command}{Colors.END}")
        elif device.lower() == 'client' and 'CLIENT' in connections:
            connections['CLIENT'].write((cmd + '\n').encode())
            print(f"\n{Colors.GREEN}[ENVIADO a CLIENT] {cmd}{Colors.END}")
        elif device.lower() == 'reset':
            handle_software_reset(cmd, connections)
        else:
            print(f"\n{Colors.RED}Dispositivo no encontrado: {device}{Colors.END}")
            
    except ValueError:
        print(f"\n{Colors.RED}Formato inválido. Use: dispositivo:comando{Colors.END}")

def send_commands_simple(detected_ports):
    """Permite enviar comandos usando readline con historial simple"""
    esp32c3_port = detected_ports.get("ESP32-C3")
    esp32_wroom_port = detected_ports.get("ESP32-WROOM")
    
    connections = {}
    command_history = []
    
    try:
        # Configurar readline para historial
        try:
            import readline
            # Configurar historial de readline
            readline.set_history_length(100)
            # Configurar completado básico
            readline.parse_and_bind("tab: complete")
        except ImportError:
            print(f"{Colors.YELLOW}⚠️ readline no disponible - historial limitado{Colors.END}")
        
        if esp32c3_port:
            connections['SERVER'] = serial.Serial(esp32c3_port, 115200, timeout=1)
        if esp32_wroom_port:
            connections['CLIENT'] = serial.Serial(esp32_wroom_port, 115200, timeout=1)
        
        print(f"\n{Colors.YELLOW}{Colors.BOLD}=== COMANDOS CON HISTORIAL SIMPLE ==={Colors.END}")
        print(f"{Colors.YELLOW}📝 Historial con ↑↓ (si readline está disponible){Colors.END}")
        print(f"{Colors.YELLOW}📝 Escribe 'dispositivo:comando' (ej: server:server:status){Colors.END}")
        print(f"{Colors.YELLOW}📝 Dispositivos: server=Broker, client=Cliente{Colors.END}")
        print(f"{Colors.YELLOW}📝 'help' = ayuda, 'history' = ver historial, 'quit' = salir{Colors.END}")
        print(f"{Colors.YELLOW}====================================={Colors.END}\n")
        
        while True:
            try:
                # Usar input() que funciona mejor con readline
                command = input(f"{Colors.YELLOW}> {Colors.END}").strip()
                
                if not command:
                    continue
                    
                # Agregar al historial local
                if command not in command_history[-1:]:  # Evitar duplicados consecutivos
                    command_history.append(command)
                    if len(command_history) > 100:
                        command_history.pop(0)
                
                if command.lower() == 'quit':
                    break
                elif command.lower() == 'help':
                    show_help_simple()
                elif command.lower() == 'history':
                    show_history_simple(command_history)
                elif ':' in command:
                    process_device_command(command, connections)
                else:
                    print(f"{Colors.RED}Formato: dispositivo:comando (ej: server:server:status){Colors.END}")
                    
            except KeyboardInterrupt:
                print(f"\n{Colors.YELLOW}Use 'quit' para salir{Colors.END}")
            except EOFError:
                break
                
    except Exception as e:
        print(f"{Colors.RED}Error con conexiones de comando: {e}{Colors.END}")
    finally:
        for key, conn in connections.items():
            if key != 'detected_ports':
                try:
                    conn.close()
                except:
                    pass

def show_help_simple():
    """Muestra la ayuda simplificada"""
    print(f"\n{Colors.CYAN}=== COMANDOS DISPONIBLES ==={Colors.END}")
    print(f"{Colors.CYAN}Comandos del sistema:{Colors.END}")
    print(f"{Colors.CYAN}  help    - Mostrar esta ayuda{Colors.END}")
    print(f"{Colors.CYAN}  history - Mostrar historial de comandos{Colors.END}")
    print(f"{Colors.CYAN}  quit    - Salir del programa{Colors.END}")
    print()
    print(f"{Colors.CYAN}Comandos de dispositivos:{Colors.END}")
    print(f"{Colors.CYAN}  server:comando  - Enviar a ESP32-C3 (Broker){Colors.END}")
    print(f"{Colors.CYAN}  client:comando  - Enviar a ESP32-WROOM (Cliente){Colors.END}")
    print(f"{Colors.CYAN}  reset:dispositivo - Reset físico{Colors.END}")
    print()
    print(f"{Colors.CYAN}Ejemplos útiles:{Colors.END}")
    print(f"{Colors.CYAN}  server:server:list_fingerprints    - Listar huellas{Colors.END}")
    print(f"{Colors.CYAN}  server:server:finger_cut:5         - Eliminar huella ID 5{Colors.END}")
    print(f"{Colors.CYAN}  server:server:enroll:Juan          - Enrolar usuario Juan{Colors.END}")
    print(f"{Colors.CYAN}  server:status                      - Estado del broker{Colors.END}")
    print(f"{Colors.CYAN}  server:modules                     - Módulos registrados{Colors.END}")
    print(f"{Colors.CYAN}  client:scan                        - Escanear huella{Colors.END}")
    print(f"{Colors.CYAN}  reset:both                         - Reset ambos ESP32{Colors.END}")
    print(f"{Colors.CYAN}========================={Colors.END}")

def show_history_simple(history):
    """Muestra el historial de comandos simple"""
    print(f"\n{Colors.MAGENTA}=== HISTORIAL DE COMANDOS ==={Colors.END}")
    if not history:
        print(f"{Colors.YELLOW}No hay comandos en el historial{Colors.END}")
    else:
        for i, cmd in enumerate(history[-20:], 1):  # Últimos 20
            print(f"{Colors.MAGENTA}{i:2d}. {cmd}{Colors.END}")
        print(f"\n{Colors.MAGENTA}Total: {len(history)} comandos{Colors.END}")
    print(f"{Colors.MAGENTA}============================{Colors.END}")

# Función principal que usa el sistema simple
send_commands = send_commands_simple

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