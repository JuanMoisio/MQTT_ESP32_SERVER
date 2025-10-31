#include "MQTTBrokerManager.h"
#include "WiFiManager.h"
#include "DeviceManager.h"

// Constructor: inicializar mqttServer y arrays
MQTTBrokerManager::MQTTBrokerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr)
    : mqttServer(MQTT_PORT), wifiManager(wifiMgr), deviceManager(deviceMgr) {
    // Inicializar arrays
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clientConnected[i] = false;
        lastHeartbeatSent[i] = 0;
    }
}

void MQTTBrokerManager::setDeviceManager(DeviceManager* deviceMgr) {
    deviceManager = deviceMgr;
}

void MQTTBrokerManager::initialize() {
    Serial.println("📡 Inicializando MQTT Broker Manager...");
    
    // Inicializar servidor MQTT
    mqttServer.begin();
    Serial.print("✅ Servidor MQTT iniciado en puerto ");
    Serial.println(MQTT_PORT);
}

void MQTTBrokerManager::handleNewConnections() {
    WiFiClient newClient = mqttServer.available();
    
    if (newClient) {
        // Buscar slot disponible
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clientConnected[i]) {
                mqttClients[i] = newClient;
                clientConnected[i] = true;
                lastHeartbeatSent[i] = millis();
                
                Serial.print("Nuevo cliente conectado en slot ");
                Serial.println(i);
                
                // Enviar mensaje de bienvenida
                sendWelcomeMessage(i);
                break;
            }
        }
    }
}

// sendWelcomeMessage: usar DynamicJsonDocument y enviar string
void MQTTBrokerManager::sendWelcomeMessage(int clientIndex) {
    DynamicJsonDocument welcome(256);
    welcome["type"] = "welcome";
    welcome["message"] = "Welcome to embedded broker";
    String out;
    serializeJson(welcome, out);
    sendToClient(clientIndex, out);
}

// sendAuthSuccessMessage: usar DynamicJsonDocument local y enviar
void MQTTBrokerManager::sendAuthSuccessMessage(int clientIndex, const String& macAddress, const String& apiKey) {
    DynamicJsonDocument authSuccess(256);
    authSuccess["type"] = "auth_success";
    authSuccess["mac_address"] = macAddress;
    authSuccess["api_key"] = apiKey;
    String authStr;
    serializeJson(authSuccess, authStr);

    // Enviar al cliente si está conectado
    if (clientIndex >= 0 && clientIndex < MAX_CLIENTS && clientConnected[clientIndex]) {
        mqttClients[clientIndex].println(authStr);
    }

    // Construir y enviar registration response también con DynamicJsonDocument
    DynamicJsonDocument regResponse(256);
    regResponse["type"] = "registration_response";
    regResponse["status"] = "success";
    regResponse["mac_address"] = macAddress;
    String regStr;
    serializeJson(regResponse, regStr);

    if (clientIndex >= 0 && clientIndex < MAX_CLIENTS && clientConnected[clientIndex]) {
        mqttClients[clientIndex].println(regStr);
    }
}

String MQTTBrokerManager::getClientIP(int clientIndex) {
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS) return String("NA");
    if (!clientConnected[clientIndex]) return String("NA");
    return mqttClients[clientIndex].remoteIP().toString();
}

void MQTTBrokerManager::sendToAllClients(const String& message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientConnected[i]) {
            mqttClients[i].println(message);
        }
    }
}

void MQTTBrokerManager::sendToClient(int clientIndex, const String& payload) {
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS) return;
    if (!clientConnected[clientIndex]) return;
    mqttClients[clientIndex].println(payload);
}

void MQTTBrokerManager::processClientMessages() {
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
            lastHeartbeatSent[i] = 0;
        }
    }
}

