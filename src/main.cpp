#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include "config.h"

// Declaraciones de funciones
void setupWiFi();
void setupSystem();
void handleNewConnections();
void processClientMessages();
void checkModuleHeartbeats();
void processSystemCommands();
void sendWelcomeMessage(int clientIndex);
void processMessage(int clientIndex, String message);
void handleModuleRegistration(int clientIndex, DynamicJsonDocument& doc);
void handleHeartbeat(int clientIndex, DynamicJsonDocument& doc);
void handlePublish(int clientIndex, DynamicJsonDocument& doc);
void handleSubscribe(int clientIndexvoid sendCommandToModule(const String& moduleId, const String& command) {
  // Buscar el módulo registrado
  if (registeredModules.find(moduleId) != registeredModules.end()) {
    // Buscar cliente conectado para ese módulo
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        // Crear mensaje JSONDocument& doc);
void handleConfiguration(int clientIndex, DynamicJsonDocument& doc);
void forwardMessage(int senderIndex, String message);
void forwardToSubscribers(String topic, String payload);
void printSystemStatus();
void printRegisteredModules();
void printConnectedClients();
String findModuleByType(const String& moduleType);
void sendCommandToModule(const String& moduleId, const String& command);

// Configuración WiFi Access Point (desde config.h)
const char* ap_ssid = AP_SSID;
const char* ap_password = AP_PASSWORD;

// IPs del Access Point
IPAddress ap_ip(AP_IP_ADDR);
IPAddress ap_gateway(AP_GATEWAY_ADDR);
IPAddress ap_subnet(AP_SUBNET_ADDR);

// WiFi cliente (opcional)
const char* sta_ssid = WIFI_SSID;
const char* sta_password = WIFI_PASSWORD;

// Configuración MQTT Broker
WiFiServer mqttServer(MQTT_PORT);
WiFiClient mqttClients[MAX_CLIENTS];  
bool clientConnected[MAX_CLIENTS] = {false};

// Base de datos de módulos registrados
struct ModuleInfo {
  String moduleId;
  String moduleType;
  String capabilities;
  unsigned long lastHeartbeat;
  bool isActive;
};

std::map<String, ModuleInfo> registeredModules;
std::vector<String> activeTopics;

// Sistema de configuración
struct SystemConfig {
  bool discoveryMode;
  int heartbeatInterval;
  std::vector<String> allowedModuleTypes;
};

SystemConfig config = {
  .discoveryMode = true,
  .heartbeatInterval = HEARTBEAT_INTERVAL,
  .allowedModuleTypes = {}  // Se inicializa en setup()
};

void setup() {
  // Inicializar Serial para ESP32-C3 con USB CDC
  Serial.begin(SERIAL_BAUDRATE);
  
  // Esperar a que se establezca la conexión USB CDC
  while (!Serial && millis() < 5000) {
    delay(10);
  }
  
  delay(500);  // Estabilización adicional
  
  Serial.println();
  Serial.println("==============================================");
  Serial.println("🚀 INICIANDO BROKER MQTT ESP32-C3 SuperMini");
  Serial.println("==============================================");
  Serial.print("💾 RAM libre: ");
  Serial.println(ESP.getFreeHeap());
  Serial.print("🔧 Versión ESP-IDF: ");
  Serial.println(ESP.getSdkVersion());
  
  // Inicializar tipos de módulos permitidos
  for (int i = 0; i < NUM_ALLOWED_TYPES; i++) {
    config.allowedModuleTypes.push_back(String(ALLOWED_MODULE_TYPES[i]));
  }
  
  // Configurar WiFi
  setupWiFi();
  
  // Inicializar servidor MQTT
  mqttServer.begin();
  Serial.print("Servidor MQTT iniciado en puerto ");
  Serial.println(MQTT_PORT);
  
  // Configurar sistema
  setupSystem();
  
  Serial.println("Sistema listo. Esperando conexiones...");
}

void loop() {
  // Manejar nuevas conexiones
  handleNewConnections();
  
  // Procesar mensajes de clientes conectados
  processClientMessages();
  
  // Verificar heartbeats de módulos
  checkModuleHeartbeats();
  
  // Procesar comandos del sistema
  processSystemCommands();
  
  delay(10);
}

void setupWiFi() {
  Serial.println("Configurando ESP32-C3 como Access Point...");
  
  // Configurar como Access Point
  WiFi.mode(ENABLE_STATION_MODE ? WIFI_AP_STA : WIFI_AP);
  
  // Configurar IP estática para el AP
  if (!WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet)) {
    Serial.println("Error configurando IP del AP");
  }
  
  // Iniciar Access Point
  if (WiFi.softAP(ap_ssid, ap_password)) {
    Serial.println("✅ Access Point iniciado exitosamente!");
    Serial.print("📶 SSID: ");
    Serial.println(ap_ssid);
    Serial.print("🔑 Password: ");
    Serial.println(ap_password);
    Serial.print("🌐 IP del broker: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("📡 MAC Address: ");
    Serial.println(WiFi.softAPmacAddress());
  } else {
    Serial.println("❌ Error iniciando Access Point");
    return;
  }
  
  // Opcional: conectar también a WiFi externo (modo híbrido)
  if (ENABLE_STATION_MODE) {
    Serial.print("Conectando también a WiFi externo: ");
    Serial.println(sta_ssid);
    
    WiFi.begin(sta_ssid, sta_password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("✅ También conectado a WiFi externo!");
      Serial.print("🌍 IP externa: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println();
      Serial.println("⚠️ No se pudo conectar a WiFi externo (solo modo AP)");
    }
  }
  
  Serial.println("🚀 Sistema WiFi listo!");
}

void setupSystem() {
  // Publicar información del sistema
  DynamicJsonDocument systemInfo(1024);
  systemInfo["broker_id"] = "esp32c3_broker_001";
  systemInfo["version"] = "1.0.0";
  systemInfo["ap_ip"] = WiFi.softAPIP().toString();
  if (ENABLE_STATION_MODE && WiFi.status() == WL_CONNECTED) {
    systemInfo["sta_ip"] = WiFi.localIP().toString();
  }
  systemInfo["port"] = MQTT_PORT;
  systemInfo["discovery_mode"] = config.discoveryMode;
  systemInfo["timestamp"] = millis();
  
  String systemInfoStr;
  serializeJson(systemInfo, systemInfoStr);
  Serial.println("Sistema configurado:");
  Serial.println(systemInfoStr);
}

void handleNewConnections() {
  WiFiClient newClient = mqttServer.available();
  
  if (newClient) {
    // Buscar slot disponible
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (!clientConnected[i]) {
        mqttClients[i] = newClient;
        clientConnected[i] = true;
        
        Serial.print("Nuevo cliente conectado en slot ");
        Serial.println(i);
        
        // Enviar mensaje de bienvenida
        sendWelcomeMessage(i);
        break;
      }
    }
  }
}

void sendWelcomeMessage(int clientIndex) {
  DynamicJsonDocument welcome(512);
  welcome["type"] = "system";
  welcome["action"] = "welcome";
  welcome["broker_id"] = "esp32c3_broker_001";
  welcome["timestamp"] = millis();
  welcome["message"] = "Conectado al broker. Envía 'register' para registrar módulo.";
  
  String welcomeStr;
  serializeJson(welcome, welcomeStr);
  
  mqttClients[clientIndex].println(welcomeStr);
}

void processClientMessages() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i] && mqttClients[i].available()) {
      String message = mqttClients[i].readStringUntil('\n');
      message.trim();
      
      if (message.length() > 0) {
        processMessage(i, message);
      }
    }
    
    // Verificar si el cliente sigue conectado
    if (clientConnected[i] && !mqttClients[i].connected()) {
      Serial.print("Cliente ");
      Serial.print(i);
      Serial.println(" desconectado");
      clientConnected[i] = false;
    }
  }
}

