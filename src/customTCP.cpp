#include <Arduino.h>
#include <customTCP.h>
#include <customWifi.h>
#include <customSD.h>

SyncState syncState{
    false,
    0,
    0,
    0,
    0
};


bool asegurar_conexion_TCP(WifiSetup& configWifi){
    if (configWifi.client.connected()) return true;


    Serial.println("TCP desconectado. Conectando...");

    // Limpiar socket anterior
    configWifi.client.stop();

    if (conectar_servidor_TCP(configWifi)){
        Serial.println("TCP conectado al servidor");
        return true;
    }

    Serial.println("No se pudo conectar TCP");

    return false;
}

void leer_datos_TCP(WifiSetup& configWifi, StorageState& storageState, char* commandBuffer, 
                    size_t& commandIndex, bool& discardCommand, SemaphoreHandle_t sdMutex){
    
    while (configWifi.client.available() > 0){
        
        int received = configWifi.client.read();
        if (received < 0){break;}
        char c = static_cast<char>(received);

        if (c == '\r'){continue;} // ignorar \r


        if (c == '\n'){ // final del comando

            syncState.endTime = millis();

            if (discardCommand){
                discardCommand = false;
                commandIndex = 0;
                
                continue;
            }
            
            if (commandIndex == 0){continue;} // Ignorar líneas vacías

            commandBuffer[commandIndex] = '\0'; // Terminar string C
            procesarComandoTCP(configWifi, storageState, commandBuffer, sdMutex);
            commandIndex = 0; // Preparar siguiente comando

            continue;
        }

        if (!discardCommand){
            if (commandIndex < TCP_COMMAND_BUFFER_SIZE - 1){
                commandBuffer[commandIndex] = c;
                commandIndex++;

            }else{
                // El comando es demasiado largo.
                // Descartamos hasta encontrar \n.

                discardCommand = true;
                commandIndex = 0;

                configWifi.client.println("ERR,COMMAND_TOO_LONG");
            }
        }
    }
}

void procesarComandoTCP(WifiSetup& configWifi, StorageState& storageState, 
                        const char* comando, SemaphoreHandle_t sdMutex){
    
    Serial.print("Comando TCP recibido: ");

    Serial.println(comando);

    if (strcmp(comando,"PING") == 0){ // Debug
        configWifi.client.println("PONG");
        return;
    }

    if (strcmp(comando, "SYNC") == 0){
        syncState.pending = true; // defino que estamos en el proceso de sincronizacion
        syncState.startTime = millis();
        configWifi.client.println("TIME_REQ");
        return;
    }

    if (strncmp(comando, "TIME,", 5) == 0){
        if (!syncState.pending){ // no aceptar time si nosotros el proceso no ha sido comenzado
            configWifi.client.println("ERR,UNEXPECTED_TIME");
            return;
        }

        const char* timestampText = comando + 5;

        if (*timestampText == '\0' || *timestampText == '-'){ // Proteccion por si el tiempo dado es malo
            syncState.pending = false;
            configWifi.client.println("ERR,INVALID_UNIX_TIME");
            return;
        }

        char* endPointer = nullptr;

        unsigned long long value = strtoull(timestampText, &endPointer, 10);

        // Comprobar que todo el string era numerico
        if (endPointer == timestampText || *endPointer != '\0'){
            syncState.pending = false;
            configWifi.client.println("ERR,INVALID_UNIX_TIME");
            return;
        }

        syncState.serverTime = static_cast<uint64_t>(value); // guardo el tiempo del server
        syncState.pending = false; // cierro el pending porque ya tengo todo lo que necesito
        syncState.plcTime = syncState.startTime + (syncState.endTime - syncState.startTime) / 2;

        size_t fileSize = 0;

        xSemaphoreTake(sdMutex, portMAX_DELAY); // ocupo la SD

        bool flag = escribir_sync_a_SD(syncState, storageState.current_writing_file, fileSize);
        
        if (flag && fileSize >= MAX_CSV_SIZE){

            uint16_t nextFile = static_cast<uint16_t>(storageState.current_writing_file + 1);
            
            if (crear_archivo_csv(nextFile)){
            
                if (guardar_current_writing_file(nextFile)){
                    storageState.current_writing_file = nextFile;
                }else{
                    Serial.println("ERROR persistiendo current_writing_file");
                }
            
            }else{
                Serial.println("ERROR creando nuevo CSV durante rotacion");
            }
        }
    

        xSemaphoreGive(sdMutex); // libero la SD

        if(!flag){
            configWifi.client.println("ERR,SYNC_SD");
            return;
        }

        // Solo confirmamos después de guardar correctamente en SD
        configWifi.client.print("SYNC_OK,");
        configWifi.client.println(syncState.plcTime);
        Serial.println("SYNC realizada | PLC");
        return;
    }

    if (strcmp(comando, "GET_ALL") == 0){

        if (!enviar_datos(configWifi, 0, sdMutex, true)){
            if (configWifi.client.connected()){
                configWifi.client.println("ERR,DATA_TRANSFER");
            }
        }

        return;
    }

    if (strncmp(comando, "GET_FROM,", 9) == 0){
        const char* timestampText = comando + 9;

        // No hay timestamp
        if (*timestampText == '\0' ||*timestampText == '-'){
            configWifi.client.println("ERR,INVALID_TIMESTAMP");
            return;
        }

        char* endPointer = nullptr;

        unsigned long value = strtoul(timestampText, &endPointer, 10);

        // Comprobar que se convirtio todo el texto
        if (endPointer == timestampText || *endPointer != '\0'){
            configWifi.client.println("ERR,INVALID_TIMESTAMP");
            return;
        }

        uint32_t timestamp = static_cast<uint32_t>(value);

        if (!enviar_datos(configWifi, timestamp, sdMutex)){
            // Si TCP sigue vivo podemos avisar.
            if (configWifi.client.connected()){
                configWifi.client.println("ERR,DATA_TRANSFER");
            }
        }

        return;
    }

    configWifi.client.println("ERR,UNKNOWN_COMMAND");
}

