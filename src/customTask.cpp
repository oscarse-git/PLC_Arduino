#include <customTask.h>
#include <customSD.h>
#include <customMeas.h>
#include <customWifi.h>

hw_timer_t* readTimer = nullptr;

QueueHandle_t measurementQueue = nullptr;

SemaphoreHandle_t sdMutex = nullptr;

TaskHandle_t taskReadHandle = nullptr;
TaskHandle_t taskWriterHandle = nullptr;
TaskHandle_t taskWifiHandle = nullptr;
TaskHandle_t taskTCPHandle = nullptr;

struct TaskTCPParams{ // estructura tiene punteros a las structuras OG
    WifiSetup* configWifi;
    StorageState* storageState;
};

TaskTCPParams taskTCPParams;

// TIMER

void initReadTimer(){
    // Timer 0
    // 80 MHz / 80 = 1 MHz
    // 1 tick = 1 us
    readTimer = timerBegin(0, 80, true);

    timerAttachInterrupt(readTimer, &onReadTimer, true);

    // 10 000 us = 10 ms
    timerAlarmWrite(readTimer, 10000, true);

    timerAlarmEnable(readTimer);
}

void IRAM_ATTR onReadTimer(){
    BaseType_t taskWoken = pdFALSE;

    vTaskNotifyGiveFromISR(taskReadHandle, &taskWoken);

    if (taskWoken == pdTRUE){portYIELD_FROM_ISR();}
}



// TASK READ

void initTaskRead(){
    measurementQueue = xQueueCreate(500, sizeof(Measurement));

    if (measurementQueue == nullptr){
        Serial.println("ERROR creando measurementQueue");
        return;
    }

    xTaskCreatePinnedToCore(
        taskRead,
        "taskRead",
        4096,
        nullptr,
        3,
        &taskReadHandle,
        1
    );
}

void taskRead(void* parameter){
    Measurement meas;
    meas.timestamp = 0;
    meas.pinState = 0;

    while (true){ 
        ulTaskNotifyTake(pdFALSE, portMAX_DELAY); // Duerme hasta que el timer dispara

        if (lecturaEntrada(meas)){ // mira si algo cambia y guarda solo en ese caso
            xQueueSend(measurementQueue, &meas, 0);
        }
    }
}



// TASK WRITE

void initTaskWriter(StorageState& storageState){
    // Crear mutex compartido de la SD
    sdMutex = xSemaphoreCreateMutex();

    if (sdMutex == nullptr){
        Serial.println("ERROR creando sdMutex");
        return;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        taskWriter,          
        "taskWriter",        
        4096,              
        &storageState,
        2,
        &taskWriterHandle,
        1
    );

    if (result != pdPASS){
        Serial.println("ERROR creando taskWriter");
        return;
    }
}

void taskWriter(void* parameter){
    
    StorageState* storageState = static_cast<StorageState*>(parameter);
    Measurement meas;

    while (true){
        // Esperar una medida de taskRead
        xQueueReceive(measurementQueue, &meas, portMAX_DELAY);

        size_t fileSize = 0;

        // Esperar acceso exclusivo a la SD
        xSemaphoreTake(sdMutex, portMAX_DELAY);

        bool flag = escribir_dato_a_SD(meas, storageState->current_writing_file, fileSize);

        // comprobar si hay que rotar (overflow de los 16 bits)
        if (flag && fileSize >= MAX_CSV_SIZE){

            uint16_t nextFile = static_cast<uint16_t>(storageState->current_writing_file + 1);
            
            if (crear_archivo_csv(nextFile)){
            
                if (guardar_current_writing_file(nextFile)){
                    storageState->current_writing_file = nextFile;
                }else{
                    Serial.println("ERROR persistiendo current_writing_file");
                }
            
            }else{
                Serial.println("ERROR creando nuevo CSV durante rotacion");
            }
        }

        xSemaphoreGive(sdMutex);
        if(!flag){Serial.println("ERROR guardando Measurement en SD");}

    }
}



// TASK WIFI

void initTaskWifi(WifiSetup& configWifi){

    BaseType_t result = xTaskCreatePinnedToCore(
        taskWifi,
        "taskWifi",
        4096,
        &configWifi,
        1,
        &taskWifiHandle,
        0
    );

    if (result != pdPASS){
        Serial.println("ERROR creando taskWifi");
    }
}

void taskWifi(void* parameter){
    WifiSetup* configWifi = static_cast<WifiSetup*>(parameter);
    WiFi.mode(WIFI_STA);

    while (true){
        if (WiFi.status() != WL_CONNECTED){
            Serial.println("WiFi desconectado. Reconectando...");

            WiFi.disconnect();

            WiFi.begin(
                configWifi->ssid,
                configWifi->password
            );

            while (WiFi.status() != WL_CONNECTED)
            {
                vTaskDelay(
                    pdMS_TO_TICKS(500)
                );
            }

            Serial.println("WiFi conectado");

            Serial.print("IP ESP32: ");
            Serial.println(WiFi.localIP());
        }


        // No hace falta comprobar WiFi continuamente
        vTaskDelay(
            pdMS_TO_TICKS(2000)
        );
    }
}


// TASK TCP

void initTaskTCP(WifiSetup& configWifi, StorageState& storageState){

    taskTCPParams.configWifi = &configWifi;
    taskTCPParams.storageState = &storageState;

    BaseType_t result =
        xTaskCreatePinnedToCore(
            taskTCP,
            "taskTCP",
            6144,
            &taskTCPParams,
            1,
            &taskTCPHandle,
            0
        );


    if (result != pdPASS){
        Serial.println("ERROR creando taskTCP");
    }
}


void taskTCP(void* parameter){

    TaskTCPParams* params = static_cast<TaskTCPParams*>(parameter);
    WifiSetup* configWifi = params->configWifi;
    StorageState* storageState = params->storageState;

    char commandBuffer[TCP_COMMAND_BUFFER_SIZE];
    size_t commandIndex = 0;
    bool discardCommand = false;


    while (true){
        if (WiFi.status() != WL_CONNECTED){
            configWifi->client.stop();

            commandIndex = 0;
            discardCommand = false;

            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (!configWifi->client.connected()){

            commandIndex = 0;
            discardCommand = false;

            if (!asegurar_conexion_TCP(*configWifi)){
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
        }

        leer_datos_TCP(*configWifi, *storageState, commandBuffer, commandIndex, discardCommand, sdMutex);

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}