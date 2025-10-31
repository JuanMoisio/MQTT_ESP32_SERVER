#include "DeviceManager.h"
#include "WiFiManager.h"
#include "MQTTBrokerManager.h"
#include <algorithm>

DeviceManager::DeviceManager(WiFiManager* wifiMgr, MQTTBrokerManager* mqttMgr, EEPROMManager* eepromMgr) 
    : wifiManager(wifiMgr), mqttBrokerManager(mqttMgr), eepromManager(eepromMgr) {
    // Inicializar variables
    lastRequestedMAC = "";
    lastRequestedDeviceType = "";
    macRequestTime = 0;
    isScanMode = false;
    
    // Configuración inicial
    config.discoveryMode = true;
    config.heartbeatInterval = HEARTBEAT_INTERVAL;
}

void DeviceManager::initialize() {
    Serial.println("📱 Inicializando Device Manager...");
    if (eepromManager) {
        eepromManager->begin(); // Inicializa EEPROM si existe
        eepromManager->compactDevices(); // Recompactar en arranque para dejar datos consistentes
    }
    
    // Inicializar tipos de módulos permitidos
    for (int i = 0; i < NUM_ALLOWED_TYPES; i++) {
        config.allowedModuleTypes.push_back(String(ALLOWED_MODULE_TYPES[i]));
    }
    
    // Cargar dispositivos autorizados
    loadAuthorizedDevices();
    
    Serial.println("📊 Dispositivos registrados: " + String(authorizedDevices.size()));
    Serial.println("✅ Device Manager listo");
}

String DeviceManager::addDevice(const String& macAddress, const String& deviceType, const String& description) {
    // Generar API key
    String apiKey = generateAPIKey();

    // Crear dispositivo
    AuthorizedDevice newDevice;
    newDevice.macAddress = macAddress;
    newDevice.deviceType = deviceType;
    newDevice.apiKey = apiKey;
    newDevice.description = description;
    newDevice.isActive = true;
    newDevice.isConnected = false;
    newDevice.clientIndex = -1;
    newDevice.lastSeen = 0;
    newDevice.currentIP = "";

    // Agregar a la lista
    authorizedDevices[macAddress] = newDevice;

    // Guardar en EEPROM inmediatamente después del registro
    DeviceInfo info;
    strncpy(info.mac, macAddress.c_str(), sizeof(info.mac));
    strncpy(info.type, deviceType.c_str(), sizeof(info.type));
    strncpy(info.apiKey, apiKey.c_str(), sizeof(info.apiKey));
    strncpy(info.desc, description.c_str(), sizeof(info.desc));
    int index = authorizedDevices.size() - 1; // Usa el último índice disponible
    eepromManager->saveDevice(info, index);

    // Verificar si el dispositivo ya está conectado (buscarlo en scannedDevices)
    for (auto& scannedDevice : scannedDevices) {
        if (scannedDevice.macAddress == macAddress) {
            Serial.println("🔄 Dispositivo recién registrado ya está conectado - actualizando estado...");
            
            // Buscar su clientIndex en clientes conectados
            if (mqttBrokerManager) {
                // If we know the clientIndex from the scannedDevice, notify that client directly
                int targetIndex = scannedDevice.clientIndex;
                if (targetIndex >= 0) {
                    // Send auth_success to the specific client
                    mqttBrokerManager->sendAuthSuccessMessage(targetIndex, macAddress, apiKey);
                    Serial.println("📤 Notificación de registro enviada al cliente index: " + String(targetIndex));
                    // Update authorized device clientIndex as well
                    authorizedDevices[macAddress].clientIndex = targetIndex;
                    
                    // Also send explicit module_credentials payload as a fallback
                    eepromManager->begin();
                    DynamicJsonDocument creds(256);
                    creds["type"] = "module_credentials";
                    creds["module_id"] = scannedDevice.moduleId;
                    creds["mac_address"] = macAddress;
                    creds["api_key"] = apiKey;
                    creds["message"] = "Credenciales enviadas tras registro manual";
                    creds["timestamp"] = millis();
                    String credsStr;
                    serializeJson(creds, credsStr);
                    mqttBrokerManager->sendToClient(targetIndex, credsStr);
                    Serial.println("📤 module_credentials enviado al cliente index: " + String(targetIndex));
                } else {
                    // Fallback: broadcast registration_success
                    DynamicJsonDocument notification(256);
                    notification["type"] = "registration_success";
                    notification["mac_address"] = macAddress;
                    notification["message"] = "Dispositivo registrado exitosamente";
                    notification["api_key"] = apiKey;
                    
                    String notificationStr;
                    serializeJson(notification, notificationStr);
                    
                    mqttBrokerManager->sendToAllClients(notificationStr);
                    Serial.println("📤 Notificación de registro enviada a todos los clientes (broadcast)");
                }
             }
            
            // Actualizar estado del dispositivo recién registrado
            authorizedDevices[macAddress].isConnected = true;
            authorizedDevices[macAddress].lastSeen = millis();
            
            break;
        }
    }
    
    // Guardar todos los dispositivos en EEPROM (opcional, si quieres persistir toda la lista)
    saveAuthorizedDevices();

    return apiKey;
}

