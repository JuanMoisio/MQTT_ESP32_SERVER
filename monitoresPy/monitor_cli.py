#!/usr/bin/env python3
"""
monitor_cli.py

Unified serial monitor for ESP32 devices.
Combines detection, cleanup, dual monitoring, and a command-line with history.

Features:
- Detect devices using uploader.detect_esp32_devices()
- Cleanup serial connections using utils.cleanup_serial_connections()
- Start a reader thread per detected device
- Interactive command input with history and arrow navigation
"""

import threading
import time
import sys
import select
import termios
import tty
from datetime import datetime

try:
    # package import
    from .uploader import detect_esp32_devices
    from .utils import Colors, cleanup_serial_connections
except Exception:
    # fallback when run as script
    from uploader import detect_esp32_devices
    from utils import Colors, cleanup_serial_connections

import serial
import serial.tools.list_ports

BAUDRATE = 115200
TIMEOUT = 1
MAX_HISTORY = 200


# Print synchronization to avoid overlapping prompt and thread output
print_lock = threading.Lock()
_input_handler = None

def register_input_handler(h):
    global _input_handler
    _input_handler = h

def safe_print(*args, **kwargs):
    """Print while keeping the interactive prompt intact."""
    end = kwargs.pop('end', '\n')
    with print_lock:
        # clear current line using ANSI escape to avoid leftover chars
        try:
            sys.stdout.write('\x1b[2K\r')
        except Exception:
            pass
        # actual print
        print(*args, end=end, **kwargs)
        # redraw prompt if input handler exists
        if _input_handler is not None:
            try:
                sys.stdout.write(f"{Colors.YELLOW}> {_input_handler.current}{Colors.END}")
                sys.stdout.flush()
            except Exception:
                pass


class CommandHistory:
    def __init__(self, max_size=MAX_HISTORY):
        self.history = []
        self.max_size = max_size
        self.current_index = 0
        self.temp_command = ""

    def add(self, cmd):
        if not cmd.strip():
            return
        if not self.history or self.history[-1] != cmd:
            self.history.append(cmd)
            if len(self.history) > self.max_size:
                self.history.pop(0)
        self.reset()

    def reset(self):
        self.current_index = len(self.history)
        self.temp_command = ""

    def previous(self, current_command=""):
        if self.current_index == len(self.history):
            self.temp_command = current_command
        if self.current_index > 0:
            self.current_index -= 1
            return self.history[self.current_index]
        return current_command

    def next(self):
        if self.current_index < len(self.history) - 1:
            self.current_index += 1
            return self.history[self.current_index]
        elif self.current_index == len(self.history) - 1:
            self.current_index += 1
            return self.temp_command
        return ""


class InputHandler:
    def __init__(self):
        self.history = CommandHistory()
        self.current = ""
        self.cursor = 0

    def set_command(self, cmd):
        with print_lock:
            # clear current line and set using ANSI clear
            try:
                sys.stdout.write('\x1b[2K\r')
            except Exception:
                pass
            self.current = cmd
            self.cursor = len(cmd)
            sys.stdout.write(f"{Colors.YELLOW}> {cmd}{Colors.END}")
            sys.stdout.flush()

    def add_char(self, ch):
        if ch == '\x7f' or ch == '\x08':  # backspace
            if self.cursor > 0:
                self.current = self.current[:self.cursor-1] + self.current[self.cursor:]
                self.cursor -= 1
                self.refresh()
        elif ch in ('\n', '\r'):
            return 'enter'
        elif ch.startswith('\x1b'):
            # handled elsewhere
            return 'escape'
        elif ch.isprintable():
            self.current = self.current[:self.cursor] + ch + self.current[self.cursor:]
            self.cursor += 1
            self.refresh()
        return 'continue'

    def refresh(self):
        with print_lock:
            try:
                sys.stdout.write('\x1b[2K\r')
            except Exception:
                pass
            sys.stdout.write(f"{Colors.YELLOW}> {self.current}{Colors.END}")
            sys.stdout.flush()
            # reposition cursor not strictly necessary

    def get_and_store(self):
        cmd = self.current
        if cmd.strip():
            self.history.add(cmd)
        self.current = ""
        self.cursor = 0
        return cmd


def get_char_nonblocking(timeout=0.01):
    # Assumes terminal mode (cbreak/raw) is set once by caller.
    if select.select([sys.stdin], [], [], timeout) == ([sys.stdin], [], []):
        ch = sys.stdin.read(1)
        if ch == '\x1b':
            # try to read escape sequence (arrow keys)
            if select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                ch2 = sys.stdin.read(1)
                if ch2 == '[' and select.select([sys.stdin], [], [], 0.01) == ([sys.stdin], [], []):
                    ch3 = sys.stdin.read(1)
                    return f"\x1b[{ch3}"
                return ch + ch2
            return ch
        return ch
    return None


def timestamp():
    return datetime.now().strftime('%H:%M:%S')


def monitor_reader(port, name, stop_event):
    color = Colors.CYAN if 'BROKER' in name.upper() or 'C3' in name.upper() else Colors.GREEN
    try:
        ser = serial.Serial(port, BAUDRATE, timeout=TIMEOUT)
    except Exception as e:
        print(f"{Colors.RED}❌ Error abriendo {port}: {e}{Colors.END}")
        return

    safe_print(f"{color}{Colors.BOLD}[{name}] Conectado en {port}{Colors.END}")
    try:
        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').rstrip()
                    if line:
                        safe_print(f"{color}[{timestamp()}] [{name}]{Colors.END} {line}")
                except Exception:
                    continue
            else:
                time.sleep(0.01)
    except KeyboardInterrupt:
        pass
    finally:
        try:
            ser.close()
        except Exception:
            pass


