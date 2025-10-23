#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "../../include/config.h"

class WiFiManager {
public:
    WiFiManager();
    
    // Métodos públicos
    void initialize();
    String getAPIP();
    String getStationIP();
    bool isStationConnected();
    int getConnectedClients();
    void printStatus();
    
private:
    // Variables privadas
    const char* ap_ssid;
    const char* ap_password;
    const char* sta_ssid;
    const char* sta_password;
    
    IPAddress ap_ip;
    IPAddress ap_gateway;
    IPAddress ap_subnet;
    
    // Métodos privados
    void setupAccessPoint();
    void setupStation();
};

#endif