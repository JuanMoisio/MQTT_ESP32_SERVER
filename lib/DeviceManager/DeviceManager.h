#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <mbedtls/md.h>
#include "../../include/config.h"

// Forward declarations
#include "../EEPROMManager/EEPROMManager.h"
class WiFiManager;
class MQTTBrokerManager;

// Estructuras de datos
struct ModuleInfo {
    String moduleId;
    String moduleType;
    String capabilities;
    unsigned long lastHeartbeat;
    bool isActive;
    bool isAuthenticated;
    String macAddress;
};

struct AuthorizedDevice {
    String macAddress;
    String deviceType;
    String apiKey;
    String description;
    bool isActive;
    bool isConnected;
    int clientIndex;
    unsigned long lastSeen;
    String currentIP;
};

struct ScannedDevice {
    String macAddress;
    String deviceType;
    String moduleId;
    unsigned long timestamp;
    int clientIndex;
};

struct SystemConfig {
    bool discoveryMode;
    int heartbeatInterval;
    std::vector<String> allowedModuleTypes;
};

class DeviceManager {
  public:
    DeviceManager(WiFiManager* wifiMgr, MQTTBrokerManager* mqttMgr, EEPROMManager* eepromMgr);

    // Métodos públicos
    void initialize();
    void printStatus();
    void processSystemCommands();

    // Gestión de dispositivos
    String addDevice(const String& macAddress, const String& deviceType, const String& description);
    bool removeDevice(const String& macAddress);
    // Eliminar solo visualmente: quitar de listados en memoria (scannedDevices y marcar desconectado)
    // NO modifica EEPROM ni llama a saveAuthorizedDevices()
    void removeDeviceVisual(const String& macAddress);
    bool authenticateDevice(const String& macAddress, const String& apiKey);

    // Gestión de módulos
    bool updateModuleHeartbeat(const String& moduleId);
    bool isModuleRegistered(const String& moduleId);
    void checkModuleHeartbeats(WiFiClient* clients, bool* clientConnected);
    bool deregisterModule(const String& moduleId);

    // Discovery y scan
    void requestDiscovery();
    void startScan();
    void setDiscoveryMode(bool mode);

    // Handlers de mensajes
    void handleMACResponse(int clientIndex, JsonDocument& doc);
    void handleDeviceInfoResponse(int clientIndex, JsonDocument& doc);
    void handleDeviceScanResponse(int clientIndex, JsonDocument& doc);
    void handleDeviceRegistration(int clientIndex, JsonDocument& doc);
    void handleModuleRegistration(int clientIndex, JsonDocument& doc, WiFiClient* client);

    // JSON Responses
    String getDevicesJSON();
    String getModulesJSON();
    String getStatsJSON();
    String getUnregisteredDevicesJSON();
    String getScanResultsJSON();
    String getLastMacJSON();
    // Debug: estado completo para inspección remota
    String getDebugJSON();
    // Getters
    int getScannedCount();
    int getRegisteredModulesCount();
    int getAuthorizedDevicesCount();
    int getConnectedClientsCount();

    // Nuevo: marcar un dispositivo autorizado como conectado (usado por MQTTBrokerManager)
    void markDeviceConnected(const String& macAddress, int clientIndex, const String& ip);

    // Nuevo: wrapper público para reportar un dispositivo escaneado (delegará en addToScannedDevices)
    void reportScannedDevice(const String& macAddress, const String& deviceType, const String& moduleId, int clientIndex = -1);

    // Nuevo: obtener MAC conocida a partir de un moduleId (busca en scannedDevices)
    String getMacByModuleId(const String& moduleId);

    String getModuleIdByMac(const String& macAddress);
    ModuleInfo* getModuleById(const String& moduleId);

    // Debug methods
    void printRegisteredModules();

  private:
    // Referencias
    WiFiManager* wifiManager;
    MQTTBrokerManager* mqttBrokerManager;
    EEPROMManager* eepromManager;

    // Datos del sistema
    std::map<String, ModuleInfo> registeredModules;
    std::map<String, AuthorizedDevice> authorizedDevices;
    std::vector<ScannedDevice> scannedDevices;
    SystemConfig config;

    // Variables para consulta de MAC
    String lastRequestedMAC;
    String lastRequestedDeviceType;
    unsigned long macRequestTime;
    bool isScanMode;

    // Constantes
    const String REGISTRATION_PASSWORD = "361500";

    // Métodos privados
    String generateAPIKey();
    String sha256Hash(const String& input);
    void addToScannedDevices(const String& macAddress, const String& deviceType, const String& moduleId, int clientIndex = -1);
    void loadAuthorizedDevices();
    void saveAuthorizedDevices();
};

#endif