#ifndef MQTT_BROKER_MANAGER_H
#define MQTT_BROKER_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "../../include/config.h"

// Forward declarations
class DeviceManager;
class WiFiManager;

class MQTTBrokerManager {
public:
    MQTTBrokerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr);
    
    // Métodos públicos
    void initialize();
    void setDeviceManager(DeviceManager* deviceMgr);
    void handleNewConnections();
    void processClientMessages();
    void checkModuleHeartbeats();
    void sendHeartbeatToClients();
    void sendCommandToModule(const String& moduleId, const String& command);
    void sendWelcomeMessage(int clientIndex);
    void sendAuthSuccessMessage(int clientIndex, const String& macAddress, const String& apiKey);
    void sendToAllClients(const String& message);
    void sendToClient(int clientIndex, const String& message);
    String getClientIP(int clientIndex);
    int getConnectedClientsCount();
    
    // Getters para otros managers
    WiFiClient* getClients() { return mqttClients; }
    bool* getClientConnections() { return clientConnected; }
    
private:
    // Referencias a otros managers
    WiFiManager* wifiManager;
    DeviceManager* deviceManager;
    
    // Configuración MQTT Broker
    WiFiServer mqttServer;
    WiFiClient mqttClients[MAX_CLIENTS];
    bool clientConnected[MAX_CLIENTS];
    unsigned long lastHeartbeatSent[MAX_CLIENTS];
    
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