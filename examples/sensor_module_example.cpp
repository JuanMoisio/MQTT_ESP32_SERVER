/*
 * ✅ CÓDIGO COMPILADO Y CORREGIDO ✅
 * 
 * MÓDULO DE EJEMPLO - SENSOR DE TEMPERATURA
 * 
 * Este es un ejemplo de cómo crear un módulo ESP32 que se conecta
 * al broker ESP32-C3 y envía datos de temperatura simulados.
 * 
 * Hardware sugerido:
 * - ESP32 DevKit
 * - Sensor DHT22 (opcional, se simula si no está conectado)
 * - LED indicador (opcional)
 * 
 * Configuración:
 * 1. Cambiar BROKER_IP por la IP de tu ESP32-C3
 * 2. Cambiar WiFi credentials
 * 3. Compilar y subir a ESP32
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Configuración de red - Conectar al Access Point del ESP32-C3
const char* ssid = "DEPOSITO_BROKER";       // SSID del AP del ESP32-C3
const char* password = "deposito123";       // Password del AP del ESP32-C3  
const char* brokerIP = "192.168.4.1";      // IP fija del ESP32-C3 como AP
const int brokerPort = 1883;

// Configuración del módulo
const String MODULE_ID = "sensor_temp_001";
const String MODULE_TYPE = "sensor_temperatura";
const String CAPABILITIES = "temperature,humidity,battery_monitor";

// Cliente WiFi
WiFiClient client;

// Variables de estado
bool connectedToBroker = false;
unsigned long lastHeartbeat = 0;
unsigned long lastDataSend = 0;
unsigned long lastReconnectAttempt = 0;

const unsigned long HEARTBEAT_INTERVAL = 30000;  // 30 segundos
const unsigned long DATA_SEND_INTERVAL = 10000;  // 10 segundos
const unsigned long RECONNECT_INTERVAL = 5000;   // 5 segundos

// Pines (opcional)
const int LED_PIN = 2;        // LED indicador
const int DHT_PIN = 4;        // Sensor DHT22 (si está conectado)

// Datos simulados
float temperature = 25.0;
float humidity = 60.0;
int batteryLevel = 100;

void setup() {
  Serial.begin(115200);
  
  // Configurar pines
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  Serial.println("Iniciando módulo sensor de temperatura...");
  
  // Conectar a WiFi
  setupWiFi();
  
  // Conectar al broker
  connectToBroker();
  
  Serial.println("Módulo listo!");
}

void loop() {
  // Verificar conexión WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi desconectado, reconectando...");
    setupWiFi();
    return;
  }
  
  // Verificar conexión al broker
  if (!connectedToBroker || !client.connected()) {
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      Serial.println("Reconectando al broker...");
      connectToBroker();
      lastReconnectAttempt = millis();
    }
    return;
  }
  
  // Procesar mensajes del broker
  processMessages();
  
  // Enviar heartbeat
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
  }
  
  // Enviar datos de sensores
  if (millis() - lastDataSend > DATA_SEND_INTERVAL) {
    sendSensorData();
    lastDataSend = millis();
  }
  
  delay(100);
}

void setupWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.print("WiFi conectado! IP: ");
  Serial.println(WiFi.localIP());
  
  digitalWrite(LED_PIN, HIGH);  // Indicar WiFi conectado
}

void connectToBroker() {
  Serial.print("Conectando al broker ");
  Serial.print(brokerIP);
  Serial.print(":");
  Serial.println(brokerPort);
  
  if (client.connect(brokerIP, brokerPort)) {
    Serial.println("Conectado al broker!");
    connectedToBroker = true;
    
    // Registrar módulo
    registerModule();
    
    // Suscribirse a comandos para este módulo
    subscribeToCommands();
    
  } else {
    Serial.println("Error conectando al broker");
    connectedToBroker = false;
  }
}

void registerModule() {
  JsonDocument registerMsg;
  registerMsg["type"] = "register";
  registerMsg["module_id"] = MODULE_ID;
  registerMsg["module_type"] = MODULE_TYPE;
  registerMsg["capabilities"] = CAPABILITIES;
  registerMsg["ip"] = WiFi.localIP().toString();
  registerMsg["timestamp"] = millis();
  
  String registerStr;
  serializeJson(registerMsg, registerStr);
  
  client.println(registerStr);
  Serial.println("Enviado mensaje de registro:");
  Serial.println(registerStr);
}

void subscribeToCommands() {
  JsonDocument subscribeMsg;
  subscribeMsg["type"] = "subscribe";
  subscribeMsg["topic"] = "deposito/" + MODULE_TYPE + "/cmd/+";
  subscribeMsg["module_id"] = MODULE_ID;
  
  String subscribeStr;
  serializeJson(subscribeMsg, subscribeStr);
  
  client.println(subscribeStr);
  Serial.println("Suscrito a comandos");
}

void sendHeartbeat() {
  JsonDocument heartbeatMsg;
  heartbeatMsg["type"] = "heartbeat";
  heartbeatMsg["module_id"] = MODULE_ID;
  heartbeatMsg["timestamp"] = millis();
  heartbeatMsg["status"] = "online";
  
  String heartbeatStr;
  serializeJson(heartbeatMsg, heartbeatStr);
  
  client.println(heartbeatStr);
  
  Serial.print("Heartbeat enviado - ");
  Serial.println(millis());
}

void sendSensorData() {
  // Simular lecturas de sensores (cambiar por lecturas reales)
  updateSimulatedSensorData();
  
  // Enviar temperatura
  JsonDocument tempMsg;
  tempMsg["type"] = "publish";
  tempMsg["topic"] = "deposito/sensor_temperatura/data/temperature";
  
  JsonObject payload = tempMsg["payload"];
  payload["module_id"] = MODULE_ID;
  payload["temperature"] = temperature;
  payload["unit"] = "celsius";
  payload["timestamp"] = millis();
  payload["battery_level"] = batteryLevel;
  
  String tempStr;
  serializeJson(tempMsg, tempStr);
  client.println(tempStr);
  
  // Enviar humedad
  JsonDocument humMsg;
  humMsg["type"] = "publish";
  humMsg["topic"] = "deposito/sensor_temperatura/data/humidity";
  
  JsonObject humPayload = humMsg["payload"];
  humPayload["module_id"] = MODULE_ID;
  humPayload["humidity"] = humidity;
  humPayload["unit"] = "percent";
  humPayload["timestamp"] = millis();
  
  String humStr;
  serializeJson(humMsg, humStr);
  client.println(humStr);
  
  Serial.print("Datos enviados - T:");
  Serial.print(temperature);
  Serial.print("°C H:");
  Serial.print(humidity);
  Serial.println("%");
}

void updateSimulatedSensorData() {
  // Simular variaciones realistas de temperatura y humedad
  
  // Temperatura: 20-30°C con variación lenta
  static float tempTrend = 0.1;
  temperature += (random(-10, 11) / 100.0) + tempTrend;
  
  if (temperature > 30.0) tempTrend = -0.1;
  if (temperature < 20.0) tempTrend = 0.1;
  
  // Humedad: 40-80% con variación
  static float humTrend = 0.2;
  humidity += (random(-20, 21) / 100.0) + humTrend;
  
  if (humidity > 80.0) humTrend = -0.2;
  if (humidity < 40.0) humTrend = 0.2;
  
  // Batería: simular descarga lenta
  static unsigned long lastBatteryUpdate = 0;
  if (millis() - lastBatteryUpdate > 300000) {  // cada 5 minutos
    batteryLevel--;
    if (batteryLevel < 0) batteryLevel = 100;  // "recargar" para la demo
    lastBatteryUpdate = millis();
  }
}

void processMessages() {
  if (client.available()) {
    String message = client.readStringUntil('\n');
    message.trim();
    
    if (message.length() > 0) {
      Serial.println("Mensaje recibido:");
      Serial.println(message);
      
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, message);
      
      if (error) {
        Serial.print("Error parseando JSON: ");
        Serial.println(error.c_str());
        return;
      }
      
      String type = doc["type"];
      
      if (type == "registration_response") {
        handleRegistrationResponse(doc);
      } else if (type == "heartbeat_ack") {
        handleHeartbeatAck(doc);
      } else if (type == "publish") {
        handlePublishMessage(doc);
      } else if (type == "system") {
        handleSystemMessage(doc);
      }
    }
  }
}

void handleRegistrationResponse(JsonDocument& doc) {
  String status = doc["status"];
  String message = doc["message"];
  
  Serial.print("Registro: ");
  Serial.print(status);
  Serial.print(" - ");
  Serial.println(message);
  
  if (status == "success") {
    digitalWrite(LED_PIN, HIGH);  // LED encendido = registrado
  } else {
    digitalWrite(LED_PIN, LOW);   // LED apagado = error
  }
}

void handleHeartbeatAck(JsonDocument& doc) {
  Serial.println("Heartbeat confirmado");
  
  // Parpadear LED para indicar comunicación activa
  digitalWrite(LED_PIN, LOW);
  delay(50);
  digitalWrite(LED_PIN, HIGH);
}

void handlePublishMessage(JsonDocument& doc) {
  String topic = doc["topic"];
  
  // Verificar si es un comando para este módulo
  if (topic.startsWith("deposito/" + MODULE_TYPE + "/cmd/")) {
    String command = topic.substring(topic.lastIndexOf('/') + 1);
    JsonObject payload = doc["payload"];
    
    Serial.print("Comando recibido: ");
    Serial.println(command);
    
    processCommand(command, payload);
  }
}

void handleSystemMessage(JsonDocument& doc) {
  String action = doc["action"];
  
  if (action == "welcome") {
    Serial.println("Bienvenida del broker recibida");
  }
}

void processCommand(String command, JsonObject payload) {
  // Procesar comandos específicos del sensor
  
  if (command == "get_status") {
    // Enviar estado actual
    sendStatusResponse();
    
  } else if (command == "calibrate") {
    // Simular calibración
    Serial.println("Calibrando sensor...");
    
    JsonDocument responseMsg;
    responseMsg["type"] = "publish";
    responseMsg["topic"] = "deposito/" + MODULE_TYPE + "/status/calibration";
    
    JsonObject responsePayload = responseMsg["payload"];
    responsePayload["module_id"] = MODULE_ID;
    responsePayload["status"] = "calibrated";
    responsePayload["timestamp"] = millis();
    
    String responseStr;
    serializeJson(responseMsg, responseStr);
    client.println(responseStr);
    
  } else if (command == "set_interval") {
    // Cambiar intervalo de envío (si se proporciona)
    if (payload.containsKey("interval")) {
      int newInterval = payload["interval"];
      if (newInterval >= 5000 && newInterval <= 60000) {
        // Actualizar intervalo (requeriría variable global)
        Serial.print("Nuevo intervalo: ");
        Serial.println(newInterval);
      }
    }
    
  } else {
    Serial.print("Comando no reconocido: ");
    Serial.println(command);
  }
}

void sendStatusResponse() {
  JsonDocument statusMsg;
  statusMsg["type"] = "publish";
  statusMsg["topic"] = "deposito/" + MODULE_TYPE + "/status/device";
  
  JsonObject payload = statusMsg["payload"];
  payload["module_id"] = MODULE_ID;
  payload["status"] = "online";
  payload["temperature"] = temperature;
  payload["humidity"] = humidity;
  payload["battery_level"] = batteryLevel;
  payload["uptime"] = millis();
  payload["free_memory"] = ESP.getFreeHeap();
  payload["wifi_rssi"] = WiFi.RSSI();
  payload["timestamp"] = millis();
  
  String statusStr;
  serializeJson(statusMsg, statusStr);
  client.println(statusStr);
  
  Serial.println("Estado enviado");
}