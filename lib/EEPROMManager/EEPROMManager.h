#pragma once
#include <Arduino.h>
#include <map>
#include <vector>

struct DeviceRecord {
    String macAddress;
    String deviceType;
    String apiKey;
    String description;
};

struct DeviceInfo {
    char mac[18];
    char type[16];
    char apiKey[33];
    char desc[64];
};

// Máximo de dispositivos que guardaremos en EEPROM
#ifndef MAX_EEPROM_DEVICES
#define MAX_EEPROM_DEVICES 10
#endif

class EEPROMManager {
public:
    static constexpr int EEPROM_SIZE = 4096;
    static constexpr int MAX_DEVICES = 32;
    static constexpr int RECORD_SIZE = 128;

    EEPROMManager();
    void begin();
    void saveDevices(const std::map<String, DeviceRecord>& devices);
    std::map<String, DeviceRecord> loadDevices();
    void removeDevice(const String& macAddress);
    void writeTestByte(uint8_t value);
    uint8_t readTestByte();
    void saveDevice(const DeviceInfo& device, int index);
    DeviceInfo loadDevice(int index);
    std::vector<DeviceInfo> loadAllDevices(int maxDevices);
    void compactDevices(); // <-- nuevo: recompacta EEPROM al arranque

    // Validación de MAC
    bool isMacValid(const String& mac);

private:
    void writeRecord(int addr, const DeviceRecord& rec);
    DeviceRecord readRecord(int addr);
    void writeString(int addr, const String& str, int maxLen);
    String readString(int addr, int maxLen);
};