bool enviar_datos(WifiSetup& configWifi, uint32_t timestamp, 
        SemaphoreHandle_t sdMutex, bool buscarTimeStamp){
    
              size_t snapshotSize = 0;

    // Obtener snapshot del tamaño actual del archivo
    xSemaphoreTake(sdMutex, portMAX_DELAY);
    bool snapshotOk = obtener_size_datos(snapshotSize);
    xSemaphoreGive(sdMutex);

    if (!snapshotOk){
        configWifi.client.println("ERR,SD");
        return false;
    }

    // Avisar del comienzo de la transmision
    configWifi.client.print("DATA_BEGIN,");
    configWifi.client.println(snapshotSize);

    // Archivo vacío
    if (snapshotSize == 0){
        configWifi.client.println("DATA_END");
        return true;
    }

    uint8_t sdBuffer[SD_READ_BUFFER_SIZE];
    char lineBuffer[SD_LINE_BUFFER_SIZE];
    size_t lineIndex = 0;
    bool discardLine = false;
    size_t offset = 0;

    while (offset < snapshotSize){

        size_t remaining = snapshotSize - offset;
        size_t bytesToRead = remaining;

        if (bytesToRead > SD_READ_BUFFER_SIZE){
            bytesToRead = SD_READ_BUFFER_SIZE;
        }

        // Leer bloque de SD
        xSemaphoreTake(sdMutex, portMAX_DELAY);
        size_t bytesRead = leer_bloque_datos(offset, sdBuffer, bytesToRead);
        xSemaphoreGive(sdMutex);

        if (bytesRead == 0){
            Serial.println("Error leyendo datos SD");
            return false;
        }

        offset += bytesRead;

        // Procesar el bloque
        for (size_t i = 0; i < bytesRead; i++){

            char c = static_cast<char>(sdBuffer[i]);

            // Ignorar CR
            if (c == '\r'){continue;}
            
            // Final de la linea
            if (c == '\n'){

                if (!discardLine && lineIndex > RECORD_PREFIX_SIZE){

                    lineBuffer[lineIndex] = '\0';

                    // DATA,xxxx,...
                    // SYNC,xxxx,...
                    const char* timestampText = lineBuffer + RECORD_PREFIX_SIZE;
                    char* endPointer = nullptr;

                    unsigned long lineTimestamp = strtoul(timestampText, &endPointer, 10);

                    // Formato valido:
                    // TTTT,<timestamp>,...
                    if (endPointer != timestampText && *endPointer == ','){
                        if (lineTimestamp >= timestamp){

                            if (!configWifi.client.connected()){return false;}

                            configWifi.client.write(reinterpret_cast<const uint8_t*>(lineBuffer), lineIndex);
                            configWifi.client.write('\n');
                        }
                    }
                }

                // Preparar siguiente línea
                lineIndex = 0;
                discardLine = false;

                continue;
            }

            // Construyo la linea
            if (!discardLine){
                if (lineIndex <SD_LINE_BUFFER_SIZE - 1){
                    lineBuffer[lineIndex] = c;
                    lineIndex++;
                }else{
                    // Línea demasiado larga.
                    // Descartar hasta encontrar '\n'
                    discardLine = true;
                    lineIndex = 0;
                }
            }
        }

        // Dar CPU al resto de tareas
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // ultima linea
    if (lineIndex > RECORD_PREFIX_SIZE && !discardLine){
        
        lineBuffer[lineIndex] = '\0';
        const char* timestampText = lineBuffer +RECORD_PREFIX_SIZE;
        char* endPointer = nullptr;

        unsigned long lineTimestamp = strtoul(timestampText, &endPointer, 10);

        if (endPointer != timestampText && *endPointer == ',' && lineTimestamp >= timestamp){
            
            if (!configWifi.client.connected()){return false;}
            configWifi.client.write(reinterpret_cast<const uint8_t*>(lineBuffer), lineIndex);
            configWifi.client.write('\n');
        }
    }

    if (!configWifi.client.connected()){return false;}
    configWifi.client.println("DATA_END");
    return true;
}

