#!/usr/bin/env python3
"""
Quick upload (legacy copy)
"""

import serial.tools.list_ports
import sys
try:
    from .utils import Colors
    from .uploader import detect_esp32_devices, upload_to_device
except Exception:
    from utils import Colors
    from uploader import detect_esp32_devices, upload_to_device

def main():
    print("This is a legacy copy of quick_upload. Use monitoresPy/uploader.py for the canonical uploader.")

if __name__ == '__main__':
    main()