void MQTTBrokerManager::processMessage(int clientIndex, String payload) {
    Serial.print("[MQTTBrokerManager] Cliente ");
    Serial.print(clientIndex);
    Serial.print(" envió: ");
    Serial.println(payload);

    DynamicJsonDocument doc(1024);
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Serial.print("[MQTTBrokerManager] JSON parse error: ");
        Serial.println(err.c_str());
        return;
    }

    // Extraer tipo de mensaje de forma segura
    String msgType = "";
    if (doc.containsKey("type") && doc["type"].is<const char*>()) {
        msgType = String((const char*)doc["type"]);
    } else if (doc.containsKey("type")) {
        String tmp; serializeJson(doc["type"], tmp); msgType = tmp;
    }
    Serial.print("[MQTTBrokerManager] 🔍 Tipo de mensaje identificado: '");
    Serial.print(msgType);
    Serial.println("'");

    // Dispatch a DeviceManager con logging
    if (msgType == "module_registration") {
        Serial.println("[MQTTBrokerManager] Dispatching module_registration -> DeviceManager::handleModuleRegistration");
        if (deviceManager) deviceManager->handleModuleRegistration(clientIndex, (JsonDocument&)doc, &mqttClients[clientIndex]);
    } else if (msgType == "mac_response" || msgType == "mac") {
        Serial.println("[MQTTBrokerManager] Dispatching mac_response -> DeviceManager::handleMACResponse");
        if (deviceManager) deviceManager->handleMACResponse(clientIndex, (JsonDocument&)doc);
    } else if (msgType == "device_info" || msgType == "info_response") {
        Serial.println("[MQTTBrokerManager] Dispatching device_info -> DeviceManager::handleDeviceInfoResponse");
        if (deviceManager) deviceManager->handleDeviceInfoResponse(clientIndex, (JsonDocument&)doc);
    } else if (msgType == "scan_response" || msgType == "device_scan") {
        Serial.println("[MQTTBrokerManager] Dispatching scan_response -> DeviceManager::handleDeviceScanResponse");
        if (deviceManager) deviceManager->handleDeviceScanResponse(clientIndex, (JsonDocument&)doc);
    } else if (msgType == "heartbeat") {
        Serial.println("[MQTTBrokerManager] Heartbeat recibido -> procesando");
        // Si viene module_id, actualizar heartbeat en DeviceManager
        if (doc.containsKey("module_id") && doc["module_id"].is<const char*>()) {
            String mid = String((const char*)doc["module_id"]);
            Serial.print("[MQTTBrokerManager] Heartbeat de module_id: ");
            Serial.println(mid);
            if (deviceManager) deviceManager->updateModuleHeartbeat(mid);
        }
        // Si viene mac_address, marcar dispositivo autorizado como conectado
        if (doc.containsKey("mac_address") && doc["mac_address"].is<const char*>()) {
            String mac = String((const char*)doc["mac_address"]);
            String ip = mqttClients[clientIndex].remoteIP().toString();
            Serial.print("[MQTTBrokerManager] Heartbeat incluye MAC: ");
            Serial.print(mac);
            Serial.print(" -> marcando como conectado clientIndex=");
            Serial.println(clientIndex);
            if (deviceManager) deviceManager->markDeviceConnected(mac, clientIndex, ip);
            // Usar wrapper público en lugar de llamar al método privado
            String moduleId = doc.containsKey("module_id") && doc["module_id"].is<const char*>() ? String((const char*)doc["module_id"]) : String("");
            if (deviceManager) deviceManager->reportScannedDevice(mac, "", moduleId, clientIndex);
        }
    } else {
        Serial.println("[MQTTBrokerManager] Tipo no manejado localmente -> forwarding/raw handling");
        // Manejo genérico: si tu lógica requiere, puedes reenviar a todos o a un handler genérico
    }

    // Ejemplo: guardar actions responses si vienen
    if (doc.containsKey("module_id") && doc.containsKey("actions")) {
        String mid = String((const char*)doc["module_id"]);
        String serializedActions;
        serializeJson(doc["actions"], serializedActions);
        actionsResponseBuffer[mid] = serializedActions;
        Serial.println("[MQTTBrokerManager] Stored actionsResponseBuffer for " + mid);
    }
}

void MQTTBrokerManager::handleModuleRegistration(int clientIndex, JsonDocument& doc) {
    // Delegar al DeviceManager
    deviceManager->handleModuleRegistration(clientIndex, doc, &mqttClients[clientIndex]);
}

void MQTTBrokerManager::handleHeartbeat(int clientIndex, JsonDocument& doc) {
    String moduleId = doc["module_id"];
    
    if (deviceManager->updateModuleHeartbeat(moduleId)) {
        // Responder heartbeat
        DynamicJsonDocument response(256);
        response["type"] = "heartbeat_ack";
        response["timestamp"] = millis();
        
        String responseStr;
        serializeJson(response, responseStr);
        mqttClients[clientIndex].println(responseStr);
    }
}

void MQTTBrokerManager::handlePublish(int clientIndex, JsonDocument& doc) {
    String topic = doc["topic"];
    String payload = doc["payload"];
    
    // Reenviar a todos los suscriptores del topic
    forwardToSubscribers(topic, payload);
}

void MQTTBrokerManager::handleSubscribe(int clientIndex, JsonDocument& doc) {
    String topic = doc["topic"];
    
    // Agregar cliente a suscriptores (implementación simple)
    Serial.print("Cliente ");
    Serial.print(clientIndex);
    Serial.print(" suscrito a: ");
    Serial.println(topic);
}

