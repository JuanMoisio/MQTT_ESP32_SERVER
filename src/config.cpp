#include "config.h"

// Definiciones únicas (una sola unidad de traducción)
const char* AP_SSID = "DEPOSITO_BROKER";
const char* AP_PASSWORD = "deposito123";

// Tus credenciales proporcionadas
const char* WIFI_SSID = "MoisioNet";
const char* WIFI_PASSWORD = "47046414*Moisio";

// Intentar STA además de AP (true mantendrá AP y además intentará conectar como cliente)
const bool ENABLE_STATION_MODE = true;

// Tipos de módulos permitidos
const char* ALLOWED_MODULE_TYPES[] = {
    "rfid_reader",
    "fingerprint_scanner"
};
const int NUM_ALLOWED_TYPES = sizeof(ALLOWED_MODULE_TYPES) / sizeof(ALLOWED_MODULE_TYPES[0]);
