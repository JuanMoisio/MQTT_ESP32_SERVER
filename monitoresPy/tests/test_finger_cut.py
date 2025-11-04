#!/usr/bin/env python3
"""
Script de prueba para el comando finger_cut
Permite probar el comando MQTT server:finger_cut:id desde Python
"""

import socket
import time
import json
import sys

BROKER_IP = "192.168.4.1"  # IP del ESP32 broker
BROKER_PORT = 1883

def send_finger_cut_command(finger_id):
    """
    Envía el comando finger_cut al broker ESP32
    
    Args:
        finger_id (int): ID de la huella a eliminar
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
            "module_id": "test_client_finger_cut",
            "module_type": "test_controller",
            "capabilities": "command_sender"
        }
        
        client.send((json.dumps(register_msg) + "\n").encode())
        
        # Esperar respuesta de registro
        response = client.recv(1024).decode('utf-8').strip()
        print(f"📨 Registro: {response}")
        
        # Crear comando para eliminar huella
        command_msg = {
            "type": "command",
            "module_id": "fingerprint_scanner",  # Buscar módulo de huella
            "command": f"delete_user:{finger_id}",
            "timestamp": int(time.time() * 1000)
        }
        
        print(f"✂️ Enviando comando finger_cut para ID {finger_id}...")
        client.send((json.dumps(command_msg) + "\n").encode())
        
        # Escuchar respuestas por unos segundos
        print("👂 Escuchando respuestas...")
        client.settimeout(5)
        
        try:
            while True:
                response = client.recv(1024).decode('utf-8').strip()
                if response:
                    try:
                        data = json.loads(response)
                        if data.get("type") == "publish" and "delete_result" in str(data):
                            print(f"✅ Resultado: {response}")
                            break
                        else:
                            print(f"📨 Mensaje: {response}")
                    except json.JSONDecodeError:
                        print(f"📨 Raw: {response}")
        except socket.timeout:
            print("⏰ Timeout - no más respuestas")
        
        client.close()
        print("✅ Conexión cerrada")
        
    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("=== TEST FINGER_CUT COMMAND ===")
    print("Este script prueba el comando server:finger_cut:id")
    print("Asegúrate de que:")
    print("1. El ESP32 broker esté ejecutándose")
    print("2. El cliente de huella esté conectado")
    print("3. Tengas huellas registradas para probar")
    print()
    
    if len(sys.argv) != 2:
        print("Uso: python test_finger_cut.py <id_de_huella>")
        print("Ejemplo: python test_finger_cut.py 1")
        sys.exit(1)
    
    try:
        finger_id = int(sys.argv[1])
        if finger_id < 1:
            print("❌ Error: El ID debe ser mayor a 0")
            sys.exit(1)
            
        send_finger_cut_command(finger_id)
        
    except ValueError:
        print("❌ Error: El ID debe ser un número entero")
        sys.exit(1)

if __name__ == "__main__":
    main()
