#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <customMeas.h>
#include <customSD.h>
#include <customWifi.h>
#include <customTCP.h>
#include <customNVS.h>

extern QueueHandle_t measurementQueue;

extern TaskHandle_t taskReadHandle;
extern TaskHandle_t taskTCPHandle;
extern TaskHandle_t taskCleanupHandle;


// TIMER
void initReadTimer();
void IRAM_ATTR onReadTimer();


// TASK READ
void initTaskRead();
void taskRead(void* parameter);

// TASK WRITER
void initTaskWriter(StorageState& storageState);
void taskWriter(void* parameter);

// TASK WIFI
void initTaskWifi(WifiSetup& configWifi);
void taskWifi(void* parameter);

// TASK TCP
void initTaskTCP(WifiSetup& configWifi, StorageState& storageState);
void taskTCP(void* parameter);

// TASK CLEANUP
void initTaskCleanup(StorageState& storageState);
void taskCleanup(void* parameter);