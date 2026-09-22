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

void leer_datos_TCP(WifiSetup& configWifi, char* commandBuffer, size_t& commandIndex, 
                        bool& discardCommand, SemaphoreHandle_t sdMutex){
    
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
            procesarComandoTCP(configWifi, commandBuffer, sdMutex);
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

void procesarComandoTCP(WifiSetup& configWifi, const char* comando, 
                        SemaphoreHandle_t sdMutex){
    
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

        xSemaphoreTake(sdMutex, portMAX_DELAY); // ocupo la SD
        bool flag = escribir_sync_a_SD(syncState);
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

        if (!enviar_historico(configWifi, timestamp, sdMutex)){
            // Si TCP sigue vivo podemos avisar.
            if (configWifi.client.connected()){
                configWifi.client.println("ERR,HISTORY_TRANSFER");
            }
        }

        return;
    }

    configWifi.client.println("ERR,UNKNOWN_COMMAND");
}

bool enviar_historico(WifiSetup& configWifi, uint32_t timestamp, SemaphoreHandle_t sdMutex){
    
    size_t snapshotSize = 0;

    xSemaphoreTake(sdMutex, portMAX_DELAY); // bloquea la SD
    bool snapshotOk = obtener_size_datos(snapshotSize);
    xSemaphoreGive(sdMutex); // libero la SD

    if (!snapshotOk){
        configWifi.client.println("ERR,SD");
        return false;
    }

    // Empiezo a transmitir
    configWifi.client.print("DATA_BEGIN,");
    configWifi.client.println(snapshotSize);

    // Archivo vacío
    if (snapshotSize == 0){
        configWifi.client.println("DATA_END");
        return true;
    }

    // gestion de buffers
    uint8_t sdBuffer[SD_READ_BUFFER_SIZE];
    char lineBuffer[SD_LINE_BUFFER_SIZE];
    size_t lineIndex = 0;
    bool discardLine = false;
    size_t offset = 0;

    while (offset < snapshotSize){ // Leer hasta Snapshot
        size_t remaining = snapshotSize - offset;
        size_t bytesToRead = remaining;

        if (bytesToRead > SD_READ_BUFFER_SIZE){
            bytesToRead = SD_READ_BUFFER_SIZE;
        }

        xSemaphoreTake(sdMutex, portMAX_DELAY); // bloqueo SD
        size_t bytesRead = leer_bloque_datos(offset, sdBuffer, bytesToRead);
        xSemaphoreGive(sdMutex); // desbloqueo la SD

        if (bytesRead == 0){
            Serial.println("Error leyendo histórico SD");
            return false;
        }

        offset += bytesRead;

        for (size_t i = 0; i < bytesRead; i++){ // Proceso el bloque
            char c = static_cast<char>(sdBuffer[i]);

            // Ignorar \r
            if (c == '\r'){continue;}

            if (c == '\n'){ // Fin de la linea
                if (!discardLine && lineIndex > 0){
                    lineBuffer[lineIndex] = '\0';

                    // Extrae el timestamp
                    char* endPointer = nullptr;
                    unsigned long lineTimestamp = strtoul(lineBuffer, &endPointer,10);

                    // El timestamp debe acabar justo
                    // donde aparece la coma.
                    if (endPointer != lineBuffer && *endPointer == ','){

                        if (lineTimestamp >= timestamp){

                            if (!configWifi.client.connected()){return false;}

                            configWifi.client.write(reinterpret_cast < const uint8_t* > (lineBuffer), lineIndex);

                            configWifi.client.write('\n');
                        }
                    }
                }


                lineIndex = 0;
                discardLine = false;
                continue;
            }

            if (!discardLine){ // construir la linea
                if (lineIndex < SD_LINE_BUFFER_SIZE - 1){
                    lineBuffer[lineIndex] = c;
                    lineIndex++;
                }else{ // por seaca para evitar overflow
                    discardLine = true;
                    lineIndex = 0;
                }
            }
        }

        // Dar algo de CPU al resto de tareas.
        vTaskDelay(pdMS_TO_TICKS(1));
    }


    if (lineIndex > 0 && !discardLine){
        
        lineBuffer[lineIndex] = '\0';

        char* endPointer = nullptr;

        unsigned long lineTimestamp = strtoul(lineBuffer, &endPointer, 10);

        if (endPointer != lineBuffer && *endPointer == ',' && lineTimestamp >= timestamp){
            if (!configWifi.client.connected()){
                return false;
            }


            configWifi.client.write(reinterpret_cast < const uint8_t* >(lineBuffer), lineIndex);
            configWifi.client.write('\n');
        }
    }

    if (!configWifi.client.connected()){return false;}
    configWifi.client.println("DATA_END");
    return true;
}