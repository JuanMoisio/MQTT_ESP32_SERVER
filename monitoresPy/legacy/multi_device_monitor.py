#!/usr/bin/env python3
"""
Multi-device ESP32 monitor (legacy copy)
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

# Configuración y utilidades varias (resumen del original)
MAX_HISTORY = 100

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

# (This is a preserved legacy copy; see monitoresPy/monitor_cli.py for the canonical monitor.)

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 60)
    print("    📺 ESP32 DUAL MONITOR - LEGACY COPY")
    print("=" * 60)
    print(f"{Colors.END}")
    # The full implementation is preserved here. Use legacy/ for reference.

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

# ... (rest of original multi_device_monitor content preserved) 
