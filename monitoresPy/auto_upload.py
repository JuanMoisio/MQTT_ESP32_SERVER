#!/usr/bin/env python3
"""
🚀 Auto-Upload ESP32
Detecta automáticamente los ESP32 y sube el código correcto a cada uno
"""

import serial.tools.list_ports
import subprocess
import sys
import os
import json
from datetime import datetime

# Colores para output
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    BOLD = '\033[1m'
    END = '\033[0m'

DEVICE_CONFIG_FILE = "device_config.json"

def load_known_devices():
    """Carga la configuración de dispositivos conocidos"""
    try:
        if os.path.exists(DEVICE_CONFIG_FILE):
            with open(DEVICE_CONFIG_FILE, 'r') as f:
                return json.load(f)
    except Exception as e:
        print(f"{Colors.YELLOW}⚠️ Error cargando configuración: {e}{Colors.END}")
    
    return {}

def save_known_devices(known_devices):
    """Guarda la configuración de dispositivos conocidos"""
    try:
        with open(DEVICE_CONFIG_FILE, 'w') as f:
            json.dump(known_devices, f, indent=2)
    except Exception as e:
        print(f"{Colors.RED}❌ Error guardando configuración: {e}{Colors.END}")

def learn_device_assignment(devices):
    """Permite al usuario asignar manualmente los dispositivos detectados"""
    unassigned_ports = []
    
    # Obtener todos los puertos detectados, sin importar si tienen chip_id o no
    for device_name, device_info in devices.items():
        if device_info["port"]:
            # Intentar obtener chip_id si no lo tenemos
            chip_id = device_info["chip_id"]
            if not chip_id:
                print(f"{Colors.CYAN}🔍 Obteniendo chip ID de {device_info['port']}...{Colors.END}")
                chip_id = get_device_chip_id(device_info["port"])
                device_info["chip_id"] = chip_id  # Actualizar en el dispositivo original
            
            unassigned_ports.append({
                "port": device_info["port"],
                "chip_id": chip_id or f"UNKNOWN_{device_info['port'].split('/')[-1]}",  # Usar puerto como fallback
                "assigned_to": None,
                "current_assignment": device_name  # Asignación actual automática
            })
    
    if len(unassigned_ports) < 2:
        print(f"{Colors.YELLOW}⚠️ Se necesitan al menos 2 dispositivos para aprender asignaciones{Colors.END}")
        return {}
    
    print(f"\n{Colors.CYAN}🎓 MODO APRENDIZAJE - Asigna tus dispositivos manualmente{Colors.END}")
    print(f"{Colors.YELLOW}Esto se guardará para futuras detecciones automáticas{Colors.END}")
    
    learned_assignments = {}
    available_roles = ["BROKER", "FINGERPRINT", "RFID"]
    
    for i, port_info in enumerate(unassigned_ports):
        print(f"\n{Colors.BLUE}━━━ Dispositivo {i+1} ━━━{Colors.END}")
        print(f"  🔌 Puerto: {Colors.BOLD}{port_info['port']}{Colors.END}")
        print(f"  🆔 Chip ID: {Colors.BOLD}{port_info['chip_id']}{Colors.END}")
        print(f"  🤖 Asignación automática actual: {Colors.YELLOW}{port_info['current_assignment']}{Colors.END}")
        
        print(f"\n{Colors.CYAN}¿Qué tipo de dispositivo es realmente?{Colors.END}")
        
        # Mostrar opción para confirmar asignación automática
        current_role = port_info['current_assignment']
        if current_role in available_roles:
            print(f"  {Colors.GREEN}0. ✅ CONFIRMAR asignación automática ({current_role}){Colors.END}")
        
        # Mostrar otras opciones disponibles
        for j, role in enumerate(available_roles):
            color = Colors.GREEN if role == current_role else Colors.CYAN
            mark = " ← ACTUAL" if role == current_role else ""
            print(f"  {color}{j+1}. {role}{mark}{Colors.END}")
            
        print(f"  {Colors.YELLOW}{len(available_roles)+1}. Saltar este dispositivo{Colors.END}")
        
        while True:
            try:
                prompt = f"{Colors.CYAN}Elige (0 para confirmar, 1-{len(available_roles)+1}): {Colors.END}"
                choice = int(input(prompt))
                
                if choice == 0 and current_role in available_roles:
                    # Confirmar asignación automática
                    selected_role = current_role
                    learned_assignments[port_info['chip_id']] = selected_role
                    available_roles.remove(selected_role)
                    print(f"{Colors.GREEN}✅ {port_info['chip_id']} confirmado como {selected_role}{Colors.END}")
                    break
                    
                elif 1 <= choice <= len(available_roles):
                    selected_role = available_roles[choice-1]
                    learned_assignments[port_info['chip_id']] = selected_role
                    available_roles.remove(selected_role)
                    print(f"{Colors.GREEN}✅ {port_info['chip_id']} asignado a {selected_role}{Colors.END}")
                    break
                    
                elif choice == len(available_roles)+1:
                    print(f"{Colors.YELLOW}⏭️ Dispositivo saltado{Colors.END}")
                    break
                    
                else:
                    print(f"{Colors.RED}❌ Opción inválida{Colors.END}")
                    
            except ValueError:
                print(f"{Colors.RED}❌ Ingresa un número válido{Colors.END}")
    
    if learned_assignments:
        save_known_devices(learned_assignments)
        print(f"\n{Colors.GREEN}💾 Configuración guardada en {DEVICE_CONFIG_FILE}{Colors.END}")
        print(f"{Colors.CYAN}🎉 ¡Ahora los dispositivos se detectarán automáticamente por chip ID!{Colors.END}")
    
    return learned_assignments