void processMessage(int clientIndex, String message) {
  Serial.print("Cliente ");
  Serial.print(clientIndex);
  Serial.print(" envió: ");
  Serial.println(message);
  
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.print("Error parseando JSON: ");
    Serial.println(error.c_str());
    return;
  }
  
  String type = doc["type"];
  String action = doc["action"];
  
  if (type == "register") {
    handleModuleRegistration(clientIndex, doc);
  } else if (type == "heartbeat") {
    handleHeartbeat(clientIndex, doc);
  } else if (type == "publish") {
    handlePublish(clientIndex, doc);
  } else if (type == "subscribe") {
    handleSubscribe(clientIndex, doc);
  } else if (type == "config") {
    handleConfiguration(clientIndex, doc);
  } else {
    // Reenviar mensaje a suscriptores
    forwardMessage(clientIndex, message);
  }
}

void handleModuleRegistration(int clientIndex, DynamicJsonDocument& doc) {
  String moduleId = doc["module_id"];
  String moduleType = doc["module_type"];
  String capabilities = doc["capabilities"];
  
  Serial.print("Registrando módulo: ");
  Serial.print(moduleId);
  Serial.print(" tipo: ");
  Serial.println(moduleType);
  
  // Verificar si el tipo de módulo está permitido
  bool typeAllowed = false;
  for (const String& allowedType : config.allowedModuleTypes) {
    if (moduleType == allowedType) {
      typeAllowed = true;
      break;
    }
  }
  
  DynamicJsonDocument response(512);
  response["type"] = "registration_response";
  response["module_id"] = moduleId;
  
  if (typeAllowed) {
    // Registrar módulo
    ModuleInfo moduleInfo;
    moduleInfo.moduleId = moduleId;
    moduleInfo.moduleType = moduleType;
    moduleInfo.capabilities = capabilities;
    moduleInfo.lastHeartbeat = millis();
    moduleInfo.isActive = true;
    
    registeredModules[moduleId] = moduleInfo;
    
    response["status"] = "success";
    response["message"] = "Módulo registrado exitosamente";
    
    Serial.print("Módulo ");
    Serial.print(moduleId);
    Serial.println(" registrado exitosamente");
  } else {
    response["status"] = "error";
    response["message"] = "Tipo de módulo no permitido";
    
    Serial.print("Tipo de módulo no permitido: ");
    Serial.println(moduleType);
  }
  
  response["timestamp"] = millis();
  
  String responseStr;
  serializeJson(response, responseStr);
  mqttClients[clientIndex].println(responseStr);
}

