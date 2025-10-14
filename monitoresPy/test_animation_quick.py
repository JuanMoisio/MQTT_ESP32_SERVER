#!/usr/bin/env python3
"""
Test rápido de animación - envía comando al monitor
"""
import time
import subprocess
import sys

def send_command_to_monitor():
    print("🧪 Enviando comando de prueba de animación...")
    print("Comando: server:scan_fingerprint")
    print("")
    print("👀 Observa la pantalla OLED del ESP32-WROOM para ver la animación!")
    print("   - La barra láser debería moverse más rápido (60ms por frame)")  
    print("   - No debería dejar píxeles encendidos")
    print("   - La animación debería ser más suave")
    print("")
    
    # Simular entrada al monitor
    sys.stdout.write("server:scan_fingerprint\n")
    sys.stdout.flush()

if __name__ == "__main__":
    send_command_to_monitor()