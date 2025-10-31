#ifndef MQTT_BROKER_MANAGER_H
#define MQTT_BROKER_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>
#include <WiFi.h>
#include "../../include/config.h"

// Forward declarations to avoid circular includes
class WiFiManager;
class DeviceManager;
class WebServerManager;

class MQTTBrokerManager {
public:
    // Constructor usado en main.cpp (WiFiManager*, DeviceManager*)
    MQTTBrokerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr);

    // Inicialización y loop-related
    void initialize();
    void setDeviceManager(DeviceManager* devMgr);
    void handleNewConnections();
    void processClientMessages();
    void sendHeartbeatToClients();
    void checkModuleHeartbeats();
    int getConnectedClientsCount();
    String getClientIP(int clientIndex);

    // Envíos y utilidades
    void sendAuthSuccessMessage(int clientIndex, const String& macAddress, const String& apiKey);
    void sendToClient(int clientIndex, const String& payload);
    void sendToAllClients(const String& payload);

    // Comandos a módulos (sobrecarga con params)
    void sendCommandToModule(const String& moduleId, const String& command);
    bool sendCommandToModule(const String& moduleId, const String& command, JsonVariantConst params);

    // Obtener última respuesta de acciones (serializada en outJson)
    bool getLastActionsResponse(const String& moduleId, String& outJson);

    // Exponer sendWelcomeMessage (usada internamente)
    void sendWelcomeMessage(int clientIndex);

private:
    WiFiManager* wifiManager = nullptr;
    DeviceManager* deviceManager = nullptr;
    WebServerManager* webServerManager = nullptr;

    // TCP server + clients
    WiFiServer mqttServer; // inicializar en cpp ctor: mqttServer(MQTT_PORT)
    WiFiClient mqttClients[MAX_CLIENTS];
    bool clientConnected[MAX_CLIENTS];
    unsigned long lastHeartbeatSent[MAX_CLIENTS];

    // Buffer de respuestas (serializadas) en lugar de JsonDocument
    std::map<String, String> actionsResponseBuffer;

    // Métodos privados
    void processMessage(int clientIndex, String message);
    void forwardMessage(int senderIndex, String message);
    void forwardToSubscribers(String topic, String payload);

    // Handlers de mensajes específicos
    void handleModuleRegistration(int clientIndex, JsonDocument& doc);
    void handleHeartbeat(int clientIndex, JsonDocument& doc);
    void handlePublish(int clientIndex, JsonDocument& doc);
    void handleSubscribe(int clientIndex, JsonDocument& doc);
    void handleConfiguration(int clientIndex, JsonDocument& doc);
};

#endif