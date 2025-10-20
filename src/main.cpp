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
void handleDeviceInfoResponse(int clientIndex, JsonDocument& doc);
void handleDeviceScanResponse(int clientIndex, JsonDocument& doc);
void addToScannedDevices(const String& macAddress, const String& deviceType, const String& moduleId);
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
void cleanupTestDevices();

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
const int DEVICE_ID_RFID = 30;

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
String lastRequestedDeviceType = "";
unsigned long macRequestTime = 0;

// Variable para controlar modo scan
bool isScanMode = false;

// Estructura para dispositivos escaneados
struct ScannedDevice {
  String macAddress;
  String deviceType;
  String moduleId;
  unsigned long timestamp;
};

// Lista de dispositivos escaneados
std::vector<ScannedDevice> scannedDevices;

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
  Serial.println("🚀 Iniciando Broker MQTT");
  Serial.println("==============================================");
  
  // Inicializar tipos de módulos permitidos
  for (int i = 0; i < NUM_ALLOWED_TYPES; i++) {
    config.allowedModuleTypes.push_back(String(ALLOWED_MODULE_TYPES[i]));
  }
  
  // Configurar WiFi
  setupWiFi();
  
  // SPIFFS no necesario - HTML embebido
  // Serial.println("✅ Sistema web configurado (HTML embebido)");
  
  // Cargar dispositivos autorizados
  loadAuthorizedDevices();
  
  // Limpiar dispositivos de prueba al iniciar
  cleanupTestDevices();
  
  Serial.println("� Dispositivos registrados: " + String(authorizedDevices.size()));
  
  // Configurar servidor web
  setupWebServer();
  
  // Inicializar servidor MQTT
  mqttServer.begin();
  Serial.print("Servidor MQTT iniciado en puerto ");
  Serial.println(MQTT_PORT);
  
  // Configurar sistema
  setupSystem();
  
  Serial.println("✅ Sistema listo");
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
  // Configurar como Access Point
  WiFi.mode(ENABLE_STATION_MODE ? WIFI_AP_STA : WIFI_AP);
  
  // Configurar IP estática para el AP
  if (!WiFi.softAPConfig(ap_ip, ap_gateway, ap_subnet)) {
    Serial.println("❌ Error configurando IP del AP");
  }
  
  // Iniciar Access Point
  if (WiFi.softAP(ap_ssid, ap_password)) {
    Serial.println("✅ Access Point iniciado");
    Serial.print("🌐 IP del broker: ");
    Serial.println(WiFi.softAPIP());
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
  } else if (type == "mac_response_unregistered") {
    handleMACResponse(clientIndex, doc);
  } else if (type == "device_info_response") {
    handleDeviceInfoResponse(clientIndex, doc);
  } else if (type == "device_scan_response") {
    handleDeviceScanResponse(clientIndex, doc);
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
  
  // AUTO-REGISTRO DESHABILITADO: Los dispositivos deben registrarse manualmente
  // Si no está autenticado, rechazar el registro
  if (!isAuthenticated) {
    Serial.println("❌ Dispositivo no autorizado - MAC: " + macAddress);
    
    response["status"] = "error";
    response["message"] = "Dispositivo no autorizado. Debe registrarse manualmente desde el panel de administración.";
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
    return;
  }
  
  // Verificar si el tipo de módulo está permitido
  bool typeAllowed = false;
  // Serial.println("🔍 Verificando tipo de módulo: " + moduleType);
  // Serial.println("🔍 Lista de tipos permitidos:");
  for (const String& allowedType : config.allowedModuleTypes) {
    // Serial.println("  - " + allowedType);
    if (moduleType == allowedType) {
      typeAllowed = true;
      // Serial.println("✅ Tipo encontrado: " + allowedType);
      break;
    }
  }
  
  // Serial.println("🔍 isAuthenticated: " + String(isAuthenticated ? "true" : "false"));
  // Serial.println("🔍 typeAllowed: " + String(typeAllowed ? "true" : "false"));
  
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
  String deviceType = doc["device_type"]; // Nuevo: tipo de dispositivo
  
  Serial.println("📡 Respuesta MAC recibida: " + macAddress + " (Tipo: " + deviceType + ") - ModuleID: " + moduleId);
  Serial.println("🔍 Modo scan activo: " + String(isScanMode ? "SÍ" : "NO"));
  
  // Mapear tipos de dispositivo para compatibilidad con web
  String webDeviceType = deviceType;
  if (deviceType == "rfid_reader") {
    webDeviceType = "rfid";
  } else if (deviceType == "fingerprint_scanner") {
    webDeviceType = "fingerprint";
  }
  
  // SIEMPRE agregar a la lista de scan (para funcionalidad scan completo)
  addToScannedDevices(macAddress, webDeviceType, moduleId);
  
  // Verificar si el dispositivo YA está registrado
  bool isAlreadyRegistered = false;
  
  // Verificar en módulos registrados
  for (auto& pair : registeredModules) {
    if (pair.second.macAddress == macAddress) {
      Serial.println("❌ MAC " + macAddress + " ya está registrada en módulos");
      isAlreadyRegistered = true;
      break;
    }
  }
  
  // Verificar en dispositivos autorizados
  if (!isAlreadyRegistered) {
    for (auto& pair : authorizedDevices) {
      if (pair.second.macAddress == macAddress) {
        Serial.println("❌ MAC " + macAddress + " ya está registrada en dispositivos autorizados");
        isAlreadyRegistered = true;
        break;
      }
    }
  }
  
  // Solo aceptar MACs de dispositivos NO registrados para el sistema de consulta manual
  if (!isAlreadyRegistered) {
    Serial.println("✅ MAC " + macAddress + " (" + deviceType + ") NO está registrada - disponible para registro");
    
    // SIEMPRE guardar MAC y tipo de dispositivos no registrados (para registro manual)
    lastRequestedMAC = macAddress;
    lastRequestedDeviceType = webDeviceType;
    macRequestTime = millis(); // Actualizar timestamp
    
    Serial.println("💾 Guardado para consulta web: " + macAddress + " (" + webDeviceType + ")");
    
    // Enviar respuesta temporal para que el dispositivo no se quede esperando
    StaticJsonDocument<200> tempResponse;
    tempResponse["type"] = "mac_query_received";
    tempResponse["message"] = "MAC recibida - registro manual requerido";
    
    String tempResponseStr;
    serializeJson(tempResponse, tempResponseStr);
    mqttClients[clientIndex].println(tempResponseStr);
    
  } else {
    Serial.println("⚠️ MAC " + macAddress + " ya está registrada - ignorando para consulta manual");
    
    // Responder al dispositivo que ya está registrado
    StaticJsonDocument<200> response;
    response["type"] = "already_registered";
    response["message"] = "Dispositivo ya está registrado en el sistema";
    
    String responseStr;
    serializeJson(response, responseStr);
    mqttClients[clientIndex].println(responseStr);
  }
}