def get_device_chip_id(port):
    """Obtiene el chip ID único del ESP32 usando esptool"""
    try:
        # Usar esptool para obtener el chip ID de forma más confiable
        cmd = [
            "/Users/jmoisio/.platformio/penv/bin/python", 
            "-m", "esptool",
            "--port", port,
            "--baud", "115200", 
            "chip_id"
        ]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
        
        if result.returncode == 0:
            # Buscar el MAC address en la salida
            import re
            mac_pattern = r'MAC: ([0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2}:[0-9a-f]{2})'
            match = re.search(mac_pattern, result.stdout, re.IGNORECASE)
            
            if match:
                return match.group(1).upper()
        
        # Fallback: intentar leer directamente del puerto serie
        try:
            import serial
            import time
            
            ser = serial.Serial(port, 115200, timeout=1)
            time.sleep(0.5)
            
            # Enviar Ctrl+C para interrumpir cualquier programa
            ser.write(b'\x03')
            time.sleep(0.2)
            
            # Leer WiFi.macAddress() si hay un programa Arduino corriendo
            ser.write(b'WiFi.macAddress()\n')
            time.sleep(0.5)
            
            response = ser.read_all().decode('utf-8', errors='ignore')
            ser.close()
            
            # Buscar patrón de MAC address
            import re
            mac_pattern = r'([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})'
            match = re.search(mac_pattern, response)
            
            if match:
                return match.group(0)
                
        except Exception:
            pass
        
    except Exception as e:
        print(f"{Colors.YELLOW}   ⚠️ Error obteniendo chip ID de {port}: {e}{Colors.END}")
    
    return None

