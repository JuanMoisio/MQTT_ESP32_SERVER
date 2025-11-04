#!/usr/bin/env python3
"""
Single terminal ESP32 monitor (legacy copy)
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

MAX_HISTORY = 100

try:
    from .utils import Colors
except Exception:
    class Colors:
        RED = '\033[91m'
        GREEN = '\033[92m'
        YELLOW = '\033[93m'
        BLUE = '\033[94m'
        MAGENTA = '\033[95m'
        CYAN = '\033[96m'
        BOLD = '\033[1m'
        END = '\033[0m'

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 60)
    print("    📺 ESP32 SINGLE TERMINAL MONITOR - LEGACY")
    print("=" * 60)
    print(f"{Colors.END}")
    print("This is a legacy copy. Use monitoresPy/monitor_cli.py for the active monitor.")

if __name__ == '__main__':
    main()
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

# Configuracion
MAX_HISTORY = 100  # Máximo de comandos en historial

# Shared utilities
try:
    from .utils import Colors
except Exception:
    from utils import Colors

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
        if char == '\x1f' or char == '\x08':  # Backspace
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

# ... rest preserved in legacy copy