bool DeviceManager::removeDevice(const String& macAddress) {
    if (authorizedDevices.find(macAddress) != authorizedDevices.end()) {
        authorizedDevices.erase(macAddress);
        saveAuthorizedDevices();
        return true;
    }
    return false;
}

// Eliminar solo visualmente: quitar del listado de scannedDevices y marcar el autorizado como desconectado
// NO persiste cambios en EEPROM
void DeviceManager::removeDeviceVisual(const String& macAddress) {
    // Quitar de scannedDevices
    for (auto it = scannedDevices.begin(); it != scannedDevices.end(); ) {
        if (it->macAddress == macAddress) {
            it = scannedDevices.erase(it);
            Serial.println("[DeviceManager] Dispositivo eliminado visualmente de scannedDevices: " + macAddress);
        } else {
            ++it;
        }
    }

    // Si está en authorizedDevices, marcar como desconectado pero NO guardar en EEPROM
    auto authIt = authorizedDevices.find(macAddress);
    if (authIt != authorizedDevices.end()) {
        authIt->second.isConnected = false;
        authIt->second.clientIndex = -1;
        authIt->second.currentIP = "";
        Serial.println("[DeviceManager] Dispositivo marcado como desconectado (visual only): " + macAddress);
    } else {
        Serial.println("[DeviceManager] removeDeviceVisual: MAC no encontrada en authorizedDevices: " + macAddress);
    }
}

