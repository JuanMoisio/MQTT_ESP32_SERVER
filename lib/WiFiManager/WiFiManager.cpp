#include "WiFiManager.h"

WiFiManager::WiFiManager() {
    ap_ssid = AP_SSID;
    ap_password = AP_PASSWORD;
    sta_ssid = WIFI_SSID;
    sta_password = WIFI_PASSWORD;
    
    ap_ip = IPAddress(AP_IP_ADDR);
    ap_gateway = IPAddress(AP_GATEWAY_ADDR);
    ap_subnet = IPAddress(AP_SUBNET_ADDR);
}

void WiFiManager::initialize() {
    Serial.println("🌐 Inicializando WiFi Manager...");
    
    // Configurar como Access Point (y opcionalmente Station)
    WiFi.mode(ENABLE_STATION_MODE ? WIFI_AP_STA : WIFI_AP);
    
    setupAccessPoint();
    
    if (ENABLE_STATION_MODE) {
        setupStation();
    }
    
    Serial.println("🚀 WiFi Manager listo!");
}

void WiFiManager::setupAccessPoint() {
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
    }
}

void WiFiManager::setupStation() {
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

String WiFiManager::getAPIP() {
    return WiFi.softAPIP().toString();
}

String WiFiManager::getStationIP() {
    return WiFi.localIP().toString();
}

bool WiFiManager::isStationConnected() {
    return WiFi.status() == WL_CONNECTED;
}

int WiFiManager::getConnectedClients() {
    return WiFi.softAPgetStationNum();
}

void WiFiManager::printStatus() {
    Serial.println("=== ESTADO WiFi ===");
    Serial.print("📶 SSID AP: ");
    Serial.println(ap_ssid);
    Serial.print("🌐 IP AP: ");
    Serial.println(getAPIP());
    Serial.print("👥 Clientes AP: ");
    Serial.println(getConnectedClients());
    
    if (ENABLE_STATION_MODE) {
        Serial.print("🌍 WiFi externo: ");
        if (isStationConnected()) {
            Serial.print("Conectado - IP: ");
            Serial.println(getStationIP());
        } else {
            Serial.println("Desconectado");
        }
    }
    Serial.println("===================");
}