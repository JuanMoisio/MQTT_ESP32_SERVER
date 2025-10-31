#include <Arduino.h>
#include <WiFi.h> 
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
  Serial.begin(SERIAL_BAUDRATE);
  while (!Serial && millis() < 5000) delay(10);
  delay(200);

  Serial.println();
  Serial.println("==============================================");
  Serial.println("🚀 Iniciando Broker MQTT Modular");
  Serial.println("==============================================");

  // 1) Wi-Fi (único lugar): NO volver a tocar WiFi.mode/softAP en main
  WiFiManager::ApConfig apCfg;
  apCfg.ssid    = AP_SSID;        // ej: "DEPOSITO_BROKER"
  apCfg.password= AP_PASSWORD;    // ej: "deposito123"
  apCfg.ip      = IPAddress(AP_IP_ADDR);       // ej: 192,168,4,1
  apCfg.gateway = IPAddress(AP_GATEWAY_ADDR);  // ej: 192,168,4,1
  apCfg.subnet  = IPAddress(AP_SUBNET_ADDR);   // ej: 255,255,255,0

  #if ENABLE_STATION_MODE
    WiFiManager::StaConfig staCfg;
    staCfg.ssid     = WIFI_SSID;
    staCfg.password = WIFI_PASSWORD;
    wifiManager = new WiFiManager(WiFiManager::AP_STA, apCfg, staCfg);
  #else
    wifiManager = new WiFiManager(WiFiManager::AP_ONLY, apCfg, WiFiManager::StaConfig());
  #endif
  wifiManager->initialize(15000); // timeout STA 15s


  // 2) MQTT broker (inyectando wifiManager)
  mqttBrokerManager = new MQTTBrokerManager(wifiManager, nullptr); // DeviceManager aún no
  // 3) EEPROM
  eepromManager = new EEPROMManager();
  eepromManager->begin();
  // 4) Devices
  deviceManager = new DeviceManager(wifiManager, mqttBrokerManager, eepromManager);
  deviceManager->initialize();
  // Conectar los dos mundos
  mqttBrokerManager->setDeviceManager(deviceManager);
  mqttBrokerManager->initialize();

  // 5) Web server
  webServerManager = new WebServerManager(wifiManager, deviceManager);
  // Inyectar mqttBrokerManager para que WebServer pueda enviar comandos a módulos si hace falta
  webServerManager->setMQTTBrokerManager(mqttBrokerManager);
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
