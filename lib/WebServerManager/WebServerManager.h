#ifndef WEB_SERVER_MANAGER_H
#define WEB_SERVER_MANAGER_H

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <map>
#include "../../include/config.h"
#include "../../src/web_interface.h"

// Forward declarations
class WiFiManager;
class DeviceManager;

struct AuthorizedDevice; // Forward declaration

class WebServerManager {
public:
    WebServerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr);
    
    // Métodos públicos
    void initialize();
    void handleClient();
    
private:
    // Referencias a otros managers
    WiFiManager* wifiManager;
    DeviceManager* deviceManager;
    
    // Servidor web
    WebServer webServer;
    
    // Credenciales
    const String ADMIN_USER = "admin";
    const String ADMIN_PASS = "deposito123";
    
    // Métodos de inicialización
    void setupRoutes();
    
    // Handlers de rutas
    void handleRoot();
    void handleModuleAction();
    void handleAdmin();
    void handleSystemInfo();
    void handleLogin();
    void handleDevices();
    void handleAddDevice();
    void handleRemoveDevice();
    void handleRequestMac();
    void handleGetMac();
    void handleModules();
    void handleStats();
    void handleUnregisteredDevices();
    void handleScanDevices();
    void handleScanResults();
    void handleModuleCapabilities(const String& moduleId);
};

#endif