void handleDeviceInfoResponse(int clientIndex, JsonDocument& doc) {
  String moduleId = doc["module_id"];
  String macAddress = doc["mac_address"];
  String deviceType = doc["device_type"];
  
  Serial.println("📡 Respuesta de info de dispositivo: " + macAddress + " (Tipo: " + deviceType + ") - ModuleID: " + moduleId);
  
  // Mapear tipos de dispositivo para compatibilidad con web
  String webDeviceType = deviceType;
  if (deviceType == "rfid_reader") {
    webDeviceType = "rfid";
  } else if (deviceType == "fingerprint_scanner") {
    webDeviceType = "fingerprint";
  }
  
  // Verificar si el dispositivo ya está registrado
  bool isAlreadyRegistered = (authorizedDevices.find(macAddress) != authorizedDevices.end());
  
  // Agregar a la lista de dispositivos escaneados
  addToScannedDevices(macAddress, webDeviceType, moduleId);
  
  // Solo guardar en variables globales si NO está registrado (para registro manual)
  if (!isAlreadyRegistered) {
    Serial.println("💾 Dispositivo NO registrado detectado - disponible para registro manual");
    lastRequestedMAC = macAddress;
    lastRequestedDeviceType = webDeviceType;
    macRequestTime = millis();
  } else {
    Serial.println("✅ Dispositivo ya registrado: " + macAddress);
  }
  
  // Confirmar recepción al dispositivo
  StaticJsonDocument<200> ack;
  ack["type"] = "device_info_ack";
  ack["message"] = "Información recibida correctamente";
  ack["is_registered"] = isAlreadyRegistered;
  
  String ackStr;
  serializeJson(ack, ackStr);
  mqttClients[clientIndex].println(ackStr);
}