bool DeviceManager::authenticateDevice(const String& macAddress, const String& apiKey) {
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

bool DeviceManager::updateModuleHeartbeat(const String& moduleId) {
    if (registeredModules.find(moduleId) != registeredModules.end()) {
        registeredModules[moduleId].lastHeartbeat = millis();
        return true;
    }
    return false;
}

bool DeviceManager::isModuleRegistered(const String& moduleId) {
    return registeredModules.find(moduleId) != registeredModules.end();
}

void DeviceManager::setDiscoveryMode(bool mode) {
    config.discoveryMode = mode;
    Serial.print("Modo discovery: ");
    Serial.println(config.discoveryMode ? "ON" : "OFF");
}

void DeviceManager::requestDiscovery() {
    Serial.println("🔍 DISCOVERY: Buscando dispositivos en la red local...");
    
    // Limpiar información anterior para nueva búsqueda
    lastRequestedMAC = "";
    lastRequestedDeviceType = "";
    macRequestTime = 0;
    
    // Marcar tiempo de solicitud
    macRequestTime = millis();
    
    if (isScanMode) {
        Serial.println("⏳ Esperando respuesta de TODOS los dispositivos conectados...");
    } else {
        Serial.println("⏳ Esperando respuesta de dispositivos NO registrados...");
    }
}

void DeviceManager::startScan() {
    Serial.println("🔍 Iniciando scan de dispositivos...");
    
    // NO limpiar scannedDevices automáticamente
    // Los dispositivos se mantienen hasta que se desconecten explícitamente
    // o se limpien manualmente desde la web
    
    // Marcar que estamos en modo scan
    isScanMode = true;
    
    // Iniciar discovery para encontrar dispositivos adicionales
    requestDiscovery();
}

String DeviceManager::generateAPIKey() {
    // Generar API key usando MAC + timestamp + random
    String mac = WiFi.macAddress();
    String timestamp = String(millis());
    String randomNum = String(esp_random());
    
    String combined = mac + timestamp + randomNum + "DEPOSITO_AUTH_v1";
    String hash = sha256Hash(combined);
    
    // Tomar los primeros 32 caracteres del hash
    return hash.substring(0, 32);
}

String DeviceManager::sha256Hash(const String& input) {
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

void DeviceManager::loadAuthorizedDevices() {
    // Cargar desde EEPROM
    Serial.println("💾 Cargando dispositivos autorizados desde EEPROM...");
    authorizedDevices.clear();
    auto eepromDevices = eepromManager->loadDevices();

    // Nuevo logging detallado sobre lo que devuelve EEPROM
    Serial.print("[DeviceManager] eepromManager->loadDevices() returned count: ");
    Serial.println(eepromDevices.size());
    int idx = 0;
    for (const auto& pair : eepromDevices) {
        Serial.print("  [EEPROM] entry "); Serial.print(idx++);
        Serial.print(" key='"); Serial.print(pair.first); Serial.print("' ");
        Serial.print("| mac='"); Serial.print(pair.second.macAddress); Serial.print("'");
        Serial.print(" | type='"); Serial.print(pair.second.deviceType); Serial.print("'");
        Serial.print(" | apiKey='"); Serial.print(pair.second.apiKey ? pair.second.apiKey : String("")); Serial.print("'");
        Serial.print(" | desc='"); Serial.print(pair.second.description); Serial.println("'");
    }

    int loaded = 0;
    for (const auto& pair : eepromDevices) {
        // Saltar entradas con clave vacía o MAC inválida
        if (pair.first.length() < 5) {
            Serial.print("[DeviceManager] Skipping entry with short/empty key: '");
            Serial.print(pair.first);
            Serial.println("'");
            continue;
        }
        if (!eepromManager->isMacValid(pair.second.macAddress)) {
            Serial.print("[DeviceManager] Skipping entry with invalid MAC: ");
            Serial.println(pair.second.macAddress);
            continue;
        }

        AuthorizedDevice dev;
        dev.macAddress = pair.second.macAddress;
        dev.deviceType = pair.second.deviceType;
        dev.apiKey = pair.second.apiKey;
        dev.description = pair.second.description;
        dev.isActive = true;
        dev.isConnected = false;
        dev.clientIndex = -1;
        dev.lastSeen = 0;
        dev.currentIP = "";
        authorizedDevices[pair.first] = dev;

        Serial.print("[DeviceManager] Dispositivo cargado: ");
        Serial.print(dev.macAddress);
        Serial.print(" tipo: ");
        Serial.print(dev.deviceType);
        Serial.print(" apiKey: ");
        Serial.print(dev.apiKey);
        Serial.print(" desc: ");
        Serial.println(dev.description);
        loaded++;
    }

    Serial.print("[DeviceManager] loadAuthorizedDevices -> loaded: ");
    Serial.print(loaded);
    Serial.print(" entries, authorizedDevices.size(): ");
    Serial.println(authorizedDevices.size());

    if (loaded == 0) {
        Serial.println("📱 Sistema de dispositivos autorizados inicializado (vacío)");
    } else {
        Serial.print("📱 Dispositivos autorizados cargados desde EEPROM: ");
        Serial.println(loaded);
    }
}

void DeviceManager::saveAuthorizedDevices() {
    // Guardar en EEPROM
    Serial.println("💾 Guardando dispositivos autorizados en EEPROM...");
    std::map<String, DeviceRecord> eepromDevices;
    for (const auto& pair : authorizedDevices) {
        DeviceRecord rec;
        rec.macAddress = pair.second.macAddress;
        rec.deviceType = pair.second.deviceType;
        rec.apiKey = pair.second.apiKey;
        rec.description = pair.second.description;
        eepromDevices[pair.first] = rec;
    }
    eepromManager->saveDevices(eepromDevices);
    Serial.print("💾 Dispositivos autorizados guardados en EEPROM: ");
    Serial.println(eepromDevices.size());
}

// Handlers de mensajes
void DeviceManager::handleMACResponse(int clientIndex, JsonDocument& doc) {
    String moduleId   = doc["module_id"]   | "";
    String macAddress = doc["mac_address"] | "";
    String deviceType = doc["device_type"] | "";
    
    Serial.println("📡 Respuesta MAC recibida: " + macAddress + " (Tipo: " + deviceType + ") - ModuleID: " + moduleId);
    
    // Mapear tipos de dispositivo para compatibilidad con web
    String webDeviceType = deviceType;
    if (deviceType == "rfid_reader") {
        webDeviceType = "rfid";
    } else if (deviceType == "fingerprint_scanner") {
        webDeviceType = "fingerprint";
    }
    
    // SIEMPRE agregar/actualizar en la lista de escaneo (preserva clientIndex si viene)
    addToScannedDevices(macAddress, webDeviceType, moduleId, clientIndex);
    
    // Verificar si el dispositivo YA está registrado (autorizado)
    bool isAlreadyRegistered = (authorizedDevices.find(macAddress) != authorizedDevices.end());
    
    if (isAlreadyRegistered) {
        // Dispositivo ya autorizado - conectar y actualizar estado
        Serial.println("✅ MAC " + macAddress + " (" + deviceType + ") YA está registrada - conectando automáticamente");
        
        AuthorizedDevice& device = authorizedDevices[macAddress];
        device.isConnected = true;
        device.lastSeen    = millis();
        device.clientIndex = clientIndex;

        // Intentar obtener IP remota desde MQTTBrokerManager
        if (mqttBrokerManager) {
            String ip = mqttBrokerManager->getClientIP(clientIndex);
            device.currentIP = ip;
            Serial.println("📌 Actualizado currentIP para " + macAddress + " -> " + ip);
        }
        
        // Enviar mensaje de autenticación exitosa al dispositivo registrado
        if (mqttBrokerManager) {
            mqttBrokerManager->sendAuthSuccessMessage(clientIndex, macAddress, device.apiKey);
        }

        // 🔘 AUTO-REGISTRO DEL MÓDULO en registeredModules (clave para /api/modules)
        if (moduleId.length() > 0) {
            auto it = registeredModules.find(moduleId);
            if (it == registeredModules.end()) {
                // Crear nueva entrada del módulo
                ModuleInfo mi;
                mi.moduleId         = moduleId;
                mi.moduleType       = webDeviceType; // 'fingerprint' / 'rfid' / ...
                mi.capabilities     = "";            // se puede actualizar luego con actions_response
                mi.lastHeartbeat    = millis();
                mi.isActive         = true;
                mi.isAuthenticated  = true;
                mi.macAddress       = macAddress;
                
                registeredModules[moduleId] = mi;
                Serial.println("✅ AUTO-REGISTER: módulo agregado por MAC response -> " + moduleId);
            } else {
                // Refrescar estado del módulo existente
                ModuleInfo& mi = it->second;
                mi.moduleType      = webDeviceType; // por si cambió el mapeo
                mi.lastHeartbeat   = millis();
                mi.isActive        = true;
                mi.isAuthenticated = true;
                mi.macAddress      = macAddress;    // asegura vínculo MAC<->módulo
                Serial.println("🔄 AUTO-REGISTER: módulo ya existía; estado refrescado -> " + moduleId);
            }
        } else {
            Serial.println("⚠️ AUTO-REGISTER omitido: module_id vacío en MAC response");
        }

    } else {
        // Dispositivo nuevo - disponible para registro manual
        Serial.println("✅ MAC " + macAddress + " (" + deviceType + ") NO está registrada - disponible para registro");
        lastRequestedMAC        = macAddress;
        lastRequestedDeviceType = webDeviceType;
        macRequestTime          = millis();
    }
}


void DeviceManager::handleDeviceInfoResponse(int clientIndex, JsonDocument& doc) {
    String moduleId = doc["module_id"];
    String macAddress = doc["mac_address"];
    String deviceType = doc["device_type"];
    
    Serial.println("📡 Respuesta de info de dispositivo: " + macAddress + " (Tipo: " + deviceType + ") - ModuleID: " + moduleId);
    
    // Mapear tipos de dispositivo
    String webDeviceType = deviceType;
    if (deviceType == "rfid_reader") {
        webDeviceType = "rfid";
    } else if (deviceType == "fingerprint_scanner") {
        webDeviceType = "fingerprint";
    }
    
    // Verificar si el dispositivo ya está registrado
    bool isAlreadyRegistered = (authorizedDevices.find(macAddress) != authorizedDevices.end());
    
    // Agregar a la lista de dispositivos escaneados
    addToScannedDevices(macAddress, webDeviceType, moduleId, clientIndex);
    
    // Solo guardar en variables globales si NO está registrado
    if (!isAlreadyRegistered) {
        lastRequestedMAC = macAddress;
        lastRequestedDeviceType = webDeviceType;
        macRequestTime = millis();
    }
}

void DeviceManager::handleDeviceScanResponse(int clientIndex, JsonDocument& doc) {
    String moduleId = doc["module_id"];
    String macAddress = doc["mac_address"];
    String deviceType = doc["device_type"];
    
    Serial.println("📡 Respuesta de scan recibida: " + macAddress + " (" + deviceType + ") - ID: " + moduleId);
    
    // Mapear tipos de dispositivo
    String webDeviceType = deviceType;
    if (deviceType == "rfid_reader") {
        webDeviceType = "rfid";
    } else if (deviceType == "fingerprint_scanner") {
        webDeviceType = "fingerprint";
    }
    
    // Usar la función helper para agregar/actualizar
    addToScannedDevices(macAddress, webDeviceType, moduleId, clientIndex);
}

void DeviceManager::addToScannedDevices(const String& macAddress, const String& deviceType, const String& moduleId, int clientIndex) {
    // Buscar si ya existe en la lista (evitar duplicados)
    bool alreadyExists = false;
    for (auto& existing : scannedDevices) {
        if (existing.macAddress == macAddress) {
            // Actualizar información existente
            existing.deviceType = deviceType;
            existing.moduleId = moduleId;
            existing.timestamp = millis();
            if (clientIndex >= 0) existing.clientIndex = clientIndex;
            alreadyExists = true;
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
        newDevice.clientIndex = clientIndex;
        
        scannedDevices.push_back(newDevice);
        Serial.println("✅ Dispositivo agregado a lista de scan: " + macAddress + " (" + deviceType + ") moduleId=" + moduleId + " clientIndex=" + String(clientIndex));
    }
}

// Nuevo: wrapper público que delega en el método privado addToScannedDevices
void DeviceManager::reportScannedDevice(const String& macAddress, const String& deviceType, const String& moduleId, int clientIndex) {
    // Reutiliza la lógica ya implementada
    addToScannedDevices(macAddress, deviceType, moduleId, clientIndex);
}

// Métodos JSON
String DeviceManager::getDevicesJSON() {
    // DEBUG: mostrar el estado interno antes de construir el JSON
    Serial.print("[DeviceManager] getDevicesJSON - authorizedDevices size: ");
    Serial.println(authorizedDevices.size());
    for (const auto &pair : authorizedDevices) {
        Serial.print("  - key: ");
        Serial.print(pair.first);
        Serial.print(" | mac: ");
        Serial.print(pair.second.macAddress);
        Serial.print(" | type: ");
        Serial.print(pair.second.deviceType);
        Serial.print(" | isActive: ");
        Serial.print(pair.second.isActive ? "YES" : "NO");
        Serial.print(" | isConnected: ");
        Serial.print(pair.second.isConnected ? "YES" : "NO");
        Serial.print(" | clientIndex: ");
        Serial.println(pair.second.clientIndex);
    }
    // También loguear scannedDevices para ver si hay entradas que coincidan
    Serial.print("[DeviceManager] scannedDevices size: ");
    Serial.println(scannedDevices.size());
    for (const auto &sd : scannedDevices) {
        Serial.print("  - scanned mac: ");
        Serial.print(sd.macAddress);
        Serial.print(" | moduleId: ");
        Serial.println(sd.moduleId);
    }

    // Construir JSON con capacidad suficiente
    DynamicJsonDocument response(2048);
    JsonArray devices = response.createNestedArray("devices");

    for (auto& pair : authorizedDevices) {
        JsonObject device = devices.createNestedObject();
        device["macAddress"] = pair.second.macAddress;
        device["deviceType"] = pair.second.deviceType;
        device["apiKey"] = pair.second.apiKey;
        device["description"] = pair.second.description;
        device["isActive"] = pair.second.isActive;
        device["isConnected"] = pair.second.isConnected;
        device["clientIndex"] = pair.second.clientIndex;
        device["lastSeen"] = pair.second.lastSeen;
        device["currentIP"] = pair.second.currentIP;
    }

    response["success"] = true;
    response["total_devices"] = authorizedDevices.size();

    String responseStr;
    serializeJson(response, responseStr);

    // DEBUG: mostrar JSON generado en consola
    Serial.print("[DeviceManager] getDevicesJSON response: ");
    Serial.println(responseStr);

    return responseStr;
}

String DeviceManager::getModulesJSON() {
    // Estimar memoria: base + por módulo
    const size_t BASE = 1024;
    const size_t PER_MODULE = 256; // ajustá si tenés campos extra
    size_t cap = BASE + registeredModules.size() * PER_MODULE;

    DynamicJsonDocument response(cap);
    JsonArray modulesArray = response.createNestedArray("modules");

    Serial.print("[DeviceManager] registeredModules size: ");
    Serial.println(registeredModules.size());

    for (auto& pair : registeredModules) {
        ModuleInfo& module = pair.second;
        JsonObject moduleObj = modulesArray.createNestedObject();
        moduleObj["moduleId"] = module.moduleId;
        moduleObj["moduleType"] = module.moduleType;
        moduleObj["capabilities"] = module.capabilities;  // si es CSV ok; si es lista, llenarla como array
        moduleObj["macAddress"] = module.macAddress;
        moduleObj["isActive"] = module.isActive;
        moduleObj["isAuthenticated"] = module.isAuthenticated;
        moduleObj["lastHeartbeat"] = module.lastHeartbeat;
    }

    String responseStr;
    serializeJson(response, responseStr);
    Serial.print("[DeviceManager] getModulesJSON response: ");
    Serial.println(responseStr);
    return responseStr;
}


String DeviceManager::getStatsJSON() {
    DynamicJsonDocument response(512);
    
    response["registered_devices"] = authorizedDevices.size();
    
     // Calcular dispositivos conectados
     int connectedDevices = 0;
     unsigned long now = millis();
     
     Serial.println("📊 DEBUG: Revisando dispositivos conectados:");
     for (auto& pair : authorizedDevices) {
         AuthorizedDevice& device = pair.second;
         unsigned long timeSince = (now - device.lastSeen);
         
         Serial.println("  - MAC: " + device.macAddress + 
                       " | isConnected: " + String(device.isConnected ? "YES" : "NO") +
                       " | isActive: " + String(device.isActive ? "YES" : "NO") +
                       " | lastSeen: " + String(timeSince) + "ms ago" +
                       " | clientIndex: " + String(device.clientIndex));
        
        // Usar isConnected flag y verificar que lastSeen no sea muy antiguo
        if (device.isConnected && device.isActive && timeSince < 300000) { // 5 minutos
            connectedDevices++;
            Serial.println("    ✅ CONTADO como conectado");
        } else {
            Serial.println("    ❌ NO contado - razón: " + 
                          String(!device.isConnected ? "no conectado" : 
                                !device.isActive ? "no activo" : "muy antiguo"));
        }
    }
    
    response["connected_devices"] = connectedDevices;
    response["depositario_status"] = false;
    response["placa_motores_status"] = false;
    
    String responseStr;
    serializeJson(response, responseStr);
    return responseStr;
}

String DeviceManager::getScanResultsJSON() {
    DynamicJsonDocument response(2048);
    JsonArray devices = response.createNestedArray("devices");

    // Debug: mostrar cuántos dispositivos hay en scannedDevices
    Serial.println("📁 getScanResultsJSON - scannedDevices size: " + String(scannedDevices.size()));
    for (const auto &d : scannedDevices) {
        Serial.println("   - scanned device: " + d.macAddress + " (" + d.deviceType + ") moduleId=" + d.moduleId);
    }
    
    // Desactivar modo scan
    if (isScanMode) {
        isScanMode = false;
    }
    
    for (auto& device : scannedDevices) {
        JsonObject deviceObj = devices.createNestedObject();
        deviceObj["macAddress"] = device.macAddress;
        deviceObj["deviceType"] = device.deviceType;
        deviceObj["moduleId"] = device.moduleId;
        
        // Verificar si está registrado
        bool isRegistered = (authorizedDevices.find(device.macAddress) != authorizedDevices.end());
        deviceObj["isRegistered"] = isRegistered;
    }
    
    response["success"] = true;
    response["total_devices"] = scannedDevices.size();
    
    String responseStr;
    serializeJson(response, responseStr);
    return responseStr;
}

String DeviceManager::getLastMacJSON() {
    DynamicJsonDocument response(256);
    
    // Verificar si la información es reciente (últimos 30 segundos)
    if (lastRequestedMAC != "" && (millis() - macRequestTime) < 30000) {
        response["success"] = true;
        response["macAddress"] = lastRequestedMAC;
        response["deviceType"] = lastRequestedDeviceType;
        response["timestamp"] = macRequestTime;
    } else {
        response["success"] = false;
        if (lastRequestedMAC == "") {
            response["message"] = "No hay MAC disponible - ningún dispositivo no registrado ha respondido";
        } else {
            response["message"] = "Información de MAC expirada (más de 30 segundos)";
        }
    }
    
    String responseStr;
    serializeJson(response, responseStr);
    return responseStr;
}

String DeviceManager::getUnregisteredDevicesJSON() {
    DynamicJsonDocument response(2048);
    JsonArray devices = response.createNestedArray("devices");
    
    // Obtener dispositivos de scan que no están registrados
    for (auto& device : scannedDevices) {
        bool isRegistered = (authorizedDevices.find(device.macAddress) != authorizedDevices.end());
        
        if (!isRegistered) {
            JsonObject deviceObj = devices.createNestedObject();
            deviceObj["macAddress"] = device.macAddress;
            deviceObj["deviceType"] = device.deviceType;
            deviceObj["moduleId"] = device.moduleId;
            deviceObj["timestamp"] = device.timestamp;
            deviceObj["isRegistered"] = false;
        }
    }
    
    response["success"] = true;
    response["total_unregistered"] = devices.size();
    
    String responseStr;
    serializeJson(response, responseStr);
    return responseStr;
}

// Métodos adicionales necesarios
void DeviceManager::handleModuleRegistration(int clientIndex, JsonDocument& doc, WiFiClient* client) {
    String moduleId = doc["module_id"];
    String moduleType = doc["module_type"];
    String capabilities = doc["capabilities"];
    String macAddress = doc["mac_address"];
    String apiKey = doc["api_key"];
    
    Serial.println("📱 Registro de módulo: " + moduleType);
    Serial.println("   MAC: " + macAddress);
    Serial.println("   ModuleID: " + moduleId);
    Serial.println("   ApiKey: " + (apiKey.length() > 0 ? apiKey.substring(0, 8) + "..." : "VACÍO"));
    
    DynamicJsonDocument response(512);
     response["type"] = "registration_response";
     response["response_type"] = "module";
     response["module_id"] = moduleId;
    
    // Verificar autenticación
    bool isAuthenticated = authenticateDevice(macAddress, apiKey);
    
    Serial.println("   🔐 Autenticación: " + String(isAuthenticated ? "EXITOSA" : "FALLIDA"));

    // If not authenticated but the MAC is already authorized, accept the registration
    // regardless of whether the client sent the correct apiKey
    if (!isAuthenticated && authorizedDevices.find(macAddress) != authorizedDevices.end()) {
        Serial.println("🔁 MAC ya autorizada - forzando autenticación exitosa");
        // Use the server-side apiKey for this device
        apiKey = authorizedDevices[macAddress].apiKey;
        isAuthenticated = true;
        
        // Update the authorized device info
        authorizedDevices[macAddress].clientIndex = clientIndex;
        authorizedDevices[macAddress].isConnected = true;
        authorizedDevices[macAddress].lastSeen = millis();
    }

    if (!isAuthenticated) {
        Serial.println("❌ Dispositivo no autorizado - MAC: " + macAddress);
        Serial.println("   Enviando registration_response con error...");
        response["status"] = "error";
        response["message"] = "Dispositivo no autorizado. Debe registrarse manualmente desde el panel de administración.";
    } else {
        // Verificar si el tipo de módulo está permitido
        bool typeAllowed = false;
        for (const String& allowedType : config.allowedModuleTypes) {
            if (moduleType == allowedType) {
                typeAllowed = true;
                break;
            }
        }
        
        if (typeAllowed) {
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
            
            // Actualizar IP y estado del dispositivo autorizado
            if (authorizedDevices.find(macAddress) != authorizedDevices.end()) {
                authorizedDevices[macAddress].currentIP = client->remoteIP().toString();
                authorizedDevices[macAddress].lastSeen = millis();
                authorizedDevices[macAddress].isConnected = true;   // <-- asegurar marcado como conectado
                authorizedDevices[macAddress].clientIndex = clientIndex; // <-- asignar clientIndex
                Serial.println("📌 Actualizado authorizedDevice: isConnected=YES clientIndex=" + String(clientIndex) + " IP=" + authorizedDevices[macAddress].currentIP);
            } else {
                // No estaba autorizado previamente: solo log
                Serial.println("📌 Registro de módulo para MAC no autorizada (no en authorizedDevices): " + macAddress);
            }
            
            response["status"] = "success";
            response["message"] = "Módulo registrado y autenticado exitosamente";
            
            Serial.println("✅ Módulo " + moduleId + " registrado y autenticado");
        } else {
            response["status"] = "error";
            response["message"] = "Tipo de módulo no permitido";
        }
    }
    
    response["timestamp"] = millis();
    
    String responseStr;
    serializeJson(response, responseStr);
    client->println(responseStr);
}

void DeviceManager::handleDeviceRegistration(int clientIndex, JsonDocument& doc) {
    // Este método maneja el registro automático de dispositivos con código de verificación
    // Por simplicidad, lo mantenemos básico por ahora
    Serial.println("📱 Solicitud de registro de dispositivo recibida");
}

void DeviceManager::checkModuleHeartbeats(WiFiClient* clients, bool* clientConnected) {
    unsigned long currentTime = millis();
    
    for (auto& pair : registeredModules) {
        if (pair.second.isActive) {
            unsigned long timeSinceHeartbeat = currentTime - pair.second.lastHeartbeat;
            
            if (timeSinceHeartbeat > config.heartbeatInterval * 2) {
                // Módulo no responde
                pair.second.isActive = false;
                Serial.println("Módulo " + pair.second.moduleId + " marcado como inactivo (sin heartbeat)");
                
                // Además marcar el dispositivo autorizado asociado como desconectado
                String mac = pair.second.macAddress;
                if (mac.length() > 0 && authorizedDevices.find(mac) != authorizedDevices.end()) {
                    authorizedDevices[mac].isConnected = false;
                    authorizedDevices[mac].clientIndex = -1;
                    authorizedDevices[mac].currentIP = "";
                    Serial.println("📌 AuthorizedDevice " + mac + " marcado como desconectado por heartbeat");
                }
            }
        }
    }
}

int DeviceManager::getConnectedClientsCount() {
    // Consultar al MQTTBrokerManager para obtener el count real
    if (mqttBrokerManager) {
        return mqttBrokerManager->getConnectedClientsCount();
    }
    return 0;
}

void DeviceManager::processSystemCommands() {
    // Procesar comandos serie para configuración
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        // Si el comando empieza con 'server:', lo quitamos
        if (command.startsWith("server:")) {
            command = command.substring(7);
            command.trim();
        }
        if (command == "status") {
            printStatus();
        } else if (command == "modules") {
            printRegisteredModules();
        } else if (command == "reset") {
            Serial.println("🔄 Reiniciando ESP32-C3 en 3 segundos...");
            delay(3000);
            ESP.restart();
        } else if (command == "help") {
            Serial.println("=== COMANDOS DISPONIBLES ===");
            Serial.println("status     - Estado del sistema");
            Serial.println("modules    - Listar módulos registrados");
            Serial.println("reset      - Reiniciar servidor");
            Serial.println("help       - Mostrar esta ayuda");
            Serial.println("write_eeprom <num> - Escribe <num> en EEPROM pos 1");
            Serial.println("read_eeprom         - Lee EEPROM pos 1");
            Serial.println("compact_eeprom      - Recompacta EEPROM (reescribe y limpia slots)");
        } else if (command.startsWith("write_eeprom")) {
            int spaceIdx = command.indexOf(' ');
            if (spaceIdx > 0) {
                int val = command.substring(spaceIdx + 1).toInt();
                eepromManager->writeTestByte((uint8_t)val);
            } else {
                Serial.println("Uso: write_eeprom <num>");
            }
        } else if (command == "read_eeprom") {
            eepromManager->readTestByte();
        } else if (command == "compact_eeprom") {
            if (eepromManager) {
                eepromManager->compactDevices();
            }
        }
    }
}

void DeviceManager::printStatus() {
    Serial.println("=== ESTADO DEL DEVICE MANAGER ===");
    Serial.print("📦 Módulos registrados: ");
    Serial.println(registeredModules.size());
    Serial.print("📱 Dispositivos autorizados: ");
    Serial.println(authorizedDevices.size());
    Serial.print("🔍 Modo discovery: ");
    Serial.println(config.discoveryMode ? "ON" : "OFF");
    Serial.println("================================");
}

void DeviceManager::printRegisteredModules() {
    Serial.println("=== MÓDULOS REGISTRADOS ===");
    for (auto& pair : registeredModules) {
        Serial.print("ID: ");
        Serial.print(pair.second.moduleId);
        Serial.print(" | Tipo: ");
        Serial.print(pair.second.moduleType);
        Serial.print(" | Activo: ");
        Serial.print(pair.second.isActive ? "SÍ" : "NO");
        Serial.print(" | MAC: ");
        Serial.println(pair.second.macAddress);
    }
    Serial.println("===========================");
}

ModuleInfo* DeviceManager::getModuleById(const String& moduleId) {
    auto it = registeredModules.find(moduleId);
    if (it != registeredModules.end()) {
        return &(it->second);
    }
    return nullptr;
}

// Elimina un módulo por su moduleId y actualiza el estado del dispositivo autorizado relacionado
bool DeviceManager::deregisterModule(const String& moduleId) {
    // Intentar eliminar como moduleId en registeredModules
    auto it = registeredModules.find(moduleId);
    if (it != registeredModules.end()) {
        String mac = it->second.macAddress;
        registeredModules.erase(it);
        Serial.println("🗑️ Módulo eliminado: " + moduleId);

        // Si el módulo estaba asociado a un dispositivo autorizado, eliminar ese dispositivo
        if (mac.length() > 0 && authorizedDevices.find(mac) != authorizedDevices.end()) {
            authorizedDevices.erase(mac);
            saveAuthorizedDevices(); // Persistir cambio en EEPROM
            Serial.println("🗑️ AuthorizedDevice eliminado (persistente): " + mac);
        }
        return true;
    }

    // Si no existe como módulo, intentar eliminar como MAC (fallback)
    if (authorizedDevices.find(moduleId) != authorizedDevices.end()) {
        bool ok = removeDevice(moduleId); // ya persiste en EEPROM
        Serial.println(ok ? "🗑️ MAC tratada como dispositivo y eliminada: " + moduleId
                          : "❌ Error eliminando dispositivo por MAC: " + moduleId);
        return ok;
    }

    Serial.println("❌ No se encontró módulo o dispositivo para eliminar: " + moduleId);
    return false;
}

String DeviceManager::getDebugJSON() {
    // Construir JSON grande con estado interno para debugging
    DynamicJsonDocument response(16384);

    // Authorized devices
    JsonArray authArr = response.createNestedArray("authorizedDevices");
    for (const auto& pair : authorizedDevices) {
        JsonObject o = authArr.createNestedObject();
        o["key"] = pair.first;
        o["macAddress"] = pair.second.macAddress;
        o["deviceType"] = pair.second.deviceType;
        o["apiKey"] = pair.second.apiKey;
        o["description"] = pair.second.description;
        o["isActive"] = pair.second.isActive;
        o["isConnected"] = pair.second.isConnected;
        o["clientIndex"] = pair.second.clientIndex;
        o["lastSeen"] = pair.second.lastSeen;
        o["currentIP"] = pair.second.currentIP;
    }

    // Registered modules
    JsonArray modArr = response.createNestedArray("registeredModules");
    for (const auto& pair : registeredModules) {
        JsonObject o = modArr.createNestedObject();
        o["moduleId"] = pair.second.moduleId;
        o["moduleType"] = pair.second.moduleType;
        o["capabilities"] = pair.second.capabilities;
        o["macAddress"] = pair.second.macAddress;
        o["isActive"] = pair.second.isActive;
        o["isAuthenticated"] = pair.second.isAuthenticated;
        o["lastHeartbeat"] = pair.second.lastHeartbeat;
    }

    // Scanned devices
    JsonArray scanArr = response.createNestedArray("scannedDevices");
    for (const auto& d : scannedDevices) {
        JsonObject o = scanArr.createNestedObject();
        o["macAddress"] = d.macAddress;
        o["deviceType"] = d.deviceType;
        o["moduleId"] = d.moduleId;
        o["timestamp"] = d.timestamp;
        o["clientIndex"] = d.clientIndex;
    }

    // EEPROM raw (si está el manager)
    if (eepromManager) {
        auto eepromDevices = eepromManager->loadDevices();
        JsonArray eepArr = response.createNestedArray("eepromDevices");
        for (const auto &p : eepromDevices) {
            JsonObject o = eepArr.createNestedObject();
            o["key"] = p.first;
            o["macAddress"] = p.second.macAddress;
            o["deviceType"] = p.second.deviceType;
            o["apiKey"] = p.second.apiKey;
            o["description"] = p.second.description;
        }
        response["eeprom_count"] = (int)eepromDevices.size();
    } else {
        response["eeprom_count"] = 0;
    }

    response["authorized_count"] = (int)authorizedDevices.size();
    response["registered_modules_count"] = (int)registeredModules.size();
    response["scanned_count"] = (int)scannedDevices.size();
    response["success"] = true;

    String out;
    serializeJson(response, out);
    Serial.print("[DeviceManager] getDebugJSON response: ");
    Serial.println(out);
    return out;
}

int DeviceManager::getScannedCount() {
    return scannedDevices.size();
}

// Nuevo: contar módulos registrados (usado por la UI)
int DeviceManager::getRegisteredModulesCount() {
    return registeredModules.size();
}

// Nuevo: contar dispositivos autorizados (usado por la UI)
int DeviceManager::getAuthorizedDevicesCount() {
    return authorizedDevices.size();
}

// Nuevo: marcar dispositivo autorizado como conectado y actualizar IP/lastSeen
void DeviceManager::markDeviceConnected(const String& macAddress, int clientIndex, const String& ip) {
    auto it = authorizedDevices.find(macAddress);
    if (it != authorizedDevices.end()) {
        it->second.isConnected = true;
        it->second.clientIndex = clientIndex;
        it->second.currentIP = ip;
        it->second.lastSeen = millis();
        Serial.println("[DeviceManager] markDeviceConnected -> MAC: " + macAddress + " clientIndex: " + String(clientIndex) + " IP: " + ip);
    } else {
        Serial.println("[DeviceManager] markDeviceConnected: MAC no encontrada en authorizedDevices: " + macAddress);
    }
}