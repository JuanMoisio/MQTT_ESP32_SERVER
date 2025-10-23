#include "WebServerManager.h"
#include "WiFiManager.h"
#include "DeviceManager.h"

WebServerManager::WebServerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr) 
    : webServer(80), wifiManager(wifiMgr), deviceManager(deviceMgr) {
}

void WebServerManager::initialize() {
    Serial.println("🌐 Inicializando Web Server Manager...");
    setupRoutes();
    webServer.begin();
    Serial.println("✅ Servidor web iniciado en http://" + wifiManager->getAPIP());
}

void WebServerManager::handleClient() {
    webServer.handleClient();
}

void WebServerManager::setupRoutes() {
    // Rutas principales
    webServer.on("/", HTTP_GET, [this]() { handleRoot(); });
    webServer.on("/admin", HTTP_GET, [this]() { handleAdmin(); });
    
    // APIs
    webServer.on("/api/system-info", HTTP_GET, [this]() { handleSystemInfo(); });
    webServer.on("/api/login", HTTP_POST, [this]() { handleLogin(); });
    webServer.on("/api/devices", HTTP_GET, [this]() { handleDevices(); });
    webServer.on("/api/add-device", HTTP_POST, [this]() { handleAddDevice(); });
    webServer.on("/api/remove-device", HTTP_POST, [this]() { handleRemoveDevice(); });
    webServer.on("/api/request-mac", HTTP_POST, [this]() { handleRequestMac(); });
    webServer.on("/api/get-mac", HTTP_GET, [this]() { handleGetMac(); });
    webServer.on("/api/modules", HTTP_GET, [this]() { handleModules(); });
    webServer.on("/api/stats", HTTP_GET, [this]() { handleStats(); });
    webServer.on("/api/unregistered-devices", HTTP_GET, [this]() { handleUnregisteredDevices(); });
    webServer.on("/api/scan-devices", HTTP_POST, [this]() { handleScanDevices(); });
    webServer.on("/api/scan-results", HTTP_GET, [this]() { handleScanResults(); });
    
    // Ruta: borrar dispositivo desde Dashboard (vista) -> usar eliminación visual (no persistente)
    webServer.on("/api/dashboard/delete_device", HTTP_POST, [this]() {
        JsonDocument response;
        
        if (webServer.hasArg("macAddress")) {
            String macAddress = webServer.arg("macAddress");
            
            deviceManager->removeDeviceVisual(macAddress); // <<-- cambio: solo visual
            Serial.println("[WebServer] Petición dashboard delete -> removeDeviceVisual: " + macAddress);
            
            response["success"] = true;
            response["message"] = "Dispositivo eliminado (visual)";
        } else {
            response["success"] = false;
            response["message"] = "MAC address faltante";
        }
        
        String responseStr;
        serializeJson(response, responseStr);
        webServer.send(200, "application/json", responseStr);
    });

    // Ruta: borrar dispositivo desde Módulos (administración) -> borrado persistente
    webServer.on("/api/modules/delete_device", HTTP_POST, [this]() {
        JsonDocument response;
        
        if (webServer.hasArg("macAddress")) {
            String macAddress = webServer.arg("macAddress");
            
            bool ok = deviceManager->removeDevice(macAddress); // <<-- persistente (modifica EEPROM)
            Serial.println("[WebServer] Petición modules delete -> removeDevice (persist): " + macAddress + " result=" + String(ok));
            
            if (ok) {
                response["success"] = true;
                response["message"] = "Dispositivo eliminado (persistente)";
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
}

void WebServerManager::handleRoot() {
    webServer.send(200, "text/html", getAdminHTML());
}

void WebServerManager::handleAdmin() {
    webServer.send(200, "text/html", getAdminHTML());
}

void WebServerManager::handleSystemInfo() {
    JsonDocument response;
    response["ap_ip"] = wifiManager->getAPIP();
    response["uptime"] = millis();
    response["free_heap"] = ESP.getFreeHeap();
    response["connected_clients"] = wifiManager->getConnectedClients();
    response["registered_modules"] = deviceManager->getRegisteredModulesCount();
    response["authorized_devices"] = deviceManager->getAuthorizedDevicesCount();
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleLogin() {
    JsonDocument response;
    
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
}

void WebServerManager::handleDevices() {
    String responseStr = deviceManager->getDevicesJSON();
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleAddDevice() {
    JsonDocument response;
    
    if (webServer.hasArg("macAddress") && 
        webServer.hasArg("deviceType") &&
        webServer.hasArg("description")) {
        
        String macAddress = webServer.arg("macAddress");
        String deviceType = webServer.arg("deviceType");
        String description = webServer.arg("description");
        
        String apiKey = deviceManager->addDevice(macAddress, deviceType, description);
        
        if (apiKey != "") {
            response["success"] = true;
            response["message"] = "Dispositivo agregado exitosamente";
            response["apiKey"] = apiKey;
            Serial.println("✅ Dispositivo agregado: " + macAddress + " (" + deviceType + ")");
        } else {
            response["success"] = false;
            response["message"] = "Error agregando dispositivo";
        }
    } else {
        response["success"] = false;
        response["message"] = "Parámetros faltantes";
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleRemoveDevice() {
    JsonDocument response;
    
    if (webServer.hasArg("macAddress")) {
        String macAddress = webServer.arg("macAddress");
        
        if (deviceManager->removeDevice(macAddress)) {
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
}

void WebServerManager::handleRequestMac() {
    JsonDocument response;
    
    Serial.println("🌐 API consulta recibida desde web");
    
    int connectedCount = deviceManager->getConnectedClientsCount();
    Serial.println("📊 Clientes TCP conectados: " + String(connectedCount));
    
    // NO hacer requestDiscovery() - usar dispositivos ya detectados
    // deviceManager->requestDiscovery();
    
    response["success"] = true;
    response["message"] = "Consultando dispositivos ya conectados";
    response["connected_clients"] = connectedCount;
    response["server_ip"] = wifiManager->getAPIP();
    response["server_port"] = 1883;
    response["debug_info"] = "Dispositivos detectados al conectarse - ver /api/get-mac";
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleGetMac() {
    String responseStr = deviceManager->getLastMacJSON();
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleModules() {
    String responseStr = deviceManager->getModulesJSON();
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleStats() {
    String responseStr = deviceManager->getStatsJSON();
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleUnregisteredDevices() {
    String responseStr = deviceManager->getUnregisteredDevicesJSON();
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleScanDevices() {
    JsonDocument response;
    
    Serial.println("🔍 API scan-devices llamada desde web");
    
    // Hacer scan pero sin limpiar dispositivos ya conectados
    deviceManager->startScan();
    
    response["success"] = true;
    response["message"] = "Scan actualizado - dispositivos conectados mantenidos";
    response["current_scan_count"] = deviceManager->getScannedCount();
    
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
}

void WebServerManager::handleScanResults() {
    String responseStr = deviceManager->getScanResultsJSON();
    webServer.send(200, "application/json", responseStr);
}