void addToScannedDevices(const String& macAddress, const String& deviceType, const String& moduleId) {
  Serial.println("🔍 addToScannedDevices llamada: " + macAddress + " (" + deviceType + ") - ID: " + moduleId);
  
  // Buscar si ya existe en la lista (evitar duplicados)
  bool alreadyExists = false;
  for (auto& existing : scannedDevices) {
    if (existing.macAddress == macAddress) {
      // Actualizar información existente
      existing.deviceType = deviceType;
      existing.moduleId = moduleId;
      existing.timestamp = millis();
      alreadyExists = true;
      Serial.println("🔄 Dispositivo actualizado en lista de scan: " + macAddress);
      break;
    }
  }
  
  // Si no existe, agregar nuevo
  if (!alreadyExists) {
    ScannedDevice newDevice;
    newDevice.macAddress = macAddress;
    newDevice.deviceType = deviceType;
    newDevice.moduleId = moduleId;
    newDevice.timestamp = millis();
    
    scannedDevices.push_back(newDevice);
    Serial.println("✅ Dispositivo agregado a lista de scan: " + macAddress + " (" + deviceType + ") - Total: " + String(scannedDevices.size()));
  }
}

void handleDeviceScanResponse(int clientIndex, JsonDocument& doc) {
  String moduleId = doc["module_id"];
  String macAddress = doc["mac_address"];
  String deviceType = doc["device_type"];
  
  Serial.println("📡 Respuesta de scan recibida: " + macAddress + " (" + deviceType + ") - ID: " + moduleId);
  
  // Mapear tipos de dispositivo para compatibilidad con web
  String webDeviceType = deviceType;
  if (deviceType == "rfid_reader") {
    webDeviceType = "rfid";
  } else if (deviceType == "fingerprint_scanner") {
    webDeviceType = "fingerprint";
  }
  
  // Usar la función helper para agregar/actualizar
  addToScannedDevices(macAddress, webDeviceType, moduleId);
}