void handleHeartbeat(int clientIndex, DynamicJsonDocument& doc) {
  String moduleId = doc["module_id"];
  
  if (registeredModules.find(moduleId) != registeredModules.end()) {
    registeredModules[moduleId].lastHeartbeat = millis();
    
    // Responder heartbeat
    DynamicJsonDocument response(256);
    response["type"] = "heartbeat_ack";
    response["module_id"] = moduleId;
    response["timestamp"] = millis();
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
  }
}

void handlePublish(int clientIndex, DynamicJsonDocument& doc) {
  String topic = doc["topic"];
  String payload = doc["payload"];
  
  // Reenviar a todos los suscriptores del topic
  forwardToSubscribers(topic, payload);
}

void handleSubscribe(int clientIndex, DynamicJsonDocument& doc) {
  String topic = doc["topic"];
  
  // Agregar cliente a suscriptores (implementación simple)
  Serial.print("Cliente ");
  Serial.print(clientIndex);
  Serial.print(" suscrito a: ");
  Serial.println(topic);
}

void handleConfiguration(int clientIndex, DynamicJsonDocument& doc) {
  String configType = doc["config_type"];
  
  if (configType == "get_modules") {
    // Enviar lista de módulos registrados
    StaticJsonDocument<1024> response;
    response["type"] = "config_response";
    response["config_type"] = "modules_list";
    
    JsonArray modules = response["modules"];
    for (auto& pair : registeredModules) {
      JsonObject module = modules.add<JsonObject>();
      module["module_id"] = pair.second.moduleId;
      module["module_type"] = pair.second.moduleType;
      module["capabilities"] = pair.second.capabilities;
      module["is_active"] = pair.second.isActive;
      module["last_heartbeat"] = pair.second.lastHeartbeat;
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
  } else if (configType == "set_discovery") {
    config.discoveryMode = doc["value"];
    
    StaticJsonDocument<1024> response;
    response["type"] = "config_response";
    response["config_type"] = "discovery_mode";
    response["status"] = "success";
    response["value"] = config.discoveryMode;
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
  }
}

void forwardMessage(int senderIndex, String message) {
  // Reenviar mensaje a todos los demás clientes conectados
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (i != senderIndex && clientConnected[i]) {
      mqttClients[i].println(message);
    }
  }
}

