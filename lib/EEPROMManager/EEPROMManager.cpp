#include "EEPROMManager.h"
#include <EEPROM.h>
#include <Arduino.h>
#include <vector>
#include <map>

EEPROMManager::EEPROMManager() {} // Constructor por defecto

void EEPROMManager::begin() {
    EEPROM.begin(EEPROM_SIZE);
}

// Guardar un DeviceInfo asegurando null-termination
void EEPROMManager::saveDevice(const DeviceInfo& device, int index) {
    int addr = index * sizeof(DeviceInfo);
    DeviceInfo tmp;
    memset(&tmp, 0, sizeof(tmp));
    // copiar con límite y asegurar terminación
    strncpy(tmp.mac, device.mac, sizeof(tmp.mac) - 1);
    strncpy(tmp.type, device.type, sizeof(tmp.type) - 1);
    strncpy(tmp.apiKey, device.apiKey, sizeof(tmp.apiKey) - 1);
    strncpy(tmp.desc, device.desc, sizeof(tmp.desc) - 1);
    tmp.mac[sizeof(tmp.mac) - 1] = '\0';
    tmp.type[sizeof(tmp.type) - 1] = '\0';
    tmp.apiKey[sizeof(tmp.apiKey) - 1] = '\0';
    tmp.desc[sizeof(tmp.desc) - 1] = '\0';

    EEPROM.put(addr, tmp);
    EEPROM.commit();
    Serial.printf("[EEPROM] Dispositivo guardado en slot %d (addr %d)\n", index, addr);
}

DeviceInfo EEPROMManager::loadDevice(int index) {
    int addr = index * sizeof(DeviceInfo);
    DeviceInfo device;
    memset(&device, 0, sizeof(device));
    EEPROM.get(addr, device);
    // asegurar terminación por si acaso
    device.mac[sizeof(device.mac)-1] = '\0';
    device.type[sizeof(device.type)-1] = '\0';
    device.apiKey[sizeof(device.apiKey)-1] = '\0';
    device.desc[sizeof(device.desc)-1] = '\0';
    Serial.printf("[EEPROM] Dispositivo leído de pos %d: MAC=%s\n", addr, device.mac);
    return device;
}

std::vector<DeviceInfo> EEPROMManager::loadAllDevices(int maxDevices) {
    std::vector<DeviceInfo> devices;
    for (int i = 0; i < maxDevices; ++i) {
        DeviceInfo device = loadDevice(i);
        if (device.mac[0] != '\0') { // Si hay MAC, está guardado
            devices.push_back(device);
        }
    }
    Serial.printf("[EEPROM] Total recuperados: %d\n", devices.size());
    return devices;
}

// Helper: validar formato MAC "AA:BB:CC:DD:EE:FF"
bool EEPROMManager::isMacValid(const String& mac) {
    if (mac.length() != 17) return false;
    for (int i = 0; i < 17; ++i) {
        if (i == 2 || i == 5 || i == 8 || i == 11 || i == 14) {
            if (mac[i] != ':') return false;
        } else {
            char c = mac[i];
            bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
            if (!isHex) return false;
        }
    }
    return true;
}

// Implementación de loadDevices (filtra entradas inválidas)
std::map<String, DeviceRecord> EEPROMManager::loadDevices() {
    std::map<String, DeviceRecord> devices;
    for (int i = 0; i < MAX_EEPROM_DEVICES; ++i) {
        DeviceInfo info = loadDevice(i);
        String macStr = String(info.mac);
        // Filtrar basura: MAC debe ser válida
        if (!isMacValid(macStr)) continue;
        DeviceRecord rec;
        rec.macAddress = macStr;
        rec.deviceType = String(info.type);
        rec.apiKey = String(info.apiKey);
        rec.description = String(info.desc);
        devices[rec.macAddress] = rec;
    }
    return devices;
}

// Implementación de saveDevices (escribe hasta MAX_EEPROM_DEVICES) - solo MACs válidas
void EEPROMManager::saveDevices(const std::map<String, DeviceRecord>& devices) {
    int index = 0;
    for (const auto& pair : devices) {
        if (index >= MAX_EEPROM_DEVICES) break;
        const DeviceRecord& rec = pair.second;
        if (!isMacValid(rec.macAddress)) {
            Serial.printf("[EEPROM] saveDevices: saltando MAC inválida: '%s'\n", rec.macAddress.c_str());
            continue;
        }
        DeviceInfo info;
        memset(&info, 0, sizeof(info));
        strncpy(info.mac, rec.macAddress.c_str(), sizeof(info.mac) - 1);
        strncpy(info.type, rec.deviceType.c_str(), sizeof(info.type) - 1);
        strncpy(info.apiKey, rec.apiKey.c_str(), sizeof(info.apiKey) - 1);
        strncpy(info.desc, rec.description.c_str(), sizeof(info.desc) - 1);
        saveDevice(info, index++);
    }
    // No limpiamos aquí; compactDevices() puede hacerlo cuando se llame.
}

// Remover un dispositivo por MAC y reescribir EEPROM
void EEPROMManager::removeDevice(const String& macAddress) {
    auto devices = loadDevices();
    if (devices.find(macAddress) != devices.end()) {
        devices.erase(macAddress);
        saveDevices(devices);
        compactDevices();
        Serial.println("[EEPROM] Device removed and EEPROM compacted");
    } else {
        Serial.println("[EEPROM] removeDevice: MAC no encontrada");
    }
}

// compactDevices: reescribe dispositivos válidos desde slot 0 y limpia el resto
void EEPROMManager::compactDevices() {
    Serial.println("[EEPROM] Iniciando recompactación...");
    auto devices = loadDevices(); // ahora solo devuelve MAC válidas
    // Reescribir en slots iniciales
    int idx = 0;
    for (const auto& pair : devices) {
        if (idx >= MAX_EEPROM_DEVICES) break;
        DeviceRecord rec = pair.second;
        DeviceInfo info;
        memset(&info, 0, sizeof(info));
        strncpy(info.mac, rec.macAddress.c_str(), sizeof(info.mac) - 1);
        strncpy(info.type, rec.deviceType.c_str(), sizeof(info.type) - 1);
        strncpy(info.apiKey, rec.apiKey.c_str(), sizeof(info.apiKey) - 1);
        strncpy(info.desc, rec.description.c_str(), sizeof(info.desc) - 1);
        saveDevice(info, idx++);
    }
    // Limpiar slots restantes
    DeviceInfo empty;
    memset(&empty, 0, sizeof(empty));
    for (; idx < MAX_EEPROM_DEVICES; ++idx) {
        saveDevice(empty, idx);
    }
    Serial.printf("[EEPROM] Recompactación completada: %d dispositivos escritos, %d slots limpiados\n",
                  (int)devices.size(), MAX_EEPROM_DEVICES - (int)devices.size());
}

// Implementaciones de test byte
void EEPROMManager::writeTestByte(uint8_t value) {
    EEPROM.write(1, value);
    EEPROM.commit();
    Serial.print("[EEPROM TEST] Byte escrito en pos 1: ");
    Serial.println(value);
}

uint8_t EEPROMManager::readTestByte() {
    uint8_t value = EEPROM.read(1);
    Serial.print("[EEPROM TEST] Byte leído de pos 1: ");
    Serial.println(value);
    return value;
}