void handleDeviceRegistration(int clientIndex, JsonDocument& doc) {
  String verificationCode = doc["verification_code"];
  
  // Serial.println("🔐 Código de verificación recibido");
  
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
    case DEVICE_ID_RFID:
      deviceType = "rfid";
      moduleType = "rfid_reader";
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
    } else if (command.startsWith("register_rfid")) {
      // Comando manual para registrar RFID: register_rfid
      String macAddress = "08:D1:F9:D2:A3:D8"; // MAC del RFID conocida
      
      // Crear dispositivo autorizado
      AuthorizedDevice newDevice;
      newDevice.macAddress = macAddress;
      newDevice.deviceType = "rfid";
      newDevice.apiKey = generateAPIKey();
      newDevice.description = "RFID registrado manualmente";
      newDevice.isActive = true;
      newDevice.lastSeen = millis();
      newDevice.currentIP = ""; // Se actualizará cuando se conecte
      
      // Agregar al mapa de dispositivos autorizados
      authorizedDevices[macAddress] = newDevice;
      
      saveAuthorizedDevices();
      Serial.println("✅ RFID registrado manualmente: " + macAddress);
      Serial.println("📱 Device Type: rfid, API Key: " + newDevice.apiKey);
      
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
  Serial.println("🔍 DISCOVERY: Buscando dispositivos en la red local...");
  
  // Limpiar información anterior para nueva búsqueda
  lastRequestedMAC = "";
  lastRequestedDeviceType = "";
  macRequestTime = 0;
  
  // Preparar mensaje de discovery
  StaticJsonDocument<400> discoveryMsg;
  discoveryMsg["type"] = "device_discovery";
  discoveryMsg["action"] = "request_info";
  discoveryMsg["message"] = "DISCOVERY: Responde con device_info_response incluyendo tu MAC, device_type y module_id";
  discoveryMsg["expected_response"] = "device_info_response";
  discoveryMsg["server_ip"] = WiFi.softAPIP().toString();
  discoveryMsg["server_port"] = 1883;  // Puerto MQTT
  discoveryMsg["timestamp"] = millis();
  discoveryMsg["timeout"] = 10000;  // 10 segundos para responder
  
  String discoveryStr;
  serializeJson(discoveryMsg, discoveryStr);
  
  // Enviar a clientes TCP conectados (si los hay)
  int tcpClientsSent = 0;
  for (int i = 0; i < MAX_CLIENTS; i++) {
    if (clientConnected[i]) {
      mqttClients[i].println(discoveryStr);
      tcpClientsSent++;
    }
  }
  
  // TODO: Agregar broadcast UDP aquí para alcanzar dispositivos no conectados por TCP
  // Por ahora, usando solo TCP
  
  if (tcpClientsSent > 0) {
    Serial.println("📡 Discovery enviado a " + String(tcpClientsSent) + " clientes TCP conectados");
  } else {
    Serial.println("⚠️ No hay clientes TCP conectados");
    Serial.println("💡 Los dispositivos deben conectarse primero al servidor MQTT en " + WiFi.softAPIP().toString() + ":1883");
  }
  
  Serial.println("⏳ Esperando respuestas de dispositivos (timeout: 10 segundos)...");
  
  if (isScanMode) {
    Serial.println("⏳ Esperando respuesta de TODOS los dispositivos conectados...");
  } else {
    Serial.println("⏳ Esperando respuesta de dispositivos NO registrados...");
  }
  
  // Marcar tiempo de solicitud
  macRequestTime = millis();
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
    JsonArray devices = response.createNestedArray("devices");
    
    Serial.println("📋 API devices solicitada - dispositivos autorizados: " + String(authorizedDevices.size()));
    
    for (auto& pair : authorizedDevices) {
      JsonObject device = devices.add<JsonObject>();
      device["macAddress"] = pair.second.macAddress;
      device["deviceType"] = pair.second.deviceType;
      device["apiKey"] = pair.second.apiKey;
      device["description"] = pair.second.description;
      device["isActive"] = pair.second.isActive;
      device["lastSeen"] = pair.second.lastSeen;
      device["currentIP"] = pair.second.currentIP;
      
      Serial.println("📱 Dispositivo en lista: " + pair.second.macAddress + " (" + pair.second.deviceType + ")");
    }
    
    response["success"] = true;
    response["total_devices"] = authorizedDevices.size();
    
    String responseStr;
    serializeJson(response, responseStr);
    
    Serial.println("📤 Respuesta JSON devices: " + responseStr);
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
    
    // NO reiniciar las variables - mantener información previa
    Serial.println("🌐 API consulta recibida desde web");
    
    // Contar clientes TCP conectados
    int connectedCount = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        connectedCount++;
        Serial.println("  📱 Cliente " + String(i) + " conectado desde: " + mqttClients[i].remoteIP().toString());
      }
    }
    
    Serial.println("📊 Resumen:");
    Serial.println("  Clientes TCP conectados: " + String(connectedCount));
    Serial.println("  Dispositivos autorizados: " + String(authorizedDevices.size()));
    Serial.println("  Módulos registrados: " + String(registeredModules.size()));
    
    // Solicitar MAC a todos los FingerprintScanners conectados
    requestMACFromFingerprintScanners();
    
    // Para propósitos de testing - si no hay dispositivos reales conectados,
    // proporcionar opción de simular un dispositivo para pruebas
    if (connectedCount == 0) {
      Serial.println("⚠️ No hay dispositivos físicos conectados por TCP");
      Serial.println("💡 Para pruebas: puede simular un dispositivo no registrado");
      Serial.println("💡 O use el botón 'Simular Dispositivo' en la interfaz web");
    }
    
    response["success"] = true;
    response["message"] = "Discovery iniciado - esperando respuestas de dispositivos reales";
    response["connected_clients"] = connectedCount;
    response["server_ip"] = WiFi.softAPIP().toString();
    response["server_port"] = 1883;
    response["debug_info"] = "Ver consola serial para más detalles";
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });
  
  // API para obtener la última MAC y tipo recibidos
  webServer.on("/api/get-mac", HTTP_GET, [](){
    StaticJsonDocument<200> response;
    
    Serial.println("🔍 DEBUG get-mac:");
    Serial.println("  lastRequestedMAC: '" + lastRequestedMAC + "'");
    Serial.println("  lastRequestedDeviceType: '" + lastRequestedDeviceType + "'");
    Serial.println("  macRequestTime: " + String(macRequestTime));
    Serial.println("  Tiempo actual: " + String(millis()));
    
    // Verificar si la información es reciente (últimos 30 segundos)
    if (lastRequestedMAC != "" && (millis() - macRequestTime) < 30000) {
      response["success"] = true;
      response["macAddress"] = lastRequestedMAC;
      response["deviceType"] = lastRequestedDeviceType;
      response["timestamp"] = macRequestTime;
      Serial.println("✅ Devolviendo MAC encontrada");
    } else {
      response["success"] = false;
      if (lastRequestedMAC == "") {
        response["message"] = "No hay MAC disponible - ningún dispositivo no registrado ha respondido";
        Serial.println("❌ No hay MAC disponible");
      } else {
        response["message"] = "Información de MAC expirada (más de 30 segundos)";
        Serial.println("⏰ MAC expirada, antigüedad: " + String((millis() - macRequestTime)/1000) + " segundos");
      }
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });

  
  // API para obtener módulos registrados
  webServer.on("/api/modules", HTTP_GET, [](){
    StaticJsonDocument<1024> response;
    JsonArray modulesArray = response.createNestedArray("modules");
    
    for (auto& pair : registeredModules) {
      ModuleInfo& module = pair.second;
      JsonObject moduleObj = modulesArray.createNestedObject();
      moduleObj["moduleId"] = module.moduleId;
      moduleObj["moduleType"] = module.moduleType;
      moduleObj["capabilities"] = module.capabilities;
      moduleObj["macAddress"] = module.macAddress;
      moduleObj["isActive"] = module.isActive;
      moduleObj["isAuthenticated"] = module.isAuthenticated;
      moduleObj["lastHeartbeat"] = module.lastHeartbeat;
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });

  // API para obtener estadísticas del dashboard
  webServer.on("/api/stats", HTTP_GET, [](){
    StaticJsonDocument<512> response;
    
    // Dispositivos registrados = total en authorizedDevices
    response["registered_devices"] = authorizedDevices.size();
    
    // Dispositivos conectados = dispositivos autorizados con IP actual conectada
    int connectedDevices = 0;
    unsigned long now = millis();
    
    for (auto& pair : authorizedDevices) {
      AuthorizedDevice& device = pair.second;
      if (device.isActive) {
        // Verificar si hay un cliente TCP actualmente conectado desde la IP del dispositivo
        bool isCurrentlyConnected = false;
        for (int i = 0; i < MAX_CLIENTS; i++) {
          if (clientConnected[i] && mqttClients[i].remoteIP().toString() == device.currentIP) {
            // Además verificar que haya actividad reciente (heartbeat o lastSeen)
            if ((now - device.lastSeen) < 120000) { // 2 minutos
              isCurrentlyConnected = true;
              break;
            }
          }
        }
        if (isCurrentlyConnected) {
          connectedDevices++;
        }
      }
    }
    
    response["connected_devices"] = connectedDevices;
    
    // Estados futuros (por ahora false)
    response["depositario_status"] = false;
    response["placa_motores_status"] = false;
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });

  // API para obtener dispositivos no registrados detectados
  webServer.on("/api/unregistered-devices", HTTP_GET, [](){
    StaticJsonDocument<2048> response;
    JsonArray devices = response.createNestedArray("devices");
    
    // Obtener dispositivos de scan que no están registrados
    for (auto& device : scannedDevices) {
      bool isRegistered = false;
      
      // Verificar en dispositivos autorizados
      for (auto& pair : authorizedDevices) {
        if (pair.second.macAddress == device.macAddress) {
          isRegistered = true;
          break;
        }
      }
      
      // Si no está registrado, agregarlo a la lista
      if (!isRegistered) {
        JsonObject deviceObj = devices.add<JsonObject>();
        deviceObj["macAddress"] = device.macAddress;
        deviceObj["deviceType"] = device.deviceType;
        deviceObj["moduleId"] = device.moduleId;
        deviceObj["timestamp"] = device.timestamp;
        deviceObj["isRegistered"] = false;
      }
    }
    
    // También buscar clientes TCP conectados que no están en authorizedDevices
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        String clientIP = mqttClients[i].remoteIP().toString();
        bool foundInAuthorized = false;
        
        for (auto& pair : authorizedDevices) {
          if (pair.second.currentIP == clientIP) {
            foundInAuthorized = true;
            break;
          }
        }
        
        if (!foundInAuthorized) {
          // Cliente conectado pero no autorizado
          JsonObject deviceObj = devices.add<JsonObject>();
          deviceObj["macAddress"] = "Unknown_" + clientIP;
          deviceObj["deviceType"] = "unknown";
          deviceObj["moduleId"] = "unidentified_" + String(i);
          deviceObj["timestamp"] = millis();
          deviceObj["isRegistered"] = false;
          deviceObj["clientIP"] = clientIP;
        }
      }
    }
    
    response["success"] = true;
    response["total_unregistered"] = devices.size();
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });

  // API para escanear todos los dispositivos conectados (usa lógica modificada)
  webServer.on("/api/scan-devices", HTTP_POST, [](){
    StaticJsonDocument<200> response;
    
    Serial.println("🔍 API scan-devices llamada desde web");
    
    // Debug: mostrar estado de conexiones antes del scan
    int clientesConectados = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
      if (clientConnected[i]) {
        clientesConectados++;
        Serial.println("  📱 Cliente " + String(i) + " conectado desde: " + mqttClients[i].remoteIP().toString());
      }
    }
    Serial.println("📊 Total clientes conectados: " + String(clientesConectados));
    
    // Limpiar resultados anteriores
    scannedDevices.clear();
    
    // Marcar que estamos en modo scan (para que handleMACResponse agregue todos los dispositivos)
    isScanMode = true;
    
    // Solo dispositivos reales - sin simulaciones
    
    // Usar la función existente (para dispositivos reales)
    requestMACFromFingerprintScanners();
    
    response["success"] = true;
    response["message"] = "Scan iniciado - esperando respuestas de dispositivos reales";
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
  });

  // API para obtener resultados del scan
  webServer.on("/api/scan-results", HTTP_GET, [](){
    StaticJsonDocument<2048> response;
    JsonArray devices = response.createNestedArray("devices");
    
    Serial.println("🔍 API scan-results solicitada - dispositivos en lista: " + String(scannedDevices.size()));
    
    // Desactivar modo scan después de obtener resultados
    if (isScanMode) {
      isScanMode = false;
      Serial.println("🔍 Modo scan desactivado");
    }
    
    // Agregar dispositivos escaneados
    for (auto& device : scannedDevices) {
      JsonObject deviceObj = devices.add<JsonObject>();
      deviceObj["macAddress"] = device.macAddress;
      deviceObj["deviceType"] = device.deviceType;
      deviceObj["moduleId"] = device.moduleId;
      
      // Verificar si está registrado
      bool isRegistered = false;
      
      // Verificar en dispositivos autorizados (la clave del mapa ES el MAC address)
      isRegistered = (authorizedDevices.find(device.macAddress) != authorizedDevices.end());
      
      // Verificar en módulos registrados
      if (!isRegistered) {
        for (auto& pair : registeredModules) {
          if (pair.second.macAddress == device.macAddress) {
            isRegistered = true;
            break;
          }
        }
      }
      
      deviceObj["isRegistered"] = isRegistered;
      
      Serial.println("📱 Dispositivo en scan: " + device.macAddress + " (" + device.deviceType + ") - Registrado: " + (isRegistered ? "Sí" : "No"));
    }
    
    response["success"] = true;
    response["total_devices"] = scannedDevices.size();
    
    String responseStr;
    serializeJson(response, responseStr);
    
    Serial.println("📤 Respuesta JSON scan-results: " + responseStr);
    Serial.println("📊 Array devices size: " + String(devices.size()));
    
    webServer.send(200, "application/json", responseStr);
  });
  
  webServer.begin();
  Serial.println("✅ Servidor web iniciado en http://" + WiFi.softAPIP().toString());
}

