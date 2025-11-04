#!/usr/bin/env python3
"""
Shared utilities for monitor scripts
Contains common helpers and constants (starts small: Colors class)
"""
import subprocess
import os
import time
import serial.tools.list_ports

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

def cleanup_serial_connections(detected_ports=None):
    """Try to free serial ports by terminating processes that use them.

    This is a pragmatic cleanup helper used by several monitor scripts.
    It attempts a gentle SIGTERM and escalates to SIGKILL if needed.
    """
    import signal

    print(f"{Colors.YELLOW}[1m Limpiando conexiones serie previas...{Colors.END}")

    if detected_ports:
        ports = [port for port in detected_ports.values() if port]
    else:
        all_ports = serial.tools.list_ports.comports()
        ports = [port.device for port in all_ports if 'usb' in (port.device or '').lower()]

    try:
        print(f"{Colors.BLUE}    Cerrando sesiones de screen y procesos que bloquean puertos...{Colors.END}")
        # Try to kill screen sessions that might be attached to devices
        subprocess.run(['pkill', '-f', 'screen'], capture_output=True)

        for port in ports:
            try:
                result = subprocess.run(['lsof', port], capture_output=True, text=True)
                if result.returncode == 0 and result.stdout:
                    lines = result.stdout.strip().split('\n')[1:]
                    for line in lines:
                        if not line.strip():
                            continue
                        parts = line.split()
                        if len(parts) >= 2:
                            pid = parts[1]
                            print(f"{Colors.CYAN}     Liberando PID {pid} de {port}...{Colors.END}")
                            try:
                                os.kill(int(pid), signal.SIGTERM)
                                time.sleep(0.5)
                                try:
                                    os.kill(int(pid), 0)
                                    os.kill(int(pid), signal.SIGKILL)
                                    print(f"{Colors.RED}     Forzado cierre de PID {pid}{Colors.END}")
                                except ProcessLookupError:
                                    pass
                            except (ValueError, ProcessLookupError, PermissionError):
                                pass
            except subprocess.SubprocessError:
                continue

        time.sleep(1)
        print(f"{Colors.GREEN}[1m Puertos serie limpiados{Colors.END}")
    except Exception as e:
        print(f"{Colors.YELLOW}[1m Advertencia limpiando puertos: {e}{Colors.END}")
