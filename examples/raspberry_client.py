#!/usr/bin/env python3
"""
Cliente MQTT para Raspberry Pi - Sistema de Depósito
Demuestra cómo interactuar con el broker ESP32-C3 y los módulos conectados.
"""

import paho.mqtt.client as mqtt
import json
import time
import threading
from datetime import datetime

class DepositoBrokerClient:
    def __init__(self, broker_ip="192.168.4.1", broker_port=1883):
        self.broker_ip = broker_ip
        self.broker_port = broker_port
        self.client = mqtt.Client(client_id="raspberry_pi_deposito")
        
        # Estado del sistema
        self.registered_modules = {}
        self.sensor_data = {}
        self.system_status = {}
        
        # Configurar callbacks
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"✅ Conectado al broker ESP32-C3 en {self.broker_ip}:{self.broker_port}")
            
            # Suscribirse a todos los topics importantes
            topics = [
                "deposito/+/status/+",      # Estados de módulos
                "deposito/+/data/+",        # Datos de sensores  
                "deposito/system/+",        # Mensajes del sistema
                "deposito/+/+/+"           # Todos los mensajes (debug)
            ]
            
            for topic in topics:
                client.subscribe(topic)
                print(f"📡 Suscrito a: {topic}")
            
            # Solicitar lista de módulos registrados
            self.request_module_list()
            
        else:
            print(f"❌ Error conectando al broker. Código: {rc}")
    
    def on_disconnect(self, client, userdata, rc):
        if rc != 0:
            print("⚠️ Desconexión inesperada del broker")
    
    def on_message(self, client, userdata, msg):
        try:
            topic = msg.topic
            payload = json.loads(msg.payload.decode())
            
            # Log del mensaje recibido
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"\n[{timestamp}] 📨 {topic}")
            
            # Procesar según el tipo de mensaje
            if "/status/" in topic:
                self.handle_status_message(topic, payload)
            elif "/data/" in topic:
                self.handle_sensor_data(topic, payload)
            elif "/system/" in topic:
                self.handle_system_message(topic, payload)
            else:
                self.handle_other_message(topic, payload)
                
        except json.JSONDecodeError as e:
            print(f"❌ Error decodificando JSON: {e}")
        except Exception as e:
            print(f"❌ Error procesando mensaje: {e}")
    
    def handle_status_message(self, topic, payload):
        """Procesar mensajes de estado de módulos"""
        module_id = payload.get("module_id", "unknown")
        status = payload.get("status", "unknown")
        
        print(f"   📊 Módulo {module_id}: {status}")
        
        # Guardar estado del módulo
        if module_id not in self.system_status:
            self.system_status[module_id] = {}
        
        self.system_status[module_id].update(payload)
        self.system_status[module_id]["last_update"] = time.time()
        
        # Procesar estados específicos
        if "door" in topic:
            door_status = payload.get("status")
            print(f"   🚪 Estado de puerta: {door_status}")
        elif "position" in topic:
            position = payload.get("position", 0)
            print(f"   🔧 Posición motor: {position}")
    
    def handle_sensor_data(self, topic, payload):
        """Procesar datos de sensores"""
        module_id = payload.get("module_id", "unknown")
        timestamp = payload.get("timestamp", time.time())
        
        # Guardar datos del sensor
        if module_id not in self.sensor_data:
            self.sensor_data[module_id] = {}
        
        # Determinar tipo de sensor por el topic
        if "temperature" in topic:
            temp = payload.get("temperature")
            battery = payload.get("battery_level", "N/A")
            print(f"   🌡️  Temperatura: {temp}°C (Batería: {battery}%)")
            
            self.sensor_data[module_id]["temperature"] = temp
            self.sensor_data[module_id]["battery"] = battery
            
        elif "humidity" in topic:
            humidity = payload.get("humidity")
            print(f"   💧 Humedad: {humidity}%")
            
            self.sensor_data[module_id]["humidity"] = humidity
        
        self.sensor_data[module_id]["last_update"] = timestamp
    
    def handle_system_message(self, topic, payload):
        """Procesar mensajes del sistema"""
        msg_type = payload.get("type", "unknown")
        
        if msg_type == "config_response":
            config_type = payload.get("config_type")
            
            if config_type == "modules_list":
                print("   📋 Lista de módulos recibida:")
                modules = payload.get("modules", [])
                
                for module in modules:
                    module_id = module.get("module_id")
                    module_type = module.get("module_type") 
                    is_active = module.get("is_active")
                    capabilities = module.get("capabilities")
                    
                    status_icon = "🟢" if is_active else "🔴"
                    print(f"      {status_icon} {module_id} ({module_type})")
                    print(f"         Capacidades: {capabilities}")
                    
                    self.registered_modules[module_id] = module
        
        elif msg_type == "module_status":
            module_id = payload.get("module_id")
            status = payload.get("status")
            print(f"   📡 {module_id} cambió a: {status}")
    
    def handle_other_message(self, topic, payload):
        """Procesar otros mensajes"""
        msg_type = payload.get("type", "message")
        print(f"   📝 Tipo: {msg_type}")
        
        if isinstance(payload, dict):
            for key, value in payload.items():
                if key != "type":
                    print(f"      {key}: {value}")
    
    def request_module_list(self):
        """Solicitar lista de módulos al broker"""
        config_msg = {
            "type": "config",
            "config_type": "get_modules"
        }
        
        self.client.publish("deposito/system/config", json.dumps(config_msg))
        print("📋 Solicitando lista de módulos...")
    
    def send_command_to_module(self, module_type, command, payload=None):
        """Enviar comando a un tipo específico de módulo"""
        topic = f"deposito/{module_type}/cmd/{command}"
        
        message = {
            "type": "publish",
            "topic": topic,
            "payload": payload or {}
        }
        
        self.client.publish("", json.dumps(message))
        print(f"📤 Comando enviado: {command} a {module_type}")
    
    def unlock_door(self):
        """Ejemplo: desbloquear puerta"""
        payload = {
            "action": "unlock",
            "duration": 5000,  # 5 segundos
            "timestamp": int(time.time() * 1000)
        }
        
        self.send_command_to_module("control_acceso", "unlock", payload)
    
    def move_motor(self, motor_id, position):
        """Ejemplo: mover motor a posición"""
        payload = {
            "action": "move_to_position",
            "position": position,
            "speed": 100,
            "timestamp": int(time.time() * 1000)
        }
        
        self.send_command_to_module("motor", "move", payload)
    
    def get_sensor_status(self):
        """Solicitar estado de sensores"""
        payload = {
            "action": "get_status",
            "timestamp": int(time.time() * 1000)
        }
        
        self.send_command_to_module("sensor_temperatura", "get_status", payload)
    
    def print_system_summary(self):
        """Imprimir resumen del sistema"""
        print("\n" + "="*50)
        print("📊 RESUMEN DEL SISTEMA")
        print("="*50)
        
        print(f"🔗 Módulos registrados: {len(self.registered_modules)}")
        for module_id, module in self.registered_modules.items():
            status_icon = "🟢" if module.get("is_active") else "🔴"
            print(f"   {status_icon} {module_id} ({module.get('module_type')})")
        
        print(f"\n📡 Datos de sensores activos: {len(self.sensor_data)}")
        for module_id, data in self.sensor_data.items():
            last_update = data.get("last_update", 0)
            age = time.time() - (last_update / 1000 if last_update > 1000000000000 else last_update)
            
            print(f"   📊 {module_id} (hace {age:.1f}s):")
            if "temperature" in data:
                print(f"      🌡️  {data['temperature']}°C")
            if "humidity" in data:
                print(f"      💧 {data['humidity']}%")
            if "battery" in data:
                print(f"      🔋 {data['battery']}%")
        
        print("="*50)
    
    def connect_and_run(self):
        """Conectar al broker y mantener la conexión"""
        try:
            self.client.connect(self.broker_ip, self.broker_port, 60)
            
            # Iniciar el loop en un hilo separado
            self.client.loop_start()
            
            print("🚀 Cliente iniciado. Presiona Ctrl+C para salir.")
            print("📝 Comandos disponibles:")
            print("   - 's': Resumen del sistema")
            print("   - 'u': Desbloquear puerta") 
            print("   - 'm': Mover motor")
            print("   - 't': Estado de sensores")
            print("   - 'q': Salir")
            
            # Hilo para comandos de usuario
            def user_input_thread():
                while True:
                    try:
                        cmd = input("\n> ").strip().lower()
                        
                        if cmd == 'q':
                            break
                        elif cmd == 's':
                            self.print_system_summary()
                        elif cmd == 'u':
                            self.unlock_door()
                        elif cmd == 'm':
                            position = input("Posición (0-100): ")
                            try:
                                pos = int(position)
                                self.move_motor("motor1", pos)
                            except ValueError:
                                print("❌ Posición inválida")
                        elif cmd == 't':
                            self.get_sensor_status()
                        elif cmd == '':
                            continue
                        else:
                            print("❌ Comando no reconocido")
                            
                    except KeyboardInterrupt:
                        break
                    except EOFError:
                        break
            
            input_thread = threading.Thread(target=user_input_thread, daemon=True)
            input_thread.start()
            
            # Mantener el programa corriendo
            try:
                while input_thread.is_alive():
                    time.sleep(1)
            except KeyboardInterrupt:
                pass
                
        except Exception as e:
            print(f"❌ Error: {e}")
        
        finally:
            print("\n👋 Desconectando...")
            self.client.loop_stop()
            self.client.disconnect()

def main():
    print("🏭 Cliente Raspberry Pi - Sistema de Depósito")
    print("=" * 50)
    
    # Configurar IP del broker ESP32-C3 (Access Point)
    broker_ip = input("IP del broker ESP32-C3 (default: 192.168.4.1): ").strip()
    if not broker_ip:
        broker_ip = "192.168.4.1"  # IP fija del ESP32-C3 como Access Point
    
    # Crear y ejecutar cliente
    client = DepositoBrokerClient(broker_ip)
    client.connect_and_run()

if __name__ == "__main__":
    main()