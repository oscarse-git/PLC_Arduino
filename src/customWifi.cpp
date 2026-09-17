#pragma once

#include <customWifi.h>
#include <WiFi.h>


void start_wifi_connection(WifiSetup& configWifi){
    WiFi.mode(WIFI_STA);
    WiFi.begin(configWifi.ssid, configWifi.password);

    Serial.print("Conectando a WiFi");

    while (WiFi.status() != WL_CONNECTED){
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi conectado");

    Serial.print("IP ESP32: ");
    Serial.println(WiFi.localIP());
}


bool conectar_servidor_TCP(WifiSetup& configWifi){
    return configWifi.client.connect(
        configWifi.servidor, 
        configWifi.puerto);
}