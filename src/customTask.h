#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <customMeas.h>
#include <customSD.h>
#include <customWifi.h>
#include <customTCP.h>

extern QueueHandle_t measurementQueue;
extern TaskHandle_t taskReadHandle;
extern TaskHandle_t taskTCPHandle;

// TIMER
void initReadTimer();
void IRAM_ATTR onReadTimer();


// TASK READ
void initTaskRead();
void taskRead(void* parameter);


// TASK WRITER
void initTaskWriter();
void taskWriter(void* parameter);

// TASK WIFI
void initTaskWifi(WifiSetup& configWifi);
void taskWifi(void* parameter);

// TASK TCP
void initTaskTCP(WifiSetup& configWifi);
void taskTCP(void* parameter);