void MQTTBrokerManager::handleConfiguration(int clientIndex, JsonDocument& doc) {
    String configType = doc["config_type"];
    
    if (configType == "get_modules") {
        String responseStr = deviceManager->getModulesJSON();
        mqttClients[clientIndex].println(responseStr);
    } else if (configType == "set_discovery") {
        bool discoveryMode = doc["value"];
        deviceManager->setDiscoveryMode(discoveryMode);
        
        DynamicJsonDocument response(512);
        response["type"] = "configuration_response";
        response["status"] = "ok";
        String responseStr;
        serializeJson(response, responseStr);
        mqttClients[clientIndex].println(responseStr);
    }
}

void MQTTBrokerManager::forwardMessage(int senderIndex, String message) {
    // Reenviar mensaje a todos los demás clientes conectados
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != senderIndex && clientConnected[i]) {
            mqttClients[i].println(message);
        }
    }
}

void MQTTBrokerManager::forwardToSubscribers(String topic, String message) {
    // Crear mensaje de publicación
    DynamicJsonDocument pubMessage(512);
    pubMessage["type"] = "publish";
    pubMessage["topic"] = topic;
    pubMessage["message"] = message;
    String pubMessageStr;
    serializeJson(pubMessage, pubMessageStr);
    
    // Enviar a todos los clientes conectados (implementación simple)
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clientConnected[i]) {
            mqttClients[i].println(pubMessageStr);
        }
    }
}

void MQTTBrokerManager::checkModuleHeartbeats() {
    deviceManager->checkModuleHeartbeats(mqttClients, clientConnected);
}

bool MQTTBrokerManager::sendCommandToModule(const String& moduleId, const String& command, JsonVariantConst params) {
    Serial.println("🔧 DEBUG sendCommandToModule(+params): " + moduleId + " -> " + command);

    if (!deviceManager || !deviceManager->isModuleRegistered(moduleId)) {
        Serial.println("❌ Módulo no encontrado: " + moduleId);
        return false;
    }

    DynamicJsonDocument cmdMsg(1024);
    cmdMsg["type"]      = "command";
    cmdMsg["module_id"] = moduleId;
    cmdMsg["command"]   = command;
    cmdMsg["timestamp"] = millis();
    if (!params.isNull()) {
        cmdMsg["params"] = params; // queda como objeto JSON
    }
    String cmdStr; serializeJson(cmdMsg, cmdStr);

    // Por ahora se reenvía a todos los clientes (cada cliente filtra por module_id)
    bool anySent = false;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientConnected[i]) {
            mqttClients[i].println(cmdStr);
            anySent = true;
            Serial.println("📨 Comando enviado a cliente " + String(i) + ": " + cmdStr);
        }
    }
    return anySent;
}

// (Opcional) mantener el viejo para compatibilidad interna
void MQTTBrokerManager::sendCommandToModule(const String& moduleId, const String& command) {
    StaticJsonDocument<1> empty; // params vacío
    sendCommandToModule(moduleId, command, empty.as<JsonVariantConst>());
}


int MQTTBrokerManager::getConnectedClientsCount() {
    int connectedCount = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientConnected[i]) {
            connectedCount++;
        }
    }
    return connectedCount;
}

void MQTTBrokerManager::sendHeartbeatToClients() {
    unsigned long now = millis();
    const unsigned long HEARTBEAT_INTERVAL = 15000; // 15 segundos
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientConnected[i] && (now - lastHeartbeatSent[i]) > HEARTBEAT_INTERVAL) {
            DynamicJsonDocument ping(128);
            ping["type"] = "ping";
            ping["timestamp"] = now;
            ping["message"] = "keep-alive";
            
            String pingStr;
            serializeJson(ping, pingStr);
            
            if (mqttClients[i].connected()) {
                mqttClients[i].println(pingStr);
                lastHeartbeatSent[i] = now;
                Serial.println("📡 Ping enviado a cliente " + String(i));
            } else {
                Serial.println("⚠️ Cliente " + String(i) + " desconectado durante heartbeat");
                clientConnected[i] = false;
                lastHeartbeatSent[i] = 0;
            }
        }
    }
}

// Implementación actualizada: serializar JsonDocument almacenado a String
bool MQTTBrokerManager::getLastActionsResponse(const String& moduleId, String& outJson) {
    auto it = actionsResponseBuffer.find(moduleId);
    if (it == actionsResponseBuffer.end()) return false;
    outJson = it->second;
    return true;
}