def detect_esp32_devices():
    """Detecta automáticamente los ESP32 conectados y los identifica por chip ID"""
    print(f"{Colors.YELLOW}🔍 Detectando dispositivos ESP32...{Colors.END}")
    
    devices = {
        "BROKER": {"port": None, "type": "ESP32-C3", "chip_id": None},
        "FINGERPRINT": {"port": None, "type": "ESP32-WROOM", "chip_id": None}, 
        "RFID": {"port": None, "type": "ESP32-WROOM", "chip_id": None}
    }
    
    # Cargar chip IDs conocidos
    known_devices = load_known_devices()
    
    ports = serial.tools.list_ports.comports()
    detected_ports = []
    
    for port in ports:
        port_name = port.device
        port_desc = port.description.lower()
        
        # Filtrar solo puertos ESP32 probables
        is_esp32 = any(keyword in port_desc for keyword in [
            'jtag', 'debug unit', 'usbmodem', 'usb serial', 'usbserial', 
            'esp32', 'cp210', 'ch340', 'ft232'
        ])
        
        if is_esp32:
            print(f"{Colors.BLUE}   📱 {port_name} - {port.description}{Colors.END}")
            detected_ports.append({
                "port": port_name,
                "desc": port.description,
                "chip_id": None
            })
    
    # Si hay dispositivos conocidos por chip ID, intentar identificarlos
    if known_devices and detected_ports:
        print(f"{Colors.CYAN}🔍 Identificando dispositivos por chip ID...{Colors.END}")
        for port_info in detected_ports:
            chip_id = get_device_chip_id(port_info["port"])
            port_info["chip_id"] = chip_id
            
            if chip_id and chip_id in known_devices:
                device_role = known_devices[chip_id]
                devices[device_role]["port"] = port_info["port"]
                devices[device_role]["chip_id"] = chip_id
                print(f"{Colors.GREEN}   ✅ {device_role} identificado: {port_info['port']} ({chip_id}){Colors.END}")
    
    # Si no hay dispositivos conocidos o no se identificaron, usar detección por tipo
    unassigned_ports = [p for p in detected_ports if not any(
        d["port"] == p["port"] for d in devices.values() if d["port"]
    )]
    
    if unassigned_ports:
        print(f"{Colors.YELLOW}⚠️ Asignando puertos por tipo de hardware...{Colors.END}")
        
        for port_info in unassigned_ports:
            port_name = port_info["port"]
            port_desc = port_info["desc"].lower()
            
            # ESP32-C3 generalmente aparece como USB JTAG/serial debug unit
            if ('jtag' in port_desc or 'debug unit' in port_desc) and not devices["BROKER"]["port"]:
                devices["BROKER"]["port"] = port_name
                devices["BROKER"]["chip_id"] = port_info["chip_id"]
                print(f"{Colors.CYAN}   🔷 BROKER (ESP32-C3): {port_name}{Colors.END}")
                
            # ESP32-WROOM para fingerprint y RFID
            elif ('usb serial' in port_desc or 'usbserial' in port_name or 'cp210' in port_desc):
                if not devices["FINGERPRINT"]["port"]:
                    devices["FINGERPRINT"]["port"] = port_name
                    devices["FINGERPRINT"]["chip_id"] = port_info["chip_id"]
                    print(f"{Colors.GREEN}   🔶 FINGERPRINT (ESP32-WROOM): {port_name}{Colors.END}")
                elif not devices["RFID"]["port"]:
                    devices["RFID"]["port"] = port_name
                    devices["RFID"]["chip_id"] = port_info["chip_id"]
                    print(f"{Colors.MAGENTA}   🏷️ RFID (ESP32-WROOM): {port_name}{Colors.END}")
    
    return devices

