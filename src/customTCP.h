#pragma once

#include <Arduino.h>
#include <customWifi.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>


struct SyncState{
    bool pending;
    uint32_t startTime;
    uint32_t endTime;
    uint32_t plcTime;
    uint64_t serverTime;
};

constexpr size_t TCP_COMMAND_BUFFER_SIZE = 64;
constexpr size_t SD_READ_BUFFER_SIZE = 512;
constexpr size_t SD_LINE_BUFFER_SIZE = 64;

bool asegurar_conexion_TCP(WifiSetup& configWifi);

void leer_datos_TCP(WifiSetup& configWifi, char* commandBuffer, size_t& commandIndex, 
                        bool& discardCommand, SemaphoreHandle_t sdMutex);
void procesarComandoTCP(WifiSetup& configWifi, const char* comando, SemaphoreHandle_t sdMutex);
bool enviar_historico(WifiSetup& configWifi, uint32_t timestamp, SemaphoreHandle_t sdMutex);

