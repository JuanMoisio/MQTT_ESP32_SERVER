#include <Arduino.h>
#include "config.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "MQTTBrokerManager.h"
#include "DeviceManager.h"
#include "EEPROMManager.h"

// Definición de variables globales de configuración
const char* AP_SSID = "DEPOSITO_BROKER";
const char* AP_PASSWORD = "deposito123";
const char* WIFI_SSID = "TU_WIFI_SSID";
const char* WIFI_PASSWORD = "TU_WIFI_PASSWORD";
const bool ENABLE_STATION_MODE = false;

const char* ALLOWED_MODULE_TYPES[] = {
    "rfid_reader",
    "fingerprint_scanner"
};

const int NUM_ALLOWED_TYPES = sizeof(ALLOWED_MODULE_TYPES) / sizeof(ALLOWED_MODULE_TYPES[0]);

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