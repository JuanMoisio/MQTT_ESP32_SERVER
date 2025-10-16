#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <mbedtls/md.h>
#include <map>
#include <vector>
#include "config.h"
#include "web_interface.h"

// Declaraciones de funciones
void setupWiFi();
void setupSystem();
void handleNewConnections();
void processClientMessages();
void checkModuleHeartbeats();
void processSystemCommands();
void sendWelcomeMessage(int clientIndex);
void processMessage(int clientIndex, String message);
void handleModuleRegistration(int clientIndex, JsonDocument& doc);
void handleHeartbeat(int clientIndex, JsonDocument& doc);
void handlePublish(int clientIndex, JsonDocument& doc);
void handleSubscribe(int clientIndex, JsonDocument& doc);
void handleConfiguration(int clientIndex, JsonDocument& doc);
void handleMACResponse(int clientIndex, JsonDocument& doc);
void handleDeviceRegistration(int clientIndex, JsonDocument& doc);
void forwardMessage(int senderIndex, String message);
void forwardToSubscribers(String topic, String payload);
void printSystemStatus();
void printRegisteredModules();
void printConnectedClients();
String findModuleByType(const String& moduleType);
void sendCommandToModule(const String& moduleId, const String& command);
String generateAPIKey();
String sha256Hash(const String& input);
void setupWebServer();
void handleWebLogin();
void handleWebAdmin();
void handleAddDevice();
void handleRemoveDevice();
bool authenticateDevice(const String& macAddress, const String& apiKey);
void loadAuthorizedDevices();
void saveAuthorizedDevices();

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

// Servidor Web para administración
WebServer webServer(80);

// Credenciales de administrador (cambiar en producción)
const String ADMIN_USER = "admin";
const String ADMIN_PASS = "deposito123";

// Sistema de registro de dispositivos
const String REGISTRATION_PASSWORD = "361500";  // Password para registro de dispositivos

// IDs de dispositivos por tipo
const int DEVICE_ID_SERVER = 1;
const int DEVICE_ID_TERMINAL = 10;
const int DEVICE_ID_FINGERPRINT = 20;
const int DEVICE_ID_SLAVE_1 = 21;
const int DEVICE_ID_SLAVE_2 = 22;
const int DEVICE_ID_SLAVE_3 = 23;

// Base de datos de módulos registrados
struct ModuleInfo {
  String moduleId;
  String moduleType;
  String capabilities;
  unsigned long lastHeartbeat;
  bool isActive;
  bool isAuthenticated;  // Nuevo campo para autenticación
  String macAddress;     // MAC del dispositivo
};

// Sistema de autenticación
struct AuthorizedDevice {
  String macAddress;      // "aa:bb:cc:dd:ee:ff"
  String deviceType;      // "fingerprint", "camera", "sensor"
  String apiKey;          // API key generada
  String description;     // Descripción del dispositivo
  bool isActive;          // Activo/Inactivo
  unsigned long lastSeen; // Última vez visto
  String currentIP;       // IP actual
};

std::map<String, ModuleInfo> registeredModules;
std::vector<String> activeTopics;
std::map<String, AuthorizedDevice> authorizedDevices;

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

// Variables para consulta de MAC
String lastRequestedMAC = "";
unsigned long macRequestTime = 0;

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
  
  // SPIFFS no necesario - HTML embebido
  Serial.println("✅ Sistema web configurado (HTML embebido)");
  
  // Cargar dispositivos autorizados
  loadAuthorizedDevices();
  
  // Configurar servidor web
  setupWebServer();
  
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
  
  // Manejar peticiones del servidor web
  webServer.handleClient();
  
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
  StaticJsonDocument<1024> systemInfo;
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
  StaticJsonDocument<512> welcome;
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
  
  JsonDocument doc;
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
  } else if (type == "mac_response") {
    handleMACResponse(clientIndex, doc);
  } else if (type == "device_registration") {
    handleDeviceRegistration(clientIndex, doc);
  } else if (type == "module_registration") {
    handleModuleRegistration(clientIndex, doc);
  } else {
    // Reenviar mensaje a suscriptores
    forwardMessage(clientIndex, message);
  }
}

