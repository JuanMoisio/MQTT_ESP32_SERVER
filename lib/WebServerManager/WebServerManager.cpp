#include "WebServerManager.h"
#include "WiFiManager.h"
#include "DeviceManager.h"
#include "MQTTBrokerManager.h"

WebServerManager::WebServerManager(WiFiManager* wifiMgr, DeviceManager* deviceMgr) 
    : webServer(80), wifiManager(wifiMgr), deviceManager(deviceMgr), mqttBrokerManager(nullptr) {
}

// Nuevo setter para inyectar MQTTBrokerManager después de la construcción
void WebServerManager::setMQTTBrokerManager(MQTTBrokerManager* mgr) {
    this->mqttBrokerManager = mgr;
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

    // Debug endpoint: estado completo para inspección remota
    webServer.on("/api/debug/state", HTTP_GET, [this]() {
        Serial.println("[WebServer] /api/debug/state called");
        String responseStr = deviceManager->getDebugJSON();
        webServer.send(200, "application/json", responseStr);
    });

    // --- NUEVO: Endpoint para desregistrar módulo ---
    webServer.on("/api/modules/deregister", HTTP_POST, [this]() {
        DynamicJsonDocument response(512);
        String moduleId = "";

        // Aceptar moduleId y module_id (form)
        if (webServer.hasArg("moduleId")) {
            moduleId = webServer.arg("moduleId");
        } else if (webServer.hasArg("module_id")) {
            moduleId = webServer.arg("module_id");
        } else {
            // Intentar parsear JSON desde body (moduleId o module_id)
            String body = webServer.arg("plain");
            if (body.length() > 0) {
                DynamicJsonDocument doc(512);
                DeserializationError err = deserializeJson(doc, body);
                if (!err) {
                    if (doc.containsKey("moduleId")) {
                        moduleId = String((const char*)doc["moduleId"]);
                    } else if (doc.containsKey("module_id")) {
                        moduleId = String((const char*)doc["module_id"]);
                    }
                }
            }
        }

        if (moduleId.length() == 0) {
            response["success"] = false;
            response["message"] = "moduleId faltante";
        } else {
            bool ok = deviceManager->deregisterModule(moduleId);
            response["success"] = ok;
            response["message"] = ok ? "Módulo eliminado correctamente" : "No se encontró el módulo";
            response["moduleId"] = moduleId;
        }

        String responseStr;
        serializeJson(response, responseStr);
        webServer.send(200, "application/json", responseStr);
    });

    // Endpoint para consultar capacidades de un módulo específico (query param)
    webServer.on("/api/modules/capabilities", HTTP_GET, [this]() {
        if (webServer.hasArg("moduleId")) {
            String moduleId = webServer.arg("moduleId");
            handleModuleCapabilities(moduleId);
        } else {
            webServer.send(400, "application/json", "{\"success\":false,\"message\":\"moduleId faltante\"}");
        }
    });
    // Endpoint para consultar capacidades de un módulo específico (path variable)
    webServer.onNotFound([this]() {
        String url = webServer.uri();
        if (webServer.method() == HTTP_GET && url.startsWith("/api/modules/") && url.endsWith("/capabilities")) {
            int start = String("/api/modules/").length();
            int end = url.length() - String("/capabilities").length();
            String moduleId = url.substring(start, end);
            if (moduleId.length() > 0) {
                handleModuleCapabilities(moduleId);
                return;
            }
        }
        webServer.send(404, "text/plain", "Not found");
    });

    // Endpoint fijo para acciones de módulo
    webServer.on("/api/modules/action", HTTP_POST, [this]() {
        handleModuleAction();
    });
    
    // Ruta: borrar dispositivo desde Dashboard (vista) -> usar eliminación visual (no persistente)
    webServer.on("/api/dashboard/delete_device", HTTP_POST, [this]() {
        DynamicJsonDocument response(512);

        String macAddress = "";
        // Prefer form param
        if (webServer.hasArg("macAddress")) {
            macAddress = webServer.arg("macAddress");
        } else {
            // Intentar parsear JSON desde body
            String body = webServer.arg("plain");
            if (body.length() > 0) {
                DynamicJsonDocument doc(512);
                DeserializationError err = deserializeJson(doc, body);
                if (!err && doc.containsKey("macAddress")) {
                    macAddress = String((const char*)doc["macAddress"]);
                }
            }
        }

        if (macAddress.length() > 0) {
            deviceManager->removeDeviceVisual(macAddress); // <<-- cambio: solo visual
            Serial.println("[WebServer] Petición dashboard delete -> removeDeviceVisual: " + macAddress);

            response["success"] = true;
            response["message"] = "Dispositivo eliminado (visual)";
            response["macAddress"] = macAddress;
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
        DynamicJsonDocument response(512);

        String macAddress = "";
        // Prefer form param directo
        if (webServer.hasArg("macAddress")) {
            macAddress = webServer.arg("macAddress");
        }

        // Si no vino MAC, intentar obtener moduleId/module_id y mapear a MAC
        String moduleId = "";
        if (macAddress.length() == 0) {
            if (webServer.hasArg("moduleId"))       moduleId = webServer.arg("moduleId");
            else if (webServer.hasArg("module_id")) moduleId = webServer.arg("module_id");
        }

        // Parsear JSON si hace falta
        if (macAddress.length() == 0 && moduleId.length() == 0) {
            String body = webServer.arg("plain");
            if (body.length() > 0) {
                DynamicJsonDocument doc(512);
                DeserializationError err = deserializeJson(doc, body);
                if (!err) {
                    if (doc.containsKey("macAddress")) {
                        macAddress = String((const char*)doc["macAddress"]);
                    } else if (doc.containsKey("moduleId")) {
                        moduleId = String((const char*)doc["moduleId"]);
                    } else if (doc.containsKey("module_id")) {
                        moduleId = String((const char*)doc["module_id"]);
                    }
                }
            }
        }

        // Si tengo moduleId pero no MAC, lo resuelvo con DeviceManager
        if (macAddress.length() == 0 && moduleId.length() > 0) {
            ModuleInfo* mod = deviceManager->getModuleById(moduleId);
            if (mod) macAddress = mod->macAddress;
        }

        if (macAddress.length() > 0) {
            bool ok = deviceManager->removeDevice(macAddress); // <<-- persistente (modifica EEPROM)
            Serial.println("[WebServer] Petición modules delete -> removeDevice (persist): " + macAddress + " result=" + String(ok));

            if (ok) {
                response["success"] = true;
                response["message"] = "Dispositivo eliminado (persistente)";
            } else {
                response["success"] = false;
                response["message"] = "Dispositivo no encontrado";
            }
            response["macAddress"] = macAddress;
        } else {
            response["success"] = false;
            response["message"] = "MAC address faltante (no se pudo resolver desde moduleId)";
        }

        String responseStr;
        serializeJson(response, responseStr);
        webServer.send(200, "application/json", responseStr);
    });

} // end setupRoutes()

// --- NUEVO: Procesa acción para módulo y reenvía por MQTT ---
void WebServerManager::handleModuleAction() {
    DynamicJsonDocument doc(1024);
    String body = webServer.arg("plain");
    DeserializationError err = deserializeJson(doc, body);
    if (err) { webServer.send(400, "application/json", "{\"success\":false,\"message\":\"JSON inválido\"}"); return; }

    String moduleId = doc["moduleId"] | "";
    String action   = doc["action"]   | "";
    JsonVariant params = doc["params"]; // puede ser null

    if (moduleId == "" || action == "") {
        webServer.send(400, "application/json", "{\"success\":false,\"message\":\"moduleId o acción faltante\"}");
        return;
    }

    Serial.println("[BROKER][MQTT] Enviando comando:");
    Serial.println("  moduleId: " + moduleId);
    Serial.println("  action: " + action);
    Serial.print  ("  params: ");
    serializeJson(params, Serial); Serial.println();

    bool ok = false;
    if (mqttBrokerManager) {
        // >>> Reenvío REAL al broker, con params <<<
        ok = mqttBrokerManager->sendCommandToModule(moduleId, action, params);
    }

    DynamicJsonDocument resp(256);
    resp["success"] = ok;
    resp["message"] = ok ? "Acción enviada a módulo por broker" : "No se pudo enviar la acción (broker no disponible)";
    resp["moduleId"] = moduleId;
    resp["action"]   = action;
    String respStr; serializeJson(resp, respStr);
    webServer.send(200, "application/json", respStr);
}

void WebServerManager::handleRoot() {
    webServer.send(200, "text/html", getAdminHTML());
}

void WebServerManager::handleAdmin() {
    webServer.send(200, "text/html", getAdminHTML());
}

void WebServerManager::handleSystemInfo() {
    DynamicJsonDocument response(256);
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
    DynamicJsonDocument response(256);
    
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
    DynamicJsonDocument response(512);
    
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
    DynamicJsonDocument response(256);
    
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
    DynamicJsonDocument response(512);
    
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
    DynamicJsonDocument response(256);
    
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

void WebServerManager::handleModuleCapabilities(const String& moduleId) {
    DynamicJsonDocument response(512);
    ModuleInfo* module = deviceManager->getModuleById(moduleId);
    if (module) {
        response["success"] = true;
        response["moduleId"] = module->moduleId;
        response["capabilities"] = module->capabilities;
        response["moduleType"] = module->moduleType;
    } else {
        response["success"] = false;
        response["message"] = "Módulo no encontrado";
    }
    String responseStr;
    serializeJson(response, responseStr);
    webServer.send(200, "application/json", responseStr);
}