def send_command_to_ports(connections, cmd, dry_run=False):
    # connections: dict name->serial.Serial
    for name, conn in connections.items():
        try:
            if dry_run:
                safe_print(f"{Colors.YELLOW}[DRY-RUN enviar a {name}]{Colors.END} {cmd}")
            else:
                conn.write((cmd + '\n').encode())
                conn.flush()
                safe_print(f"{Colors.YELLOW}[ENVIADO a {name}]{Colors.END} {cmd}")
        except Exception as e:
            safe_print(f"{Colors.RED}Error enviando a {name}: {e}{Colors.END}")


def main(dry_run=False):
    safe_print(f"{Colors.BOLD}{Colors.MAGENTA}")
    safe_print("=" * 60)
    safe_print("    MONITOR CLI - ESP32 DUAL MONITOR")
    safe_print("=" * 60)
    safe_print(f"{Colors.END}")
    if dry_run:
        safe_print(f"{Colors.YELLOW}MODO DRY-RUN: no se abrirán puertos serie ni se enviarán datos{Colors.END}")

    # Detect devices
    devices = detect_esp32_devices()
    if not devices:
        print(f"{Colors.RED}No se detectaron dispositivos.{Colors.END}")
        return 1

    # Show detected
    for role, info in devices.items():
        if info.get('port'):
            safe_print(f"{Colors.CYAN}   {role}: {info.get('port')}{Colors.END}")

    # Cleanup prev connections
    if not dry_run:
        safe_print(" Limpiando conexiones serie previas...")
        safe_print("    Cerrando sesiones de screen y procesos que bloquean puertos...")
        cleanup_serial_connections({k: v.get('port') for k, v in devices.items()})
        safe_print(" Puertos serie limpiados")
    else:
        safe_print(" Modo dry-run: se omite limpieza de puertos serie")

    # Prepare threads and serial connections
    stop_event = threading.Event()
    threads = []
    connections = {}

    # Start reader threads for available ports
    for role, info in devices.items():
        port = info.get('port')
        if not port:
            continue
        name = f"{role}"
        if not dry_run:
            t = threading.Thread(target=monitor_reader, args=(port, name, stop_event), daemon=True)
            t.start()
            threads.append(t)

            # open serial for sending commands
            try:
                conn = serial.Serial(port, BAUDRATE, timeout=1)
                connections[name] = conn
            except Exception:
                pass
        else:
            # dry-run: record available ports but don't open them
            connections[name] = None

    # Interactive command input
    input_handler = InputHandler()
    register_input_handler(input_handler)
    safe_print(f"{Colors.YELLOW}Escribe 'help' para ver comandos, 'quit' para salir{Colors.END}")
    # initial prompt drawn by safe_print registration

    # put terminal into cbreak mode once to support single-char reads
    fd = sys.stdin.fileno()
    old_term = termios.tcgetattr(fd)
    tty.setcbreak(fd)
    try:
        while True:
            ch = get_char_nonblocking(0.02)
            if ch is None:
                time.sleep(0.01)
                continue

            # Handle arrow keys
            if ch.startswith('\x1b'):
                if ch == '\x1b[A':  # up
                    prev = input_handler.history.previous(input_handler.current)
                    input_handler.set_command(prev)
                elif ch == '\x1b[B':  # down
                    nxt = input_handler.history.next()
                    input_handler.set_command(nxt)
                elif ch == '\x1b[C':
                    # right - ignore
                    pass
                elif ch == '\x1b[D':
                    # left - ignore
                    pass
                continue

            res = input_handler.add_char(ch)
            if res == 'enter':
                cmd = input_handler.get_and_store()
                print()  # newline after command
                if not cmd:
                    sys.stdout.write(f"{Colors.YELLOW}> {Colors.END}")
                    sys.stdout.flush()
                    continue
                lower = cmd.strip().lower()
                if lower in ('quit', 'exit'):
                    break
                if lower == 'help':
                    print("Comandos: help, quit, <role>:<command> (ej: BROKER:status) o simplemente 'scan')")
                    sys.stdout.write(f"{Colors.YELLOW}> {Colors.END}")
                    sys.stdout.flush()
                    continue

                # If command contains target prefix ROLE:cmd
                if ':' in cmd:
                    target, payload = cmd.split(':', 1)
                    target = target.strip().upper()
                    payload = payload.strip()
                    if target in connections:
                        send_command_to_ports({target: connections[target]}, payload, dry_run=dry_run)
                    else:
                        # try broadcast
                        send_command_to_ports(connections, cmd, dry_run=dry_run)
                else:
                    # broadcast to all
                    send_command_to_ports(connections, cmd, dry_run=dry_run)

                sys.stdout.write(f"{Colors.YELLOW}> {Colors.END}")
                sys.stdout.flush()

    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        # restore terminal settings
        try:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_term)
        except Exception:
            pass
        safe_print(f"\n{Colors.YELLOW}Cerrando conexiones...{Colors.END}")
        for conn in connections.values():
            try:
                conn.close()
            except Exception:
                pass
        time.sleep(0.2)

    return 0


if __name__ == '__main__':
    try:
        import argparse
        p = argparse.ArgumentParser(prog='monitor_cli')
        p.add_argument('--dry-run', action='store_true', help='Detectar dispositivos pero no abrir puertos ni enviar datos')
        args = p.parse_args()
        sys.exit(main(dry_run=args.dry_run))
    except Exception as e:
        print(f"{Colors.RED}Error en monitor_cli: {e}{Colors.END}")
        sys.exit(1)
