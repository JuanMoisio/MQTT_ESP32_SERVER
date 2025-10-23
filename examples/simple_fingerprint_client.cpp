/*
 * CLIENTE FINGERPRINT SIMPLE ACTUALIZADO
 * Responde correctamente al discovery del broker
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Configuración de red
const char* ssid = "DEPOSITO_BROKER";
const char* password = "deposito123";
const char* brokerIP = "192.168.4.1";
const int brokerPort = 1883;

// Configuración del dispositivo
const String DEVICE_MAC = WiFi.macAddress();
const String DEVICE_TYPE = "fingerprint_scanner";
const String MODULE_ID = "fingerprint_" + WiFi.macAddress().substring(12).replace(":", "");

WiFiClient client;
bool connectedToBroker = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("==============================================");
  Serial.println("🔍 FINGERPRINT CLIENT - DISCOVERY SUPPORT");
  Serial.println("==============================================");
  Serial.println("📱 MAC: " + DEVICE_MAC);
  Serial.println("🔧 Module ID: " + MODULE_ID);
  
  setupWiFi();
  connectToBroker();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    setupWiFi();
    return;
  }
  
  if (!connectedToBroker || !client.connected()) {
    connectToBroker();
    return;
  }
  
  processMessages();
  
  if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
    sendHeartbeat();
    lastHeartbeat = millis();
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
  Serial.println("✅ WiFi conectado! IP: " + WiFi.localIP().toString());
}

void connectToBroker() {
  Serial.println("🔗 Conectando al broker...");
  
  if (client.connect(brokerIP, brokerPort)) {
    Serial.println("✅ Conectado al broker!");
    connectedToBroker = true;
    
    // Auto-registrar (si es necesario)
    registerModule();
  } else {
    Serial.println("❌ Error conectando al broker");
    connectedToBroker = false;
    delay(5000);
  }
}

void registerModule() {
  StaticJsonDocument<300> registerMsg;
  registerMsg["type"] = "register";
  registerMsg["module_id"] = MODULE_ID;
  registerMsg["module_type"] = DEVICE_TYPE;
  registerMsg["mac_address"] = DEVICE_MAC;
  registerMsg["capabilities"] = "scan,enroll,delete,list";
  registerMsg["timestamp"] = millis();
  
  String registerStr;
  serializeJson(registerMsg, registerStr);
  
  client.println(registerStr);
  Serial.println("📤 Registro enviado");
}

void sendHeartbeat() {
  StaticJsonDocument<200> heartbeatMsg;
  heartbeatMsg["type"] = "heartbeat";
  heartbeatMsg["module_id"] = MODULE_ID;
  heartbeatMsg["timestamp"] = millis();
  
  String heartbeatStr;
  serializeJson(heartbeatMsg, heartbeatStr);
  
  client.println(heartbeatStr);
  Serial.println("💓 Heartbeat enviado");
}

void processMessages() {
  if (client.available()) {
    String message = client.readStringUntil('\n');
    message.trim();
    
    if (message.length() > 0) {
      Serial.println("📨 Mensaje recibido: " + message);
      
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, message);
      
      if (error) {
        Serial.println("❌ Error parseando JSON: " + String(error.c_str()));
        return;
      }
      
      String type = doc["type"];
      
      if (type == "device_discovery") {
        handleDeviceDiscovery(doc);
      } else if (type == "registration_response") {
        handleRegistrationResponse(doc);
      } else if (type == "heartbeat_ack") {
        Serial.println("💓 Heartbeat confirmado");
      } else if (type == "command") {
        handleCommand(doc);
      }
    }
  }
}

void handleDeviceDiscovery(const StaticJsonDocument<1024>& doc) {
  Serial.println("🔍 DISCOVERY REQUEST recibido!");
  
  String action = doc["action"];
  String expectedResponse = doc["expected_response"];
  
  if (action == "request_info" && expectedResponse == "device_info_response") {
    Serial.println("📡 Enviando device_info_response...");
    
    // Responder con device_info_response
    StaticJsonDocument<400> response;
    response["type"] = "device_info_response";
    response["module_id"] = MODULE_ID;
    response["mac_address"] = DEVICE_MAC;
    response["device_type"] = DEVICE_TYPE;
    response["capabilities"] = "scan,enroll,delete,list";
    response["status"] = "online";
    response["ip_address"] = WiFi.localIP().toString();
    response["timestamp"] = millis();
    
    String responseStr;
    serializeJson(response, responseStr);
    
    client.println(responseStr);
    Serial.println("✅ Respuesta enviada: " + responseStr);
  }
}

void handleRegistrationResponse(const StaticJsonDocument<1024>& doc) {
  String status = doc["status"];
  String message = doc["message"];
  
  Serial.println("📋 Registro: " + status + " - " + message);
}

void handleCommand(const StaticJsonDocument<1024>& doc) {
  String command = doc["command"];
  String moduleId = doc["module_id"];
  
  if (moduleId != MODULE_ID) {
    return; // No es para este módulo
  }
  
  Serial.println("⚡ Comando: " + command);
  
  if (command == "scan_fingerprint") {
    simulateFingerprintScan();
  } else if (command.startsWith("enroll_user:")) {
    String userName = command.substring(12);
    simulateEnrollment(userName);
  } else if (command == "list_all_fingerprints") {
    sendFingerprintList();
  }
}

void simulateFingerprintScan() {
  Serial.println("👆 Simulando escaneo de huella...");
  
  // Simular resultado aleatorio
  bool success = (random(0, 10) < 7); // 70% éxito
  
  StaticJsonDocument<300> result;
  result["type"] = "fingerprint_scan_result";
  result["module_id"] = MODULE_ID;
  result["success"] = success;
  result["timestamp"] = millis();
  
  if (success) {
    int userId = random(1, 6); // Usuario 1-5
    result["user_id"] = userId;
    result["user_name"] = "Usuario" + String(userId);
    result["confidence"] = random(85, 100);
    Serial.println("✅ Huella reconocida: Usuario" + String(userId));
  } else {
    result["error"] = "Huella no reconocida";
    Serial.println("❌ Huella no reconocida");
  }
  
  String resultStr;
  serializeJson(result, resultStr);
  client.println(resultStr);
}

void simulateEnrollment(const String& userName) {
  Serial.println("👤 Simulando enrolamiento: " + userName);
  
  delay(2000); // Simular tiempo de enrolamiento
  
  StaticJsonDocument<300> result;
  result["type"] = "fingerprint_enroll_result";
  result["module_id"] = MODULE_ID;
  result["success"] = true;
  result["user_name"] = userName;
  result["user_id"] = random(10, 100);
  result["timestamp"] = millis();
  
  String resultStr;
  serializeJson(result, resultStr);
  client.println(resultStr);
  
  Serial.println("✅ Enrolamiento completado");
}

void sendFingerprintList() {
  Serial.println("📋 Enviando lista de huellas...");
  
  StaticJsonDocument<500> list;
  list["type"] = "fingerprint_list";
  list["module_id"] = MODULE_ID;
  
  JsonArray users = list.createNestedArray("users");
  
  // Simular algunos usuarios registrados
  for (int i = 1; i <= 5; i++) {
    JsonObject user = users.createNestedObject();
    user["id"] = i;
    user["name"] = "Usuario" + String(i);
    user["registered_at"] = millis() - (i * 86400000); // Hace i días
  }
  
  list["total_users"] = 5;
  list["timestamp"] = millis();
  
  String listStr;
  serializeJson(list, listStr);
  client.println(listStr);
  
  Serial.println("📤 Lista enviada");
}