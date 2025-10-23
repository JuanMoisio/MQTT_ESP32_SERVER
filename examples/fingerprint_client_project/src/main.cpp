/*
 * CLIENTE FINGERPRINT SIMPLE ACTUALIZADO
 * Responde correctamente al discovery del broker
 * 
 * PARA PROGRAMAR EN: /dev/cu.usbserial-1110 (ESP32-WROOM)
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
String DEVICE_MAC;
const String DEVICE_TYPE = "fingerprint_scanner";
String MODULE_ID;

WiFiClient client;
bool connectedToBroker = false;
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Obtener MAC real del dispositivo
  DEVICE_MAC = WiFi.macAddress();
  MODULE_ID = "fingerprint_" + DEVICE_MAC.substring(12);
  MODULE_ID.replace(":", "");
  
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
    Serial.println("📡 WiFi desconectado, reconectando...");
    setupWiFi();
    return;
  }
  
  if (!connectedToBroker || !client.connected()) {
    Serial.println("🔗 Broker desconectado, reconectando...");
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
  Serial.print("📡 Conectando a WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("✅ WiFi conectado! IP: " + WiFi.localIP().toString());
  } else {
    Serial.println();
    Serial.println("❌ Error conectando a WiFi");
    delay(5000);
  }
}

void connectToBroker() {
  Serial.println("🔗 Conectando al broker " + String(brokerIP) + ":" + String(brokerPort));
  
  if (client.connect(brokerIP, brokerPort)) {
    Serial.println("✅ Conectado al broker!");
    connectedToBroker = true;
    
    // Auto-registrar módulo
    registerModule();
  } else {
    Serial.println("❌ Error conectando al broker");
    connectedToBroker = false;
    delay(5000);
  }
}

void registerModule() {
  StaticJsonDocument<400> registerMsg;
  registerMsg["type"] = "register";
  registerMsg["module_id"] = MODULE_ID;
  registerMsg["module_type"] = DEVICE_TYPE;
  registerMsg["mac_address"] = DEVICE_MAC;
  registerMsg["capabilities"] = "scan,enroll,delete,list";
  registerMsg["timestamp"] = millis();
  
  String registerStr;
  serializeJson(registerMsg, registerStr);
  
  client.println(registerStr);
  Serial.println("📤 Mensaje de registro enviado:");
  Serial.println("   " + registerStr);
}

void sendHeartbeat() {
  StaticJsonDocument<200> heartbeatMsg;
  heartbeatMsg["type"] = "heartbeat";
  heartbeatMsg["module_id"] = MODULE_ID;
  heartbeatMsg["timestamp"] = millis();
  
  String heartbeatStr;
  serializeJson(heartbeatMsg, heartbeatStr);
  
  client.println(heartbeatStr);
  Serial.println("💓 Heartbeat enviado (" + String(millis()) + ")");
}

void processMessages() {
  if (client.available()) {
    String message = client.readStringUntil('\n');
    message.trim();
    
    if (message.length() > 0) {
      Serial.println("📨 Mensaje recibido:");
      Serial.println("   " + message);
      
      StaticJsonDocument<1024> doc;
      DeserializationError error = deserializeJson(doc, message);
      
      if (error) {
        Serial.println("❌ Error parseando JSON: " + String(error.c_str()));
        return;
      }
      
      String type = doc["type"];
      Serial.println("🔧 Tipo de mensaje: " + type);
      
      if (type == "device_discovery") {
        handleDeviceDiscovery(doc);
      } else if (type == "registration_response") {
        handleRegistrationResponse(doc);
      } else if (type == "heartbeat_ack") {
        Serial.println("💓 Heartbeat confirmado por broker");
      } else if (type == "command") {
        handleCommand(doc);
      } else if (type == "system") {
        handleSystemMessage(doc);
      } else {
        Serial.println("❓ Tipo de mensaje no manejado: " + type);
      }
    }
  }
}

void handleDeviceDiscovery(const StaticJsonDocument<1024>& doc) {
  Serial.println("🔍 ¡DISCOVERY REQUEST RECIBIDO!");
  
  String action = doc["action"];
  String expectedResponse = doc["expected_response"];
  
  Serial.println("   Action: " + action);
  Serial.println("   Expected Response: " + expectedResponse);
  
  if (action == "request_info" && expectedResponse == "device_info_response") {
    Serial.println("📡 Enviando device_info_response...");
    
    // Responder con device_info_response
    StaticJsonDocument<500> response;
    response["type"] = "device_info_response";
    response["module_id"] = MODULE_ID;
    response["mac_address"] = DEVICE_MAC;
    response["device_type"] = DEVICE_TYPE;
    response["capabilities"] = "scan,enroll,delete,list";
    response["status"] = "online";
    response["ip_address"] = WiFi.localIP().toString();
    response["timestamp"] = millis();
    response["firmware_version"] = "1.0.0";
    
    String responseStr;
    serializeJson(response, responseStr);
    
    client.println(responseStr);
    Serial.println("✅ device_info_response enviado:");
    Serial.println("   " + responseStr);
  } else {
    Serial.println("⚠️ Discovery request no coincide con lo esperado");
  }
}

void handleRegistrationResponse(const StaticJsonDocument<1024>& doc) {
  String status = doc["status"];
  String message = doc["message"];
  
  Serial.println("📋 Respuesta de registro:");
  Serial.println("   Status: " + status);
  Serial.println("   Message: " + message);
  
  if (status == "success") {
    Serial.println("🎉 ¡Módulo registrado exitosamente!");
  } else {
    Serial.println("❌ Error en registro del módulo");
  }
}

void handleSystemMessage(const StaticJsonDocument<1024>& doc) {
  String action = doc["action"];
  
  if (action == "welcome") {
    Serial.println("👋 Mensaje de bienvenida del broker recibido");
  }
}

void handleCommand(const StaticJsonDocument<1024>& doc) {
  String command = doc["command"];
  String moduleId = doc["module_id"];
  
  if (moduleId != MODULE_ID) {
    Serial.println("📨 Comando para otro módulo: " + moduleId);
    return;
  }
  
  Serial.println("⚡ Comando recibido: " + command);
  
  if (command == "scan_fingerprint") {
    simulateFingerprintScan();
  } else if (command.startsWith("enroll_user:")) {
    String userName = command.substring(12);
    simulateEnrollment(userName);
  } else if (command == "list_all_fingerprints") {
    sendFingerprintList();
  } else if (command.startsWith("delete_user:")) {
    String userIdStr = command.substring(12);
    int userId = userIdStr.toInt();
    simulateUserDeletion(userId);
  } else {
    Serial.println("❓ Comando no reconocido: " + command);
  }
}

void simulateFingerprintScan() {
  Serial.println("👆 Simulando escaneo de huella...");
  
  delay(1000); // Simular tiempo de escaneo
  
  // Simular resultado aleatorio
  bool success = (random(0, 10) < 7); // 70% éxito
  
  StaticJsonDocument<400> result;
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
  
  Serial.println("📤 Resultado enviado: " + resultStr);
}

void simulateEnrollment(const String& userName) {
  Serial.println("👤 Simulando enrolamiento de: " + userName);
  
  delay(3000); // Simular tiempo de enrolamiento
  
  int newUserId = random(10, 100);
  
  StaticJsonDocument<400> result;
  result["type"] = "fingerprint_enroll_result";
  result["module_id"] = MODULE_ID;
  result["success"] = true;
  result["user_name"] = userName;
  result["user_id"] = newUserId;
  result["message"] = "Usuario enrolado exitosamente";
  result["timestamp"] = millis();
  
  String resultStr;
  serializeJson(result, resultStr);
  client.println(resultStr);
  
  Serial.println("✅ Enrolamiento completado - ID: " + String(newUserId));
  Serial.println("📤 Resultado enviado: " + resultStr);
}

void simulateUserDeletion(int userId) {
  Serial.println("🗑️ Simulando eliminación de usuario ID: " + String(userId));
  
  delay(500); // Simular tiempo de eliminación
  
  StaticJsonDocument<400> result;
  result["type"] = "fingerprint_delete_result";
  result["module_id"] = MODULE_ID;
  result["success"] = true;
  result["user_id"] = userId;
  result["message"] = "Usuario eliminado exitosamente";
  result["timestamp"] = millis();
  
  String resultStr;
  serializeJson(result, resultStr);
  client.println(resultStr);
  
  Serial.println("✅ Usuario eliminado - ID: " + String(userId));
  Serial.println("📤 Resultado enviado: " + resultStr);
}

void sendFingerprintList() {
  Serial.println("📋 Enviando lista de huellas registradas...");
  
  StaticJsonDocument<800> list;
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
  
  Serial.println("📤 Lista enviada con 5 usuarios simulados");
  Serial.println("   " + listStr);
}