void forwardToSubscribers(String topic, String payload) {
  // Crear mensaje de publicación
  StaticJsonDocument<1024> pubMessage;
  pubMessage["type"] = "publish";
  pubMessage["topic"] = topic;
  pubMessage["payload"] = payload;
  pubMessage["timestamp"] = millis();
  
  String pubMessageStr;
  serializeJson(pubMessage, pubMessageStr);
  
  // Enviar a todos los clientes conectados (implementación simple)
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i]) {
      mqttClients[i].println(pubMessageStr);
    }
  }
}

void checkModuleHeartbeats() {
  unsigned long currentTime = millis();
  
  for (auto& pair : registeredModules) {
    if (pair.second.isActive) {
      unsigned long timeSinceHeartbeat = currentTime - pair.second.lastHeartbeat;
      
      if (timeSinceHeartbeat > config.heartbeatInterval * 2) {
        // Módulo no responde
        pair.second.isActive = false;
        
        Serial.print("Módulo ");
        Serial.print(pair.second.moduleId);
        Serial.println(" marcado como inactivo (sin heartbeat)");
        
        // Notificar a clientes
        StaticJsonDocument<1024> notification;
        notification["type"] = "module_status";
        notification["module_id"] = pair.second.moduleId;
        notification["status"] = "offline";
        notification["timestamp"] = currentTime;
        
        String notificationStr;
        serializeJson(notification, notificationStr);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (clientConnected[i]) {
            mqttClients[i].println(notificationStr);
          }
        }
      }
    }
  }
}

void processSystemCommands() {
  // Procesar comandos serie para configuración
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "status") {
      printSystemStatus();
    } else if (command == "modules") {
      printRegisteredModules();
    } else if (command == "clients") {
      printConnectedClients();
    } else if (command == "reset") {
      Serial.println("🔄 Reiniciando ESP32-C3 en 3 segundos...");
      delay(3000);
      ESP.restart();
    } else if (command.startsWith("server:")) {
      // Comandos cortos del servidor
      String shortCommand = command.substring(7); // Remover "server:"
      
      if (shortCommand == "scan_fingerprint") {
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          sendCommandToModule(fingerprintId, "scan_fingerprint");
          Serial.println("🔍 Iniciando escaneo de huella...");
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else if (shortCommand == "status_fingerprint") {
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          sendCommandToModule(fingerprintId, "get_status");
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else if (shortCommand == "info_fingerprint") {
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          sendCommandToModule(fingerprintId, "get_device_info");
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else {
        Serial.println("❌ Comandos server: scan_fingerprint, status_fingerprint, info_fingerprint");
      }
    } else if (command.startsWith("discovery ")) {
      String mode = command.substring(10);
      config.discoveryMode = (mode == "on");
      Serial.print("Modo discovery: ");
      Serial.println(config.discoveryMode ? "ON" : "OFF");
    } else if (command.startsWith("send ")) {
      // Comando: send <module_id> <command>
      // Ejemplo: send fingerprint_4b224f7c630 scan_fingerprint
      String params = command.substring(5); // Remover "send "
      int spaceIndex = params.indexOf(' ');
      
      if (spaceIndex > 0) {
        String moduleId = params.substring(0, spaceIndex);
        String moduleCommand = params.substring(spaceIndex + 1);
        
        // Buscar el módulo registrado
        if (registeredModules.find(moduleId) != registeredModules.end()) {
          // Buscar cliente conectado para ese módulo
          for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientConnected[i]) {
              // Enviar comando al cliente
              DynamicJsonDocument cmdMsg(200);
              cmdMsg["type"] = "command";
              cmdMsg["module_id"] = moduleId;
              cmdMsg["command"] = moduleCommand;
              cmdMsg["timestamp"] = millis();
              
              String cmdStr;
              serializeJson(cmdMsg, cmdStr);
              
              mqttClients[i].println(cmdStr);
              
              Serial.print("📨 Comando enviado a ");
              Serial.print(moduleId);
              Serial.print(": ");
              Serial.println(moduleCommand);
              break;
            }
          }
        } else {
          Serial.println("❌ Módulo no encontrado: " + moduleId);
        }
      } else {
        Serial.println("❌ Formato: send <module_id> <command>");
        Serial.println("💡 Ejemplo: send fingerprint_4b224f7c630 scan_fingerprint");
      }
    } else if (command == "help") {
      Serial.println("=== COMANDOS DISPONIBLES ===");
      Serial.println("status     - Estado del sistema");
      Serial.println("modules    - Listar módulos registrados");
      Serial.println("clients    - Listar clientes conectados");
      Serial.println("send <id> <cmd> - Enviar comando a módulo");
      Serial.println("discovery on/off - Cambiar modo discovery");
      Serial.println("reset      - Reiniciar servidor");
      Serial.println("help       - Mostrar esta ayuda");
      Serial.println("");
      Serial.println("=== COMANDOS RÁPIDOS ===");
      Serial.println("server:scan_fingerprint  - Escanear huella");
      Serial.println("server:status_fingerprint - Estado de huella");
      Serial.println("server:info_fingerprint  - Info de huella");
    }
  }
}

