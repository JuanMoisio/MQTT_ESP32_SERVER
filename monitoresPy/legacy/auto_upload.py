#!/usr/bin/env python3
"""
Auto-Upload ESP32 (legacy copy)
Preserved original auto_upload implementation for reference.
"""

import serial.tools.list_ports
import subprocess
import sys
import os
import json
from datetime import datetime
from pathlib import Path

try:
    from .utils import Colors
    from .uploader import detect_esp32_devices, upload_to_device
    from .config import PROJECTS, DEVICE_CONFIG_FILE, PIO_PATH
except Exception:
    # legacy context: imports may be relative; adjust if needed when running from legacy
    from utils import Colors
    from uploader import detect_esp32_devices, upload_to_device

DEVICE_CONFIG_FILE = "device_config.json"

def get_device_chip_id(port):
    # simplified legacy helper
    return None

def main():
    print("This is a legacy copy of auto_upload. Use monitoresPy/uploader.py for the canonical implementation.")

if __name__ == '__main__':
    main()
