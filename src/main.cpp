#include <Arduino.h>
#include <WiFi.h> // <-- Moved here (required for AP+STA)
#include "config.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "MQTTBrokerManager.h"
#include "DeviceManager.h"
#include "EEPROMManager.h"



// Managers del sistema
WiFiManager* wifiManager;
WebServerManager* webServerManager;
MQTTBrokerManager* mqttBrokerManager;
DeviceManager* deviceManager;
EEPROMManager* eepromManager;

void setup() {
    // Inicializar Serial para ESP32-C3 con USB CDC
    Serial.begin(SERIAL_BAUDRATE);
    
    // Esperar a que se establezca la conexión USB CDC
    while (!Serial && millis() < 5000) {
        delay(10);
    }
    
    delay(500);  // Estabilización adicional
    
    Serial.println();
    Serial.println("==============================================");
    Serial.println("🚀 Iniciando Broker MQTT Modular");
    Serial.println("==============================================");
    
    // Inicializar managers en orden
    wifiManager = new WiFiManager();
    wifiManager->initialize();

    // --- Mantener AP y (opcionalmente) conectar como STA a la misma red ---
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    Serial.print("🔌 Access Point activo: ");
    Serial.println(AP_SSID);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP().toString());

    // Si ENABLE_STATION_MODE == true intentará conectar como cliente a tu red
    if (ENABLE_STATION_MODE) {
      Serial.print("🌐 Intentando conectar como STA a: ");
      Serial.println(WIFI_SSID);

      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

      unsigned long start = millis();
      const unsigned long timeout = 15000; // 15s máximo (ajustable)
      while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
        delay(500);
        Serial.print(".");
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("✅ Conectado a WiFi. STA IP: ");
        Serial.println(WiFi.localIP().toString());
        Serial.println("➡️ Puedes acceder al dashboard desde la IP STA o desde la IP del AP.");
      } else {
        Serial.println();
        Serial.println("⚠️ No se pudo conectar a la WiFi como STA. Manteniendo AP activo.");
      }
    }
    // --- Fin nuevo bloque ---
    
    mqttBrokerManager = new MQTTBrokerManager(wifiManager, nullptr);  // Temporal sin DeviceManager

    eepromManager = new EEPROMManager(); // Instancia antes de DeviceManager
    eepromManager->begin();
    deviceManager = new DeviceManager(wifiManager, mqttBrokerManager, eepromManager); // Pasa el puntero
    deviceManager->initialize();
    
    // Ahora conectar DeviceManager con MQTTBrokerManager
    mqttBrokerManager->setDeviceManager(deviceManager);
    mqttBrokerManager->initialize();
    
    webServerManager = new WebServerManager(wifiManager, deviceManager);
    webServerManager->initialize();
    

    
    Serial.println("✅ Sistema modular listo");
    Serial.println("==============================================");
}

void loop() {
    // Manejar nuevas conexiones MQTT
    mqttBrokerManager->handleNewConnections();
    
    // Procesar mensajes de clientes conectados
    mqttBrokerManager->processClientMessages();
    
    // Enviar heartbeat a clientes para mantener conexión TCP
    mqttBrokerManager->sendHeartbeatToClients();
    
    // Verificar heartbeats de módulos
    mqttBrokerManager->checkModuleHeartbeats();
    
    // Procesar comandos del sistema
    deviceManager->processSystemCommands();
    
    // Manejar peticiones del servidor web
    webServerManager->handleClient();
    
    delay(10);
}

// Elimina las siguientes funciones de main.cpp:
// void EEPROMManager::writeTestByte(uint8_t value) { ... }
// uint8_t EEPROMManager::readTestByte() { ... }