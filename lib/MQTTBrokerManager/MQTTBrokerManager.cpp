#include "MQTTBrokerManager.h"
#include "WiFiManager.h"
#include "DeviceManager.h"

MQTTBrokerManager::MQTTBrokerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr)
    : mqttServer(MQTT_PORT), wifiManager(wifiMgr), deviceManager(deviceMgr) {
    
    // Inicializar array de conexiones
    for (int i = 0; i < MAX_CLIENTS; i++) {
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

void MQTTBrokerManager::sendWelcomeMessage(int clientIndex) {
    JsonDocument welcome;
    welcome["type"] = "system";
    welcome["action"] = "welcome";
    welcome["broker_id"] = "esp32c3_broker_001";
    welcome["timestamp"] = millis();
    welcome["message"] = "Conectado al broker. Envía 'register' para registrar módulo.";
    
    String welcomeStr;
    serializeJson(welcome, welcomeStr);
    
    mqttClients[clientIndex].println(welcomeStr);
}

void MQTTBrokerManager::sendAuthSuccessMessage(int clientIndex, const String& macAddress, const String& apiKey) {
    // Find module_id for this mac address from scanned devices
    String moduleId = "";
    if (deviceManager) {
        moduleId = deviceManager->getModuleIdByMac(macAddress);
    }
    
    JsonDocument authSuccess;
    authSuccess["type"] = "auth_success";
    authSuccess["mac_address"] = macAddress;
    authSuccess["api_key"] = apiKey;
    authSuccess["message"] = "Dispositivo autenticado y conectado";
    authSuccess["timestamp"] = millis();
    if (moduleId.length() > 0) {
        authSuccess["module_id"] = moduleId;
    }
    
    String authStr;
    serializeJson(authSuccess, authStr);
    
    Serial.println("📤 Enviando auth_success al cliente " + String(clientIndex) + ": " + authStr);
    mqttClients[clientIndex].println(authStr);
    
    // Also send registration_response as expected by example clients
    JsonDocument regResponse;
    regResponse["type"] = "registration_response";
    regResponse["response_type"] = "module";
    regResponse["mac_address"] = macAddress;
    regResponse["api_key"] = apiKey;
    regResponse["status"] = "success";
    regResponse["message"] = "Dispositivo registrado y autenticado exitosamente";
    regResponse["timestamp"] = millis();
    if (moduleId.length() > 0) {
        regResponse["module_id"] = moduleId;
    }
    
    String regStr;
    serializeJson(regResponse, regStr);
    
    Serial.println("📤 Enviando registration_response al cliente " + String(clientIndex) + ": " + regStr);
    mqttClients[clientIndex].println(regStr);
    
    // Also send module_registered event as some clients expect this
    JsonDocument moduleReg;
    moduleReg["type"] = "module_registered";
    moduleReg["module_id"] = moduleId;
    moduleReg["mac_address"] = macAddress;
    moduleReg["api_key"] = apiKey;
    moduleReg["status"] = "connected";
    moduleReg["message"] = "Módulo conectado y listo";
    moduleReg["timestamp"] = millis();
    
    String moduleRegStr;
    serializeJson(moduleReg, moduleRegStr);
    
    Serial.println("📤 Enviando module_registered al cliente " + String(clientIndex) + ": " + moduleRegStr);
    mqttClients[clientIndex].println(moduleRegStr);
    
    // Send a command to force display update
    JsonDocument displayCmd;
    displayCmd["type"] = "command";
    displayCmd["module_id"] = moduleId;
    displayCmd["command"] = "display_connected";
    displayCmd["message"] = "Dispositivo conectado - actualizar display";
    displayCmd["timestamp"] = millis();
    
    String displayCmdStr;
    serializeJson(displayCmd, displayCmdStr);
    
    Serial.println("📤 Enviando display_connected command al cliente " + String(clientIndex) + ": " + displayCmdStr);
    mqttClients[clientIndex].println(displayCmdStr);
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

void MQTTBrokerManager::sendToClient(int clientIndex, const String& message) {
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS) return;
    if (!clientConnected[clientIndex]) return;
    mqttClients[clientIndex].println(message);
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

void MQTTBrokerManager::processMessage(int clientIndex, String message) {
    Serial.print("Cliente ");
    Serial.print(clientIndex);
    Serial.print(" envió: ");
    Serial.println(message);
    Serial.println("📥 Mensaje RAW recibido de cliente " + String(clientIndex) + ": " + message);
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.print("Error parseando JSON: ");
        Serial.println(error.c_str());
        return;
    }
    
    String type = doc["type"];
    
    Serial.println("🔍 Tipo de mensaje identificado: '" + type + "'");
    
    // Log ALL messages to see what the client is really sending
    if (type == "register") {
        Serial.println("📝 Procesando mensaje REGISTER...");
        Serial.println("📝 REGISTER DETAILS:");
        Serial.println("   module_id: " + doc["module_id"].as<String>());
        Serial.println("   mac_address: " + doc["mac_address"].as<String>());
        Serial.println("   device_type: " + doc["device_type"].as<String>());
        Serial.println("   api_key: " + (doc["api_key"].as<String>().length() > 0 ? doc["api_key"].as<String>().substring(0, 8) + "..." : "EMPTY"));
        handleModuleRegistration(clientIndex, doc);
    } else if (type == "heartbeat") {
        handleHeartbeat(clientIndex, doc);
    } else if (type == "pong") {
        Serial.println("🏓 Pong recibido de cliente " + String(clientIndex));
        // Resetear el timer de heartbeat para este cliente
        lastHeartbeatSent[clientIndex] = millis();
    } else if (type == "publish") {
        handlePublish(clientIndex, doc);
    } else if (type == "subscribe") {
        handleSubscribe(clientIndex, doc);
    } else if (type == "config") {
        handleConfiguration(clientIndex, doc);
    } else if (type == "mac_response" || type == "mac_response_unregistered") {
        Serial.println("📡 Procesando mensaje MAC_RESPONSE...");
        deviceManager->handleMACResponse(clientIndex, doc);
    } else if (type == "device_info_response") {
        deviceManager->handleDeviceInfoResponse(clientIndex, doc);
    } else if (type == "device_scan_response") {
        deviceManager->handleDeviceScanResponse(clientIndex, doc);
    } else if (type == "device_registration") {
        deviceManager->handleDeviceRegistration(clientIndex, doc);
    } else if (type == "module_registration") {
        handleModuleRegistration(clientIndex, doc);
    } else {
        Serial.println("❓ Tipo de mensaje no reconocido: '" + type + "' - reenviando a suscriptores");
        // Reenviar mensaje a suscriptores
        forwardMessage(clientIndex, message);
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
        JsonDocument response;
        response["type"] = "heartbeat_ack";
        response["module_id"] = moduleId;
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
        
        JsonDocument response;
        response["type"] = "config_response";
        response["config_type"] = "discovery_mode";
        response["status"] = "success";
        response["value"] = discoveryMode;
        
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

void MQTTBrokerManager::forwardToSubscribers(String topic, String payload) {
    // Crear mensaje de publicación
    JsonDocument pubMessage;
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

void MQTTBrokerManager::checkModuleHeartbeats() {
    deviceManager->checkModuleHeartbeats(mqttClients, clientConnected);
}

void MQTTBrokerManager::sendCommandToModule(const String& moduleId, const String& command) {
    Serial.println("🔧 DEBUG sendCommandToModule: " + moduleId + " -> " + command);
    
    if (deviceManager->isModuleRegistered(moduleId)) {
        // Buscar cliente conectado para ese módulo
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clientConnected[i]) {
                Serial.println("🔧 DEBUG: Enviando a cliente " + String(i));
                
                // Enviar comando al cliente
                JsonDocument cmdMsg;
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
            JsonDocument ping;
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