#!/usr/bin/env python3
"""
Dual ESP32 Serial Monitor with Command History
Monitorea simultáneamente ESP32-C3 (Broker MQTT) y ESP32 WROOM (Cliente Fingerprint)
Incluye historial de comandos navegable con flechas arriba/abajo
"""

import serial
import threading
import time
import sys
import os
import termios
import tty
import select
from datetime import datetime

# Configuración
BAUDRATE = 115200
TIMEOUT = 1
MAX_HISTORY = 100  # Máximo de comandos en historial

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
    """Obtiene timestamp formateado"""
    return datetime.now().strftime("%H:%M:%S")

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
        print(f"\n{color}{Colors.BOLD}[{device_name}] Conectado a {port}{Colors.END}")
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = get_timestamp()
                        print(f"\n{color}[{timestamp}] [{device_name}]{Colors.END} {line}")
                        print(f"{Colors.YELLOW}> {Colors.END}", end='', flush=True)
                except UnicodeDecodeError:
                    continue
            else:
                time.sleep(0.01)
                
    except KeyboardInterrupt:
        pass
    except Exception as e:
        print(f"\n{Colors.RED}[{device_name}] Error: {e}{Colors.END}")
        print(f"{Colors.YELLOW}> {Colors.END}", end='', flush=True)
    finally:
        try:
            ser.close()
        except:
            pass

def get_char():
    """Obtiene un carácter sin presionar Enter"""
    fd = sys.stdin.fileno()
    old_settings = termios.tcgetattr(fd)
    try:
        tty.setraw(sys.stdin.fileno())
        
        # Verificar si hay entrada disponible
        if select.select([sys.stdin], [], [], 0) == ([sys.stdin], [], []):
            ch = sys.stdin.read(1)
            
            # Manejar secuencias de escape (flechas)
            if ch == '\x1b':
                ch2 = sys.stdin.read(1)
                if ch2 == '[':
                    ch3 = sys.stdin.read(1)
                    return f'\x1b[{ch3}'
            return ch
        return None
    finally:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)

def send_commands_with_history(ports_info):
    """Permite enviar comandos con historial navegable"""
    print(f"\n{Colors.YELLOW}{Colors.BOLD}=== COMANDOS CON HISTORIAL ==={Colors.END}")
    print(f"{Colors.YELLOW}📝 Usa flechas ↑↓ para navegar el historial{Colors.END}")
    print(f"{Colors.YELLOW}📝 Escribe 'dispositivo:comando' (ej: 1:server:status){Colors.END}")
    print(f"{Colors.YELLOW}📝 Dispositivos: 1=Broker, 2=Cliente{Colors.END}")
    print(f"{Colors.YELLOW}📝 'help' = ayuda, 'history' = ver historial, 'quit' = salir{Colors.END}")
    print(f"{Colors.YELLOW}================================{Colors.END}\n")
    
    # Abrir conexiones para envío de comandos
    connections = {}
    
    for port, device_type, device_name in ports_info:
        try:
            connections[device_name] = serial.Serial(port, BAUDRATE, timeout=1)
        except Exception as e:
            print(f"{Colors.RED}Error abriendo conexión de comando para {device_name}: {e}{Colors.END}")
    
    input_handler = InputHandler()
    
    print(f"{Colors.YELLOW}> {Colors.END}", end='', flush=True)
    
    try:
        while True:
            try:
                char = get_char()
                if char is None:
                    time.sleep(0.01)
                    continue
                
                # Manejar teclas especiales
                if char.startswith('\x1b'):
                    if input_handler.handle_special_key(char):
                        continue
                
                # Manejar caracteres normales
                result = input_handler.add_char(char)
                
                if result == 'enter':
                    command = input_handler.get_command()
                    print()  # Nueva línea después del comando
                    
                    if command.strip().lower() == 'quit':
                        break
                    elif command.strip().lower() == 'help':
                        show_help()
                    elif command.strip().lower() == 'history':
                        show_history(input_handler.history)
                    elif ':' in command:
                        process_device_command(command, connections)
                    else:
                        print(f"{Colors.RED}Formato: dispositivo:comando (ej: 1:server:status){Colors.END}")
                    
                    print(f"{Colors.YELLOW}> {Colors.END}", end='', flush=True)
                    
            except KeyboardInterrupt:
                print(f"\n{Colors.YELLOW}Use 'quit' para salir{Colors.END}")
                print(f"{Colors.YELLOW}> {Colors.END}", end='', flush=True)
                
    finally:
        for conn in connections.values():
            try:
                conn.close()
            except:
                pass

def show_help():
    """Muestra la ayuda"""
    print(f"\n{Colors.CYAN}=== COMANDOS DISPONIBLES ==={Colors.END}")
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
    print(f"{Colors.CYAN}  1:comando  - Enviar a ESP32-C3 (Broker){Colors.END}")
    print(f"{Colors.CYAN}  2:comando  - Enviar a ESP32-WROOM (Cliente){Colors.END}")
    print()
    print(f"{Colors.CYAN}Ejemplos útiles:{Colors.END}")
    print(f"{Colors.CYAN}  1:server:list_fingerprints    - Listar huellas{Colors.END}")
    print(f"{Colors.CYAN}  1:server:finger_cut:5         - Eliminar huella ID 5{Colors.END}")
    print(f"{Colors.CYAN}  1:server:enroll:Juan          - Enrolar usuario Juan{Colors.END}")
    print(f"{Colors.CYAN}  1:status                      - Estado del broker{Colors.END}")
    print(f"{Colors.CYAN}  2:scan                        - Escanear huella{Colors.END}")
    print(f"{Colors.CYAN}========================={Colors.END}")

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
        device_num, cmd = command.split(':', 1)
        
        if device_num == '1' and 'ESP32-C3' in connections:
            connections['ESP32-C3'].write((cmd + '\n').encode())
            print(f"{Colors.CYAN}[ENVIADO a SERVER] {cmd}{Colors.END}")
        elif device_num == '2' and 'ESP32-WROOM' in connections:
            connections['ESP32-WROOM'].write((cmd + '\n').encode())
            print(f"{Colors.GREEN}[ENVIADO a CLIENT] {cmd}{Colors.END}")
        else:
            print(f"{Colors.RED}Dispositivo no encontrado: {device_num}{Colors.END}")
            
    except ValueError:
        print(f"{Colors.RED}Formato inválido. Use: dispositivo:comando{Colors.END}")

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 70)
    print("    DUAL ESP32 SERIAL MONITOR WITH COMMAND HISTORY")
    print("    ESP32-C3 (Broker) + ESP32-WROOM (Cliente)")
    print("    Navegación: ↑↓ para historial, ←→ para cursor")
    print("=" * 70)
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
    
    # Dar tiempo para que se establezcan las conexiones
    time.sleep(2)
    
    # Manejar comandos con historial
    try:
        send_commands_with_history(ports_info)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Cerrando monitor...{Colors.END}")

if __name__ == "__main__":
    try:
        import serial
    except ImportError:
        print(f"{Colors.RED}❌ Instalar pyserial: pip install pyserial{Colors.END}")
        sys.exit(1)
    
    main()