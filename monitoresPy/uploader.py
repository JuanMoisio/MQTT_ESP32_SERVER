#!/usr/bin/env python3
"""
Centralized detection and uploader utilities.

Functions:
- detect_esp32_devices(): returns mapping of roles to ports and optional chip_id
- upload_to_device(project_path, device_port, pio_path): performs platformio upload
"""
import serial.tools.list_ports
import subprocess
import json
import os
import re
import time
from pathlib import Path

# Allow importing as package (relative) or as script/module (absolute)
try:
    from .config import PROJECTS, PIO_PATH, DEVICE_CONFIG_FILE
    from .utils import Colors
except Exception:
    from config import PROJECTS, PIO_PATH, DEVICE_CONFIG_FILE
    from utils import Colors


def get_device_chip_id(port, timeout=8):
    """Try to retrieve an identifier (MAC/chip) using esptool or serial fallback."""
    # Try esptool if available
    try:
        cmd = [
            str(Path(PIO_PATH).parent / 'python'), '-m', 'esptool', '--port', port, '--baud', '115200', 'chip_id'
        ]
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        if result.returncode == 0:
            m = re.search(r'MAC: ([0-9A-Fa-f:]{17})', result.stdout)
            if m:
                return m.group(1).upper()
    except Exception:
        pass

    # Fallback: open serial and try to read a MAC-like string
    try:
        import serial
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(0.5)
        ser.write(b'\x03')
        ser.write(b'WiFi.macAddress()\n')
        time.sleep(0.5)
        out = ser.read_all().decode('utf-8', errors='ignore')
        ser.close()
        m = re.search(r'([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})', out)
        if m:
            return m.group(0).upper()
    except Exception:
        pass

    return None


def detect_esp32_devices(known_devices=None):
    """Detect connected ESP32 devices and attempt to classify them.

    Returns dict: { 'BROKER': {'port': '/dev/..', 'chip_id': '..'}, ... }
    """
    devices = {
        'BROKER': {'port': None, 'type': 'ESP32-C3', 'chip_id': None},
        'FINGERPRINT': {'port': None, 'type': 'ESP32-WROOM', 'chip_id': None},
        'RFID': {'port': None, 'type': 'ESP32-WROOM', 'chip_id': None}
    }

    ports = serial.tools.list_ports.comports()
    detected = []

    for port in ports:
        name = port.device
        desc = (port.description or '').lower()
        if any(k in desc for k in ['usb serial', 'usb-serial', 'silicon labs', 'cp210', 'ch340']) or 'usbserial' in name.lower():
            detected.append((name, desc))
        elif 'usbmodem' in name.lower() or 'jtag' in desc or 'debug' in desc:
            detected.append((name, desc))

    # Try known_devices mapping by chip id first
    if known_devices:
        for name, desc in detected:
            chip = get_device_chip_id(name)
            if chip and chip in known_devices:
                role = known_devices[chip]
                if role in devices and not devices[role]['port']:
                    devices[role]['port'] = name
                    devices[role]['chip_id'] = chip

    # Assign remaining by heuristics
    unassigned = [p for p in detected if not any(d['port'] == p[0] for d in devices.values())]
    for name, desc in unassigned:
        lname = name.lower()
        if ('usbmodem' in lname or 'jtag' in desc or 'debug' in desc) and not devices['BROKER']['port']:
            devices['BROKER']['port'] = name
        elif ('usb serial' in desc or 'usbserial' in lname or 'cp210' in desc) and not devices['FINGERPRINT']['port']:
            devices['FINGERPRINT']['port'] = name
        elif ('usb serial' in desc or 'usbserial' in lname) and not devices['RFID']['port']:
            devices['RFID']['port'] = name
        else:
            # fallback assignments
            if not devices['FINGERPRINT']['port']:
                devices['FINGERPRINT']['port'] = name
            elif not devices['BROKER']['port']:
                devices['BROKER']['port'] = name

    return devices


def upload_to_device(project_path, device_port, device_name, pio_path=PIO_PATH, timeout=120):
    """Upload project at project_path to device_port using platformio.

    Returns True on success, False otherwise. project_path may be absolute or relative.
    """
    print(f"\n{Colors.YELLOW} Subiendo c\u00f3digo a {device_name}...{Colors.END}")
    print(f"{Colors.BLUE}   Proyecto: {project_path}{Colors.END}")
    print(f"{Colors.BLUE}   Puerto: {device_port}{Colors.END}")

    original_dir = os.getcwd()
    try:
        os.chdir(project_path)
        cmd = [pio_path, 'run', '--target', 'upload', '--upload-port', device_port]
        print(f"{Colors.CYAN}   Ejecutando: {' '.join(cmd)}{Colors.END}")
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
        os.chdir(original_dir)
        if result.returncode == 0:
            print(f"{Colors.GREEN}   \u2705 {device_name} - Upload exitoso!{Colors.END}")
            return True
        else:
            print(f"{Colors.RED}   \u274c {device_name} - Error en upload:\n{result.stderr}{Colors.END}")
            return False
    except subprocess.TimeoutExpired:
        os.chdir(original_dir)
        print(f"{Colors.RED}   \u274c {device_name} - Timeout (> {timeout}s){Colors.END}")
        return False
    except Exception as e:
        os.chdir(original_dir)
        print(f"{Colors.RED}   \u274c {device_name} - Error: {e}{Colors.END}")
        return False
