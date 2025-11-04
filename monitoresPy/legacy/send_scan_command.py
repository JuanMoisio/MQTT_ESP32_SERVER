#!/usr/bin/env python3
"""
Script to send scan_fingerprint command to the running monitor via process communication
"""

import subprocess
import time
import signal
import psutil

def send_scan_command_to_monitor():
    print("🔍 Looking for running monitor process...")
    
    # Find the running monitor process
    monitor_process = None
    for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
        try:
            if proc.info['cmdline'] and len(proc.info['cmdline']) > 1:
                cmdline = ' '.join(proc.info['cmdline'])
                if 'single_terminal_monitor.py' in cmdline:
                    monitor_process = proc
                    print(f"✅ Found monitor process: PID {proc.info['pid']}")
                    break
        except (psutil.NoSuchProcess, psutil.AccessDenied):
            continue
    
    if not monitor_process:
        print("❌ Monitor process not found. Make sure single_terminal_monitor.py is running.")
        return
    
    # Since the monitor is waiting for stdin, we need to use a different approach
    # Let's create a new process to directly send the command to the ESP32-C3
    print("📡 Sending command directly to ESP32-C3...")
    
    import serial
    
    try:
        # Connect to ESP32-C3 server
        server_port = "/dev/cu.usbmodem11201"
        print(f"🔌 Connecting to ESP32-C3 server at {server_port}")
        
        ser = serial.Serial(server_port, 115200, timeout=2)
        time.sleep(0.5)  # Brief pause
        
        print("✅ Connected to ESP32-C3")
        print("📤 Sending 'server:scan_fingerprint' command...")
        
        # Send the command in the format the server expects
        command = "server:scan_fingerprint\n"
        ser.write(command.encode())
        
        print("📨 Command sent!")
        print("⏳ Waiting for debug messages (check monitor for full output)...")
        
        # Give time for processing
        time.sleep(3)
        
        ser.close()
        print("🔌 Connection closed")
        print("👀 Check the monitor terminal for debug output and client response!")
        
    except Exception as e:
        print(f"❌ Error sending command: {e}")

if __name__ == "__main__":
    send_scan_command_to_monitor()
