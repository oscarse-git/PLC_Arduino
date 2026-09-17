#pragma once
#include <WiFi.h>

struct WifiSetup {
    IPAddress servidor;
    uint16_t puerto;
    WiFiClient client;
    const char* ssid;
    const char* password;
};


void start_wifi_connection(WifiSetup& configWifi);
bool conectar_servidor_TCP(WifiSetup& configWifi);