def upload_to_device(project_path, device_port, device_name):
    """Sube código a un dispositivo específico"""
    print(f"\n{Colors.YELLOW}🚀 Subiendo código a {device_name}...{Colors.END}")
    print(f"{Colors.BLUE}   📁 Proyecto: {project_path}{Colors.END}")
    print(f"{Colors.BLUE}   📱 Puerto: {device_port}{Colors.END}")
    
    try:
        # Cambiar al directorio del proyecto
        original_dir = os.getcwd()
        os.chdir(project_path)
        
        # Comando platformio con puerto específico
        cmd = [
            "/Users/jmoisio/.platformio/penv/bin/platformio",
            "run",
            "--target", "upload",
            "--upload-port", device_port
        ]
        
        print(f"{Colors.CYAN}   ⚡ Ejecutando: {' '.join(cmd)}{Colors.END}")
        
        # Ejecutar comando
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            timeout=120  # 2 minutos timeout
        )
        
        # Restaurar directorio original
        os.chdir(original_dir)
        
        if result.returncode == 0:
            print(f"{Colors.GREEN}   ✅ {device_name} - Upload exitoso!{Colors.END}")
            return True
        else:
            print(f"{Colors.RED}   ❌ {device_name} - Error en upload:{Colors.END}")
            print(f"{Colors.RED}      {result.stderr}{Colors.END}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"{Colors.RED}   ❌ {device_name} - Timeout (>2min){Colors.END}")
        os.chdir(original_dir)
        return False
    except Exception as e:
        print(f"{Colors.RED}   ❌ {device_name} - Error: {e}{Colors.END}")
        os.chdir(original_dir)
        return False

def main():
    print(f"{Colors.BOLD}{Colors.MAGENTA}")
    print("=" * 70)
    print("    🚀 ESP32 MULTI-DEVICE AUTO-UPLOAD SYSTEM")
    print("=" * 70)
    print(f"{Colors.END}")
    
    # Detectar dispositivos
    devices = detect_esp32_devices()
    
    # Verificar si hay algún dispositivo
    connected_devices = [k for k, v in devices.items() if v["port"]]
    if not connected_devices:
        print(f"{Colors.RED}❌ No se detectaron dispositivos ESP32{Colors.END}")
        return False
    
    print(f"\n{Colors.GREEN}📋 Dispositivos detectados:{Colors.END}")
    for device_name, device_info in devices.items():
        if device_info["port"]:
            chip_info = f" ({device_info['chip_id']})" if device_info["chip_id"] else ""
            if device_name == "BROKER":
                print(f"{Colors.CYAN}   🔷 {device_name} ({device_info['type']}): {device_info['port']}{chip_info}{Colors.END}")
            elif device_name == "FINGERPRINT":
                print(f"{Colors.GREEN}   🔶 {device_name} ({device_info['type']}): {device_info['port']}{chip_info}{Colors.END}")
            elif device_name == "RFID":
                print(f"{Colors.MAGENTA}   🏷️ {device_name} ({device_info['type']}): {device_info['port']}{chip_info}{Colors.END}")
    
    # Rutas de los proyectos
    projects = {
        "BROKER": "/Users/jmoisio/Documents/PlatformIO/Projects/BorkerMQTT",
        "FINGERPRINT": "/Users/jmoisio/Documents/PlatformIO/Projects/HuellaDactilar", 
        "RFID": "/Users/jmoisio/Documents/PlatformIO/Projects/RFIDReader"
    }
    
    # Verificar si los proyectos existen
    available_projects = {}
    for device_name, project_path in projects.items():
        if os.path.exists(project_path):
            available_projects[device_name] = project_path
        else:
            print(f"{Colors.YELLOW}⚠️ Proyecto {device_name} no encontrado: {project_path}{Colors.END}")
    
    if not available_projects:
        print(f"{Colors.RED}❌ No se encontraron proyectos válidos{Colors.END}")
        return False
    
    print(f"\n{Colors.YELLOW}🎯 Selecciona qué subir:{Colors.END}")
    options = []
    
    # Opciones individuales
    if "BROKER" in connected_devices and "BROKER" in available_projects:
        options.append(("BROKER", f"{Colors.CYAN}  1. Solo BROKER (ESP32-C3 - MQTT Server){Colors.END}"))
    
    if "FINGERPRINT" in connected_devices and "FINGERPRINT" in available_projects:
        option_num = len(options) + 1
        options.append(("FINGERPRINT", f"{Colors.GREEN}  {option_num}. Solo FINGERPRINT (ESP32-WROOM - Huella Dactilar){Colors.END}"))
    
    if "RFID" in connected_devices and "RFID" in available_projects:
        option_num = len(options) + 1
        options.append(("RFID", f"{Colors.MAGENTA}  {option_num}. Solo RFID (ESP32-WROOM - Lector RFID){Colors.END}"))
    
    # Opciones combinadas
    if len([d for d in connected_devices if d in available_projects]) >= 2:
        # FINGERPRINT + BROKER (para cambios en protocolo de comunicación)
        if "FINGERPRINT" in connected_devices and "BROKER" in connected_devices and "FINGERPRINT" in available_projects and "BROKER" in available_projects:
            option_num = len(options) + 1
            options.append(("FINGERPRINT_SERVER", f"{Colors.GREEN}  {option_num}. FINGERPRINT + BROKER (Huella Dactilar + Servidor){Colors.END}"))
        
        # RFID + BROKER 
        if "RFID" in connected_devices and "BROKER" in connected_devices and "RFID" in available_projects and "BROKER" in available_projects:
            option_num = len(options) + 1
            options.append(("RFID_SERVER", f"{Colors.BLUE}  {option_num}. RFID + BROKER (Lector RFID + Servidor){Colors.END}"))
        
        # TODOS los dispositivos
        option_num = len(options) + 1
        options.append(("ALL", f"{Colors.BOLD}  {option_num}. TODOS los dispositivos disponibles{Colors.END}"))
    
    # Opción de aprendizaje
    if len(connected_devices) >= 2:
        option_num = len(options) + 1
        options.append(("LEARN", f"{Colors.CYAN}  {option_num}. 🎓 Modo Aprendizaje (Asignar dispositivos manualmente){Colors.END}"))
    
    # Opción cancelar
    option_num = len(options) + 1
    options.append(("CANCEL", f"{Colors.RED}  {option_num}. Cancelar{Colors.END}"))
    
    # Mostrar opciones
    for _, option_text in options:
        print(option_text)
    
    choice = input(f"\n{Colors.YELLOW}Elige opción (1-{len(options)}): {Colors.END}")
    
    try:
        choice_idx = int(choice) - 1
        if choice_idx < 0 or choice_idx >= len(options):
            raise ValueError("Opción fuera de rango")
        
        selected_option = options[choice_idx][0]
    except (ValueError, IndexError):
        print(f"{Colors.RED}❌ Opción inválida{Colors.END}")
        return False
    
    success_count = 0
    
    if selected_option == "BROKER":
        if upload_to_device(available_projects["BROKER"], devices["BROKER"]["port"], "BROKER (ESP32-C3)"):
            success_count += 1
            
    elif selected_option == "FINGERPRINT":
        if upload_to_device(available_projects["FINGERPRINT"], devices["FINGERPRINT"]["port"], "FINGERPRINT (ESP32-WROOM)"):
            success_count += 1
            
    elif selected_option == "RFID":
        if upload_to_device(available_projects["RFID"], devices["RFID"]["port"], "RFID (ESP32-WROOM)"):
            success_count += 1
            
    elif selected_option == "FINGERPRINT_SERVER":
        print(f"\n{Colors.GREEN}🔄 Subiendo FINGERPRINT + BROKER...{Colors.END}")
        
        if "FINGERPRINT" in available_projects and devices["FINGERPRINT"]["port"]:
            if upload_to_device(available_projects["FINGERPRINT"], devices["FINGERPRINT"]["port"], "FINGERPRINT (ESP32-WROOM)"):
                success_count += 1
        
        if "BROKER" in available_projects and devices["BROKER"]["port"]:
            if upload_to_device(available_projects["BROKER"], devices["BROKER"]["port"], "BROKER (ESP32-C3)"):
                success_count += 1
            
    elif selected_option == "RFID_SERVER":
        print(f"\n{Colors.BLUE}🔄 Subiendo RFID + BROKER...{Colors.END}")
        
        if "RFID" in available_projects and devices["RFID"]["port"]:
            if upload_to_device(available_projects["RFID"], devices["RFID"]["port"], "RFID (ESP32-WROOM)"):
                success_count += 1
        
        if "BROKER" in available_projects and devices["BROKER"]["port"]:
            if upload_to_device(available_projects["BROKER"], devices["BROKER"]["port"], "BROKER (ESP32-C3)"):
                success_count += 1
                
    elif selected_option == "ALL":
        print(f"\n{Colors.BOLD}🔄 Subiendo a TODOS los dispositivos...{Colors.END}")
        
        for device_name in ["BROKER", "FINGERPRINT", "RFID"]:
            if device_name in available_projects and devices[device_name]["port"]:
                device_type = devices[device_name]["type"]
                if upload_to_device(available_projects[device_name], devices[device_name]["port"], f"{device_name} ({device_type})"):
                    success_count += 1
            else:
                print(f"{Colors.YELLOW}⚠️ {device_name} no disponible, saltando...{Colors.END}")
                
    elif selected_option == "LEARN":
        learned = learn_device_assignment(devices)
        if learned:
            print(f"{Colors.GREEN}✅ ¡Configuración aprendida! Ejecuta el script nuevamente para usar la detección automática{Colors.END}")
            return True
        else:
            print(f"{Colors.YELLOW}⚠️ No se guardó ninguna configuración{Colors.END}")
            return False
                
    elif selected_option == "CANCEL":
        print(f"{Colors.BLUE}🚪 Operación cancelada{Colors.END}")
        return True
    
    # Resumen final
    print(f"\n{Colors.BOLD}📊 RESUMEN:{Colors.END}")
    if success_count > 0:
        print(f"{Colors.GREEN}✅ Uploads exitosos: {success_count}{Colors.END}")
        print(f"{Colors.CYAN}🎉 ¡Listo para usar!{Colors.END}")
    else:
        print(f"{Colors.RED}❌ No se completaron uploads exitosos{Colors.END}")
    
    return success_count > 0

if __name__ == "__main__":
    try:
        success = main()
        sys.exit(0 if success else 1)
    except KeyboardInterrupt:
        print(f"\n{Colors.YELLOW}🛑 Operación interrumpida{Colors.END}")
        sys.exit(1)
    except Exception as e:
        print(f"{Colors.RED}❌ Error inesperado: {e}{Colors.END}")
        sys.exit(1)