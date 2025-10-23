#ifndef CONFIG_H
#define CONFIG_H

// =================================
// CONFIGURACIÓN WIFI ACCESS POINT
// =================================
extern const char* AP_SSID;
extern const char* AP_PASSWORD;

// IPs se definen en main.cpp (necesitan #include <WiFi.h>)
#define AP_IP_ADDR      192, 168, 4, 1
#define AP_GATEWAY_ADDR 192, 168, 4, 1  
#define AP_SUBNET_ADDR  255, 255, 255, 0

// Configuración opcional para WiFi cliente (si se quiere conectar también a internet)
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;
extern const bool ENABLE_STATION_MODE;

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
extern const char* ALLOWED_MODULE_TYPES[];
extern const int NUM_ALLOWED_TYPES;

// =================================
// CONFIGURACIÓN DE DEPURACIÓN
// =================================
#define SERIAL_BAUDRATE 115200

#endif // CONFIG_H