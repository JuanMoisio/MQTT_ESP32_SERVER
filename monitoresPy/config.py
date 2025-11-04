#!/usr/bin/env python3
"""
Configuration for monitor utilities.
Contains defaults for PlatformIO path and project mappings.
"""
import os
from pathlib import Path

# Path to platformio executable (can be overridden with env var PIO_PATH)
PIO_PATH = os.environ.get('PIO_PATH') or str(Path.home() / '.platformio' / 'penv' / 'bin' / 'platformio')

# Default project mapping: device role -> project absolute path
PROJECTS = {
    'BROKER': os.environ.get('BROKER_PROJECT') or str(Path.home() / 'Documents' / 'PlatformIO' / 'Projects' / 'BorkerMQTT'),
    'FINGERPRINT': os.environ.get('FINGERPRINT_PROJECT') or str(Path.home() / 'Documents' / 'PlatformIO' / 'Projects' / 'HuellaDactilar'),
    'RFID': os.environ.get('RFID_PROJECT') or str(Path.home() / 'Documents' / 'PlatformIO' / 'Projects' / 'RFIDReader'),
}

# Device config file name (used to persist learned chip IDs)
DEVICE_CONFIG_FILE = 'device_config.json'