void handleModuleRegistration(int clientIndex, JsonDocument& doc) {
  String moduleId = doc["module_id"];
  String moduleType = doc["module_type"];
  String capabilities = doc["capabilities"];
  String macAddress = doc["mac_address"];
  String apiKey = doc["api_key"];
  
  Serial.println("� Registro de módulo: " + moduleType);
  
  StaticJsonDocument<512> response;
  response["type"] = "registration_response";
  response["response_type"] = "module";
  response["module_id"] = moduleId;
  
  // Para clientes ya conectados, buscar si ya está autenticado
  bool isAuthenticated = false;
  String realApiKey = "";
  
  // Buscar en dispositivos autorizados por MAC
  for (auto& pair : authorizedDevices) {
    AuthorizedDevice& device = pair.second;
    if (device.isActive && device.macAddress == macAddress) {
      isAuthenticated = true;
      realApiKey = device.apiKey;
      Serial.println("✅ Dispositivo ya autenticado, usando API key existente");
      break;
    }
  }
  
  // Fallback: autenticar con la API key recibida
  if (!isAuthenticated) {
    isAuthenticated = authenticateDevice(macAddress, apiKey);
  }
  
  // DEBUG: Si no está autenticado, agregar automáticamente para fingerprint_scanner
  if (!isAuthenticated && moduleType == "fingerprint_scanner") {
    Serial.println("🔧 Auto-registrando dispositivo...");
    
    AuthorizedDevice newDevice;
    newDevice.macAddress = macAddress;
    newDevice.deviceType = "fingerprint";
    newDevice.apiKey = generateAPIKey();
    newDevice.description = "Scanner Auto-registrado";
    newDevice.isActive = true;
    newDevice.lastSeen = millis();
    newDevice.currentIP = mqttClients[clientIndex].remoteIP().toString();
    
    authorizedDevices[macAddress] = newDevice;
    
    Serial.println("✅ Dispositivo auto-registrado");
    
    // Actualizar el cliente para que use el nuevo API key
    isAuthenticated = true;
    
    // Enviar el API key correcto al cliente para futuras conexiones
    StaticJsonDocument<200> apiKeyMsg;
    apiKeyMsg["type"] = "api_key_update";
    apiKeyMsg["api_key"] = newDevice.apiKey;
    String apiKeyStr;
    serializeJson(apiKeyMsg, apiKeyStr);
    mqttClients[clientIndex].println(apiKeyStr);
  }
  
  // Verificar si el tipo de módulo está permitido
  bool typeAllowed = false;
  Serial.println("🔍 Verificando tipo de módulo: " + moduleType);
  Serial.println("🔍 Lista de tipos permitidos:");
  for (const String& allowedType : config.allowedModuleTypes) {
    Serial.println("  - " + allowedType);
    if (moduleType == allowedType) {
      typeAllowed = true;
      Serial.println("✅ Tipo encontrado: " + allowedType);
      break;
    }
  }
  
  Serial.println("🔍 isAuthenticated: " + String(isAuthenticated ? "true" : "false"));
  Serial.println("🔍 typeAllowed: " + String(typeAllowed ? "true" : "false"));
  
  if (isAuthenticated && typeAllowed) {
    // Registrar módulo
    ModuleInfo moduleInfo;
    moduleInfo.moduleId = moduleId;
    moduleInfo.moduleType = moduleType;
    moduleInfo.capabilities = capabilities;
    moduleInfo.lastHeartbeat = millis();
    moduleInfo.isActive = true;
    moduleInfo.isAuthenticated = true;
    moduleInfo.macAddress = macAddress;
    
    registeredModules[moduleId] = moduleInfo;
    
    // Actualizar IP del dispositivo autorizado
    if (authorizedDevices.find(macAddress) != authorizedDevices.end()) {
      authorizedDevices[macAddress].currentIP = mqttClients[clientIndex].remoteIP().toString();
      authorizedDevices[macAddress].lastSeen = millis();
    }
    
    response["status"] = "success";
    response["message"] = "Módulo registrado y autenticado exitosamente";
    
    Serial.print("✅ Módulo ");
    Serial.print(moduleId);
    Serial.println(" registrado y autenticado");
  } else {
    response["status"] = "error";
    
    if (!isAuthenticated) {
      response["message"] = "Dispositivo no autorizado";
      Serial.println("❌ Dispositivo no autorizado: " + macAddress);
    } else {
      response["message"] = "Tipo de módulo no permitido";
      Serial.println("❌ Tipo de módulo no permitido: " + moduleType);
    }
  }
  
  response["timestamp"] = millis();
  
  String responseStr;
  serializeJson(response, responseStr);
  Serial.println("📤 Enviando respuesta de módulo: " + responseStr);
  mqttClients[clientIndex].println(responseStr);
}

