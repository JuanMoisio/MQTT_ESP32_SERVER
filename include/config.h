#ifndef CONFIG_H
#define CONFIG_H

// =================================
// CONFIGURACIÓN WIFI ACCESS POINT
// =================================
const char* AP_SSID = "DEPOSITO_BROKER";
const char* AP_PASSWORD = "deposito123";  // Mínimo 8 caracteres

// IPs se definen en main.cpp (necesitan #include <WiFi.h>)
#define AP_IP_ADDR      192, 168, 4, 1
#define AP_GATEWAY_ADDR 192, 168, 4, 1  
#define AP_SUBNET_ADDR  255, 255, 255, 0

// Configuración opcional para WiFi cliente (si se quiere conectar también a internet)
const char* WIFI_SSID = "TU_WIFI_SSID";  // Opcional
const char* WIFI_PASSWORD = "TU_WIFI_PASSWORD";  // Opcional
const bool ENABLE_STATION_MODE = false;  // true para modo híbrido AP+STA

// =================================
// CONFIGURACIÓN DEL BROKER MQTT
// =================================
const int MQTT_PORT = 1883;
const int MAX_CLIENTS = 10;

// =================================
// CONFIGURACIÓN DEL SISTEMA
// =================================
const unsigned long HEARTBEAT_INTERVAL = 30000;  // 30 segundos
const unsigned long HEARTBEAT_TIMEOUT = 60000;   // 60 segundos (2x heartbeat)

// =================================
// TIPOS DE MÓDULOS PERMITIDOS
// =================================
const char* ALLOWED_MODULE_TYPES[] = {
  "control_acceso",
  "motor",
  "sensor_temperatura", 
  "sensor_humedad",
  "actuador",
  "display",
  "rfid",
  "camara",
  "fingerprint_scanner"
};

const int NUM_ALLOWED_TYPES = sizeof(ALLOWED_MODULE_TYPES) / sizeof(ALLOWED_MODULE_TYPES[0]);

// =================================
// CONFIGURACIÓN DE DEPURACIÓN
// =================================
#define DEBUG_ENABLED true
#define SERIAL_BAUDRATE 115200

// =================================
// ESTRUCTURA DE TOPICS MQTT
// =================================
// deposito/
// ├── control_acceso/
// │   ├── cmd/unlock
// │   ├── status/door
// │   └── config/
// ├── motor1/
// │   ├── cmd/move
// │   ├── status/position
// │   └── config/
// ├── sensores/
// │   ├── temperatura/data
// │   ├── humedad/data
// │   └── config/
// └── system/
//     ├── discovery/
//     ├── health/
//     └── config/

#endif // CONFIG_H