#!/usr/bin/env python3
"""
Script de prueba para el comando list_fingerprints
Permite probar el listado de huellas registradas via MQTT
"""

import socket
import time
import json
import sys

BROKER_IP = "192.168.4.1"  # IP del ESP32 broker
BROKER_PORT = 1883

def send_list_fingerprints_command():
    """
    Envía el comando list_fingerprints al broker ESP32
    """
    try:
        # Conectar al broker
        client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        client.settimeout(10)
        
        print(f"🔗 Conectando a {BROKER_IP}:{BROKER_PORT}...")
        client.connect((BROKER_IP, BROKER_PORT))
        
        # Esperar mensaje de bienvenida
        welcome = client.recv(1024).decode('utf-8').strip()
        print(f"📨 Bienvenida: {welcome}")
        
        # Registrarse como cliente de prueba
        register_msg = {
            "type": "register",
            "module_id": "test_client_list_fp",
            "module_type": "test_controller", 
            "capabilities": "command_sender"
        }
        
        client.send((json.dumps(register_msg) + "\n").encode())
        
        # Esperar respuesta de registro
        response = client.recv(1024).decode('utf-8').strip()
        print(f"📨 Registro: {response}")
        
        # Crear comando para listar huellas
        command_msg = {
            "type": "command",
            "module_id": "fingerprint_scanner",
            "command": "list_all_fingerprints",
            "timestamp": int(time.time() * 1000)
        }
        
        print("📋 Enviando comando list_fingerprints...")
        client.send((json.dumps(command_msg) + "\n").encode())
        
        # Escuchar respuestas
        print("👂 Escuchando respuestas...")
        client.settimeout(10)  # Más tiempo para el escaneo
        
        try:
            while True:
                response = client.recv(2048).decode('utf-8').strip()
                if response:
                    try:
                        data = json.loads(response)
                        if (data.get("type") == "publish" and 
                            "fingerprint_list" in str(data)):
                            
                            payload = data.get("payload", {})
                            event_data = payload.get("data", {})
                            
                            print("\n📊 RESULTADOS:")
                            print(f"Total registrados: {event_data.get('total_count', 0)}")
                            
                            registered_ids = event_data.get("registered_ids", [])
                            registered_names = event_data.get("registered_names", [])
                            
                            if registered_ids:
                                print("\n👤 Usuarios registrados:")
                                for i, user_id in enumerate(registered_ids):
                                    name = registered_names[i] if i < len(registered_names) else "Sin nombre"
                                    print(f"  ID {user_id}: {name}")
                            else:
                                print("❌ No hay huellas registradas")
                            
                            print(f"\n✅ Respuesta completa: {json.dumps(event_data, indent=2)}")
                            break
                        else:
                            print(f"📨 Otro mensaje: {response}")
                    except json.JSONDecodeError:
                        print(f"📨 Raw: {response}")
        except socket.timeout:
            print("⏰ Timeout - no se recibió respuesta")
        
        client.close()
        print("✅ Conexión cerrada")
        
    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("=== TEST LIST_FINGERPRINTS COMMAND ===")
    print("Este script lista todas las huellas registradas")
    print("Asegúrate de que:")
    print("1. El ESP32 broker esté ejecutándose")
    print("2. El cliente de huella esté conectado")
    print("3. Tengas algunas huellas registradas")
    print()
    
    send_list_fingerprints_command()

if __name__ == "__main__":
    main()