void handleHeartbeat(int clientIndex, JsonDocument& doc) {
  String moduleId = doc["module_id"];
  
  if (registeredModules.find(moduleId) != registeredModules.end()) {
    registeredModules[moduleId].lastHeartbeat = millis();
    
    // Responder heartbeat
    StaticJsonDocument<256> response;
    response["type"] = "heartbeat_ack";
    response["module_id"] = moduleId;
    response["timestamp"] = millis();
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
  }
}

void handlePublish(int clientIndex, JsonDocument& doc) {
  String topic = doc["topic"];
  String payload = doc["payload"];
  
  // Reenviar a todos los suscriptores del topic
  forwardToSubscribers(topic, payload);
}

void handleSubscribe(int clientIndex, JsonDocument& doc) {
  String topic = doc["topic"];
  
  // Agregar cliente a suscriptores (implementación simple)
  Serial.print("Cliente ");
  Serial.print(clientIndex);
  Serial.print(" suscrito a: ");
  Serial.println(topic);
}

void handleConfiguration(int clientIndex, JsonDocument& doc) {
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

void handleMACResponse(int clientIndex, JsonDocument& doc) {
  String moduleId = doc["module_id"];
  String macAddress = doc["mac_address"];
  
  Serial.println("📡 Respuesta MAC recibida");
  
  // Almacenar la MAC recibida
  lastRequestedMAC = macAddress;
  
  // Verificar si el dispositivo está registrado
  bool isRegistered = false;
  if (registeredModules.find(moduleId) != registeredModules.end()) {
    registeredModules[moduleId].macAddress = macAddress;
    Serial.println("✅ MAC actualizada para módulo registrado");
    isRegistered = true;
  } else {
    // Verificar si la MAC está en dispositivos autorizados
    for (auto& pair : authorizedDevices) {
      if (pair.second.macAddress == macAddress) {
        Serial.println("✅ MAC encontrada en dispositivos autorizados");
        isRegistered = true;
        break;
      }
    }
  }
  
  // Si no está registrado, solicitar registro
  if (!isRegistered) {
    Serial.println("⚠️ Dispositivo no registrado - solicitando registro");
    Serial.println("📤 Enviando solicitud de registro...");
    
    StaticJsonDocument<200> registrationRequest;
    registrationRequest["type"] = "registration_required";
    registrationRequest["message"] = "Dispositivo no registrado. Envía código de verificación.";
    registrationRequest["format"] = "ID:MAC:PASSWORD";
    registrationRequest["example"] = "20:" + macAddress + ":" + REGISTRATION_PASSWORD;
    
    String requestStr;
    serializeJson(registrationRequest, requestStr);
    mqttClients[clientIndex].println(requestStr);
  } else {
    // Si ya está registrado, enviar confirmación directa
    Serial.println("✅ Dispositivo ya registrado - enviando confirmación");
    
    StaticJsonDocument<200> confirmation;
    confirmation["type"] = "registration_response";
    confirmation["response_type"] = "device";
    confirmation["status"] = "success";
    confirmation["message"] = "Dispositivo ya autenticado";
    confirmation["module_id"] = moduleId;
    
    String confirmStr;
    serializeJson(confirmation, confirmStr);
    mqttClients[clientIndex].println(confirmStr);
  }
}

void handleDeviceRegistration(int clientIndex, JsonDocument& doc) {
  String verificationCode = doc["verification_code"];
  
  Serial.println("🔐 Código de verificación recibido");
  
  // Parsear el código formato ID:MAC:PASSWORD
  int firstColon = verificationCode.indexOf(':');
  int lastColon = verificationCode.lastIndexOf(':');
  
  if (firstColon == -1 || lastColon == -1 || firstColon == lastColon) {
    Serial.println("❌ Formato de código inválido");
    
    StaticJsonDocument<200> response;
    response["type"] = "registration_response";
    response["response_type"] = "device";
    response["status"] = "error";
    response["message"] = "Formato de código inválido. Use ID:MAC:PASSWORD";
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
    return;
  }
  
  String deviceIdStr = verificationCode.substring(0, firstColon);
  String macAddress = verificationCode.substring(firstColon + 1, lastColon);
  String password = verificationCode.substring(lastColon + 1);
  
  // Limpiar espacios en blanco
  deviceIdStr.trim();
  macAddress.trim();
  password.trim();
  
  int deviceId = deviceIdStr.toInt();
  
  Serial.println("� Procesando código de verificación para dispositivo ID: " + String(deviceId));
  
  // Verificar contraseña de registro
  if (password != REGISTRATION_PASSWORD) {
    Serial.println("❌ Contraseña de registro incorrecta");
    
    StaticJsonDocument<200> response;
    response["type"] = "registration_response";
    response["response_type"] = "device";
    response["status"] = "error";
    response["message"] = "Contraseña de registro incorrecta";
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
    return;
  }
  
  // Determinar tipo de dispositivo por ID
  String deviceType;
  String moduleType;
  switch (deviceId) {
    case DEVICE_ID_FINGERPRINT:
      deviceType = "fingerprint";
      moduleType = "fingerprint_scanner";
      break;
    case DEVICE_ID_SLAVE_1:
    case DEVICE_ID_SLAVE_2:
    case DEVICE_ID_SLAVE_3:
      deviceType = "slave";
      moduleType = "slave_device";
      break;
    case DEVICE_ID_TERMINAL:
      deviceType = "terminal";
      moduleType = "terminal";
      break;
    default:
      Serial.println("❌ ID de dispositivo no válido: " + String(deviceId));
      
      StaticJsonDocument<200> response;
      response["type"] = "registration_response";
      response["response_type"] = "device";
      response["status"] = "error";
      response["message"] = "ID de dispositivo no válido";
      
      String responseStr;
      serializeJson(response, responseStr);
      mqttClients[clientIndex].println(responseStr);
      return;
  }
  
  // Registrar dispositivo
  AuthorizedDevice newDevice;
  newDevice.macAddress = macAddress;
  newDevice.deviceType = deviceType;
  newDevice.apiKey = generateAPIKey();
  newDevice.description = "Dispositivo ID " + String(deviceId);
  newDevice.isActive = true;
  newDevice.lastSeen = millis();
  newDevice.currentIP = mqttClients[clientIndex].remoteIP().toString();
  
  authorizedDevices[macAddress] = newDevice;
  
  Serial.println("✅ Dispositivo registrado exitosamente - Tipo: " + deviceType);
  
  // Responder con éxito
  StaticJsonDocument<300> response;
  response["type"] = "registration_response";
  response["response_type"] = "device";
  response["status"] = "success";
  response["message"] = "Dispositivo registrado exitosamente";
  response["device_id"] = deviceId;
  response["device_type"] = deviceType;
  response["api_key"] = newDevice.apiKey;
  
  String responseStr;
  serializeJson(response, responseStr);
  mqttClients[clientIndex].println(responseStr);
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
      } else if (shortCommand.startsWith("enroll:")) {
        // Comando: server:enroll:nombre
        String userName = shortCommand.substring(7); // Remover "enroll:"
        
        if (userName.length() == 0) {
          Serial.println("❌ Error: Debe especificar un nombre");
          Serial.println("💡 Formato: server:enroll:Juan");
          return;
        }
        
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          String enrollCommand = "enroll_user:" + userName;
          sendCommandToModule(fingerprintId, enrollCommand);
          Serial.println("👤 Iniciando enrolamiento para: " + userName);
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else if (shortCommand.startsWith("delete:")) {
        // Comando: server:delete:userID
        String userIdStr = shortCommand.substring(7); // Remover "delete:"
        
        if (userIdStr.length() == 0) {
          Serial.println("❌ Error: Debe especificar un ID de usuario");
          Serial.println("💡 Formato: server:delete:1");
          return;
        }
        
        int userId = userIdStr.toInt();
        if (userId <= 0) {
          Serial.println("❌ Error: ID de usuario inválido");
          return;
        }
        
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          String deleteCommand = "delete_user:" + String(userId);
          sendCommandToModule(fingerprintId, deleteCommand);
          Serial.println("🗑️ Eliminando usuario ID: " + String(userId));
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else if (shortCommand.startsWith("finger_cut:")) {
        // Comando: server:finger_cut:id (alias amigable para delete)
        String userIdStr = shortCommand.substring(11); // Remover "finger_cut:"
        
        if (userIdStr.length() == 0) {
          Serial.println("❌ Error: Debe especificar un ID de huella");
          Serial.println("💡 Formato: server:finger_cut:1");
          return;
        }
        
        int userId = userIdStr.toInt();
        if (userId <= 0) {
          Serial.println("❌ Error: ID de huella inválido");
          return;
        }
        
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          String deleteCommand = "delete_user:" + String(userId);
          sendCommandToModule(fingerprintId, deleteCommand);
          Serial.println("✂️ Cortando huella ID: " + String(userId));
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else if (shortCommand == "list_fingerprints") {
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          sendCommandToModule(fingerprintId, "list_all_fingerprints");
          Serial.println("📋 Solicitando lista de huellas registradas...");
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else if (shortCommand == "clear_all") {
        // Comando: server:clear_all
        Serial.println("⚠️ ADVERTENCIA: Esto borrará TODAS las huellas");
        Serial.println("🗑️ Ejecutando reseteo de fábrica...");
        
        String fingerprintId = findModuleByType("fingerprint_scanner");
        if (fingerprintId.length() > 0) {
          sendCommandToModule(fingerprintId, "clear_all");
          Serial.println("🔄 Comando de reseteo enviado");
        } else {
          Serial.println("❌ No hay módulos de huella registrados");
        }
      } else {
        Serial.println("❌ Comandos server disponibles:");
        Serial.println("   scan_fingerprint     - Escanear huella");
        Serial.println("   status_fingerprint   - Estado de huella");
        Serial.println("   info_fingerprint     - Info de huella");
        Serial.println("   enroll:nombre        - Enrolar usuario");
        Serial.println("   delete:userID        - Eliminar usuario");
        Serial.println("   finger_cut:id        - Cortar huella (alias de delete)");
        Serial.println("   list_fingerprints    - Listar IDs registrados");
        Serial.println("   clear_all            - Borrar todas las huellas");
        Serial.println("💡 Ejemplos:");
        Serial.println("   server:enroll:Juan");
        Serial.println("   server:delete:1");
        Serial.println("   server:finger_cut:5");
        Serial.println("   server:list_fingerprints");
        Serial.println("   server:clear_all");
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
              StaticJsonDocument<200> cmdMsg;
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
      Serial.println("server:enroll:nombre     - Enrolar usuario");
      Serial.println("server:delete:userID     - Eliminar usuario");
      Serial.println("server:finger_cut:id     - Cortar huella (alias de delete)");
      Serial.println("server:list_fingerprints - Listar IDs registrados");
      Serial.println("server:clear_all         - Borrar todas las huellas");
      Serial.println("💡 Ejemplos:");
      Serial.println("   server:enroll:Juan");
      Serial.println("   server:delete:1");
      Serial.println("   server:finger_cut:5");
      Serial.println("   server:list_fingerprints");
      Serial.println("   server:clear_all");
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

String sha256Hash(const String& input) {
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
  
  const size_t inputLength = input.length();
  const unsigned char* inputBytes = (const unsigned char*)input.c_str();
  unsigned char output[32];
  
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, inputBytes, inputLength);
  mbedtls_md_finish(&ctx, output);
  mbedtls_md_free(&ctx);
  
  String result = "";
  for (int i = 0; i < 32; i++) {
    if (output[i] < 16) result += "0";
    result += String(output[i], HEX);
  }
  
  return result;
}

String generateAPIKey() {
  // Generar API key usando MAC + timestamp + random
  String mac = WiFi.macAddress();
  String timestamp = String(millis());
  String randomNum = String(esp_random());
  
  String combined = mac + timestamp + randomNum + "DEPOSITO_AUTH_v1";
  String hash = sha256Hash(combined);
  
  // Tomar los primeros 32 caracteres del hash
  return hash.substring(0, 32);
}

bool authenticateDevice(const String& macAddress, const String& apiKey) {
  // Buscar dispositivo en la lista de autorizados
  if (authorizedDevices.find(macAddress) != authorizedDevices.end()) {
    AuthorizedDevice& device = authorizedDevices[macAddress];
    
    if (device.isActive && device.apiKey == apiKey) {
      device.lastSeen = millis();
      return true;
    }
  }
  
  Serial.println("❌ Autenticación fallida para MAC: " + macAddress);
  return false;
}

void sendCommandToModule(const String& moduleId, const String& command) {
  Serial.println("🔧 DEBUG sendCommandToModule: " + moduleId + " -> " + command);
  
  // Buscar el módulo registrado
  if (registeredModules.find(moduleId) != registeredModules.end()) {
    ModuleInfo& module = registeredModules[moduleId];
    
    // Verificar que el módulo esté autenticado
    if (!module.isAuthenticated) {
      Serial.println("❌ Módulo no autenticado: " + moduleId);
      return;
    }
    
    Serial.println("🔧 DEBUG: Módulo encontrado en registro");
    
    // Buscar cliente conectado para ese módulo
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        Serial.println("🔧 DEBUG: Enviando a cliente " + String(i));
        
        // Enviar comando al cliente
        StaticJsonDocument<200> cmdMsg;
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

void requestMACFromFingerprintScanners() {
  Serial.println("🔍 Consultando MAC de FingerprintScanners conectados...");
  
  // Buscar todos los módulos de tipo fingerprint_scanner
  for (auto& pair : registeredModules) {
    ModuleInfo& module = pair.second;
    
    if (module.moduleType == "fingerprint_scanner" && module.isAuthenticated) {
      Serial.println("📡 Solicitando MAC a: " + pair.first);
      sendCommandToModule(pair.first, "server:request_mac");
    }
  }
  
  // Marcar tiempo de solicitud
  macRequestTime = millis();
  lastRequestedMAC = "";
}

void setupWebServer() {
  // Usar HTML limpio desde archivo separado
  webServer.on("/", HTTP_GET, [](){
    webServer.send(200, "text/html", getAdminHTML());
  });
  
  webServer.on("/admin", HTTP_GET, [](){
    webServer.send(200, "text/html", getAdminHTML());
  });
  
  // API para información del sistema
  webServer.on("/api/system-info", HTTP_GET, [](){
    StaticJsonDocument<512> response;
    response["ap_ip"] = WiFi.softAPIP().toString();
    response["uptime"] = millis();
    response["free_heap"] = ESP.getFreeHeap();
    response["connected_clients"] = WiFi.softAPgetStationNum();
    response["registered_modules"] = registeredModules.size();
    response["authorized_devices"] = authorizedDevices.size();
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para login
  webServer.on("/api/login", HTTP_POST, [](){
    StaticJsonDocument<200> response;
    
    if (webServer.hasArg("username") && webServer.hasArg("password")) {
      String username = webServer.arg("username");
      String password = webServer.arg("password");
      
      if (username == ADMIN_USER && password == ADMIN_PASS) {
        response["success"] = true;
        response["message"] = "Login exitoso";
        Serial.println("✅ Login exitoso desde: " + webServer.client().remoteIP().toString());
      } else {
        response["success"] = false;
        response["message"] = "Credenciales incorrectas";
        Serial.println("❌ Intento de login fallido desde: " + webServer.client().remoteIP().toString());
      }
    } else {
      response["success"] = false;
      response["message"] = "Parámetros faltantes";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para listar dispositivos
  webServer.on("/api/devices", HTTP_GET, [](){
    StaticJsonDocument<2048> response;
    JsonArray devices = response["devices"];
    
    for (auto& pair : authorizedDevices) {
      JsonObject device = devices.add<JsonObject>();
      device["macAddress"] = pair.second.macAddress;
      device["deviceType"] = pair.second.deviceType;
      device["apiKey"] = pair.second.apiKey;
      device["description"] = pair.second.description;
      device["isActive"] = pair.second.isActive;
      device["lastSeen"] = pair.second.lastSeen;
      device["currentIP"] = pair.second.currentIP;
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para agregar dispositivo
  webServer.on("/api/add-device", HTTP_POST, [](){
    StaticJsonDocument<400> response;
    
    if (webServer.hasArg("macAddress") && 
        webServer.hasArg("deviceType") &&
        webServer.hasArg("description")) {
      
      String macAddress = webServer.arg("macAddress");
      String deviceType = webServer.arg("deviceType");
      String description = webServer.arg("description");
      
      // Generar API key
      String apiKey = generateAPIKey();
      
      // Crear dispositivo
      AuthorizedDevice newDevice;
      newDevice.macAddress = macAddress;
      newDevice.deviceType = deviceType;
      newDevice.apiKey = apiKey;
      newDevice.description = description;
      newDevice.isActive = true;
      newDevice.lastSeen = 0;
      newDevice.currentIP = "";
      
      // Agregar a la lista
      authorizedDevices[macAddress] = newDevice;
      
      // Guardar en SPIFFS
      saveAuthorizedDevices();
      
      response["success"] = true;
      response["message"] = "Dispositivo agregado exitosamente";
      response["apiKey"] = apiKey;
      
      Serial.println("✅ Dispositivo agregado: " + macAddress + " (" + deviceType + ")");
    } else {
      response["success"] = false;
      response["message"] = "Parámetros faltantes";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para eliminar dispositivo
  webServer.on("/api/remove-device", HTTP_POST, [](){
    StaticJsonDocument<200> response;
    
    if (webServer.hasArg("macAddress")) {
      String macAddress = webServer.arg("macAddress");
      
      if (authorizedDevices.find(macAddress) != authorizedDevices.end()) {
        authorizedDevices.erase(macAddress);
        saveAuthorizedDevices();
        
        response["success"] = true;
        response["message"] = "Dispositivo eliminado";
        
        Serial.println("🗑️ Dispositivo eliminado: " + macAddress);
      } else {
        response["success"] = false;
        response["message"] = "Dispositivo no encontrado";
      }
    } else {
      response["success"] = false;
      response["message"] = "MAC address faltante";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para consultar MAC de FingerprintScanners
  webServer.on("/api/request-mac", HTTP_POST, [](){
    StaticJsonDocument<200> response;
    
    // Reiniciar la variable de MAC recibida
    lastRequestedMAC = "";
    
    // Solicitar MAC a todos los FingerprintScanners conectados
    requestMACFromFingerprintScanners();
    
    response["success"] = true;
    response["message"] = "Solicitud enviada a FingerprintScanners conectados";
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para obtener la última MAC recibida
  webServer.on("/api/get-mac", HTTP_GET, [](){
    StaticJsonDocument<200> response;
    
    if (lastRequestedMAC != "") {
      response["success"] = true;
      response["macAddress"] = lastRequestedMAC;
    } else {
      response["success"] = false;
      response["message"] = "No hay MAC disponible";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  webServer.begin();
  Serial.println("✅ Servidor web iniciado en http://" + WiFi.softAPIP().toString());
}

void loadAuthorizedDevices() {
  // TODO: Implementar carga desde SPIFFS
  // Por ahora, agregar un dispositivo de prueba
  AuthorizedDevice testDevice;
  testDevice.macAddress = "AA:BB:CC:DD:EE:FF";
  testDevice.deviceType = "fingerprint";
  testDevice.apiKey = generateAPIKey();
  testDevice.description = "Scanner de Prueba";
  testDevice.isActive = true;
  testDevice.lastSeen = 0;
  testDevice.currentIP = "";
  
  authorizedDevices[testDevice.macAddress] = testDevice;
  
  Serial.println("📱 Dispositivo de prueba agregado");
}

void saveAuthorizedDevices() {
  // TODO: Implementar guardado en SPIFFS
  Serial.println("💾 Dispositivos autorizados guardados (pendiente implementar persistencia)");
}