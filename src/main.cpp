#include <Arduino.h>
#include <M5StamPLC.h>

#include <customWifi.h>
#include <customTask.h>
#include <customNVS.h>



// Config global
WifiSetup configWifi;
StorageState storageState{};

void setup(){
    
    Serial.begin(115200);

    delay(500);

    Serial.println();
    Serial.println("==============================");
    Serial.println("        INICIANDO PLC");
    Serial.println("==============================");


    // Configurar la PLC
    auto config = M5StamPLC.config();
    config.enableSdCard = true;
    M5StamPLC.config(config);
    M5StamPLC.begin();
    Serial.println("M5StamPLC inicializado");

    // Inicializar NVS
    if (!init_NVS()){
        Serial.println("ERROR inicializando NVS. PLC detenida");
        return;
    }

    // Cargar estado persistente a RAM
    if (!cargar_storage_state(storageState)){
        Serial.println("ERROR cargando StorageState. PLC detenida");
        return;
    }


    // Configurar Wifi
    configWifi.ssid = "test_server_plc";
    configWifi.password = "PedritoServerTest";
    configWifi.servidor = IPAddress(192, 168, 50, 1);
    configWifi.puerto = 5000;
    Serial.println("Configuracion WiFi/TCP cargada");


    // Iniciamos task read
    initTaskRead();
    Serial.println("taskRead inicializado");

    // Iniciamos task write
    initTaskWriter(storageState);
    Serial.println("taskWriter inicializado");

    // Iniciamos task wifi
    initTaskWifi(configWifi);
    Serial.println("taskWifi inicializado");

    // Iniciamos task TCP
    initTaskTCP(configWifi, storageState);
    Serial.println("taskTCP inicializado");

    // Iniciamos task cleanup
    initTaskCleanup(storageState);
    Serial.println("taskCleanup inicializado");

    // Iniciamos el timer y empieza a funcionar esto
    initReadTimer();
    
    Serial.println("Timer de lectura iniciado");
    Serial.println("==============================");
    Serial.println("      PLC INICIALIZADO");
    Serial.println("==============================");
}

void loop(){
    // Toda la logica funciona mediante FreeRTOS tasks.
    // No necesitamos ejecutar nada aqui.
    // El bucle solo mantiene loop() dormido
    vTaskDelay(pdMS_TO_TICKS(1000));
}