void printSystemStatus() {
  Serial.println("=== ESTADO DEL SISTEMA ===");
  Serial.print("📶 SSID AP: ");
  Serial.println(ap_ssid);
  Serial.print("🌐 IP AP: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("👥 Clientes AP: ");
  Serial.println(WiFi.softAPgetStationNum());
  
  if (ENABLE_STATION_MODE) {
    Serial.print("🌍 WiFi externo: ");
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Conectado - IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Desconectado");
    }
  }
  
  Serial.print("🔗 Clientes MQTT: ");
  int connectedCount = 0;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i]) connectedCount++;
  }
  Serial.println(connectedCount);
  Serial.print("📦 Módulos registrados: ");
  Serial.println(registeredModules.size());
  Serial.print("🔍 Modo discovery: ");
  Serial.println(config.discoveryMode ? "ON" : "OFF");
  Serial.print("💾 RAM libre: ");
  Serial.print(ESP.getFreeHeap());
  Serial.println(" bytes");
  Serial.println("==========================");
}

void printRegisteredModules() {
  Serial.println("=== MÓDULOS REGISTRADOS ===");
  for (auto& pair : registeredModules) {
    Serial.print("ID: ");
    Serial.print(pair.second.moduleId);
    Serial.print(" | Tipo: ");
    Serial.print(pair.second.moduleType);
    Serial.print(" | Activo: ");
    Serial.print(pair.second.isActive ? "SÍ" : "NO");
    Serial.print(" | Último heartbeat: ");
    Serial.println(pair.second.lastHeartbeat);
  }
  Serial.println("==========================");
}

void printConnectedClients() {
  Serial.println("=== CLIENTES CONECTADOS ===");
  for (int i = 0; i < 10; i++) {
    if (clientConnected[i]) {
      Serial.print("Slot ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(mqttClients[i].remoteIP());
    }
  }
  Serial.println("===========================");
}

String findModuleByType(const String& moduleType) {
  // Buscar el primer módulo activo del tipo especificado
  for (auto& pair : registeredModules) {
    if (pair.second.moduleType == moduleType && pair.second.isActive) {
      return pair.first; // Retornar el moduleId
    }
  }
  return ""; // No encontrado
}

void sendCommandToModule(const String& moduleId, const String& command) {
  Serial.println("🔧 DEBUG sendCommandToModule: " + moduleId + " -> " + command);
  
  // Buscar el módulo registrado
  if (registeredModules.find(moduleId) != registeredModules.end()) {
    Serial.println("🔧 DEBUG: Módulo encontrado en registro");
    
    // Buscar cliente conectado para ese módulo
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        Serial.println("🔧 DEBUG: Enviando a cliente " + String(i));
        
        // Enviar comando al cliente
        DynamicJsonDocument cmdMsg(200);
        cmdMsg["type"] = "command";
        cmdMsg["module_id"] = moduleId;
        cmdMsg["command"] = command;
        cmdMsg["timestamp"] = millis();
        
        String cmdStr;
        serializeJson(cmdMsg, cmdStr);
        
        mqttClients[i].println(cmdStr);
        
        Serial.print("📨 Comando enviado a ");
        Serial.print(moduleId);
        Serial.print(": ");
        Serial.println(command);
        break;
      }
    }
  } else {
    Serial.println("❌ Módulo no encontrado: " + moduleId);
  }
}