void loadAuthorizedDevices() {
  // TODO: Implementar carga desde SPIFFS
  // Por ahora, inicializar vacío - los dispositivos se agregan manualmente
  Serial.println("📱 Sistema de dispositivos autorizados inicializado (vacío)");
}

void saveAuthorizedDevices() {
  // TODO: Implementar guardado en SPIFFS
  Serial.println("💾 Dispositivos autorizados guardados (pendiente implementar persistencia)");
}

void cleanupTestDevices() {
  Serial.println("🧹 Limpiando TODOS los dispositivos de prueba...");
  
  // Lista extendida de MACs de dispositivos de prueba a eliminar
  std::vector<String> testMacs = {
    "C8:2B:96:12:34:56",  // fingerprint_scanner_001
    "C8:2B:96:AB:CD:EF",  // rfid_reader_002  
    "C8:2B:96:78:90:12",  // sensor_module_003
    "AA:BB:CC:DD:EE:FF"   // Dispositivo de ejemplo que se agregaba
  };
  
  int removedDevices = 0;
  int removedModules = 0;
  
  // Eliminar de authorizedDevices - primero por MACs específicas
  for (const String& testMac : testMacs) {
    auto it = authorizedDevices.find(testMac);
    if (it != authorizedDevices.end()) {
      Serial.println("🗑️ Eliminando dispositivo de prueba: " + testMac);
      authorizedDevices.erase(it);
      removedDevices++;
    }
  }
  
  // Eliminar dispositivos por descripción de prueba
  auto deviceIt = authorizedDevices.begin();
  while (deviceIt != authorizedDevices.end()) {
    String description = deviceIt->second.description;
    bool isTestDevice = false;
    
    // Verificar si contiene palabras clave de prueba
    if (description.indexOf("Prueba") != -1 || 
        description.indexOf("TEST") != -1 || 
        description.indexOf("test") != -1 ||
        description.indexOf("SIMULADO") != -1 ||
        description.indexOf("simulado") != -1 ||
        description.indexOf("DEMO") != -1 ||
        description.indexOf("demo") != -1 ||
        description.indexOf("Auto-registrado") != -1 ||
        description.indexOf("Eliminar cuando tengas dispositivos reales") != -1) {
      isTestDevice = true;
    }
    
    if (isTestDevice) {
      Serial.println("🗑️ Eliminando dispositivo de prueba por descripción: " + deviceIt->first + " - " + description);
      deviceIt = authorizedDevices.erase(deviceIt);
      removedDevices++;
    } else {
      ++deviceIt;
    }
  }
  
  // Eliminar de registeredModules (buscar por MAC)
  auto moduleIt = registeredModules.begin();
  while (moduleIt != registeredModules.end()) {
    bool isTestDevice = false;
    for (const String& testMac : testMacs) {
      if (moduleIt->second.macAddress == testMac) {
        isTestDevice = true;
        break;
      }
    }
    
    // También eliminar por descripción que contenga "Auto-registrado" o IDs de prueba
    if (!isTestDevice) {
      String moduleId = moduleIt->second.moduleId;
      if (moduleId == "fingerprint_scanner_001" || 
          moduleId == "rfid_reader_002" || 
          moduleId == "sensor_module_003" ||
          moduleIt->second.macAddress.indexOf("Scanner Auto-registrado") != -1) {
        isTestDevice = true;
      }
    }
    
    if (isTestDevice) {
      Serial.println("🗑️ Eliminando módulo de prueba: " + moduleIt->second.moduleId);
      moduleIt = registeredModules.erase(moduleIt);
      removedModules++;
    } else {
      ++moduleIt;
    }
  }
  
  if (removedDevices > 0 || removedModules > 0) {
    Serial.println("✅ Limpieza completada - Dispositivos: " + String(removedDevices) + ", Módulos: " + String(removedModules));
    saveAuthorizedDevices();
  } else {
    Serial.println("ℹ️ No se encontraron dispositivos de prueba");
  }
}           

