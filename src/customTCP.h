#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <customWifi.h>
#include <customNVS.h>

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
constexpr size_t RECORD_PREFIX_SIZE = 5; // todos los tipos de dato son 4 caracteres + ,



bool asegurar_conexion_TCP(WifiSetup& configWifi);

void leer_datos_TCP(WifiSetup& configWifi, StorageState& storageState, char* commandBuffer, 
                    size_t& commandIndex, bool& discardCommand, SemaphoreHandle_t sdMutex);

void procesarComandoTCP(WifiSetup& configWifi, StorageState& storageState, 
                        const char* comando, SemaphoreHandle_t sdMutex);

bool enviar_datos(WifiSetup& configWifi, uint32_t timestamp, 
                    SemaphoreHandle_t sdMutex, bool buscarTimeStamp = false);

