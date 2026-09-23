#include <Arduino.h>
#include <SD.h>

#include <customMeas.h>
#include <customTCP.h>
#include <customSD.h>
#include <customNVS.h>

bool escribir_dato_a_SD(Measurement& meas, uint16_t& fileId, size_t& fileSize){

    char filePath[12];

    snprintf(filePath, sizeof(filePath), "/%05u.csv", 
            static_cast<unsigned int>(fileId));

    File file = SD.open(filePath, FILE_APPEND);

    if (!file){
        Serial.print("Error abriendo ");
        Serial.println(filePath);
        return false;
    }


    file.print("DATA,");
    file.print(meas.timestamp);
    file.print(",");
    file.println(meas.pinState);

    fileSize = file.position(); // file.position() es al final del archivo, corresponde al tamaño

    file.close();
    return true;
}


bool escribir_sync_a_SD(SyncState& state, uint16_t& fileId, size_t& fileSize){
    char filePath[12];

    snprintf(filePath, sizeof(filePath), "/%05u.csv", static_cast<unsigned int>(fileId));

    File file = SD.open(filePath, FILE_APPEND);

    if (!file){
        Serial.print("Error abriendo ");
        Serial.println(filePath);
        return false;
    }

    file.print("SYNC,");
    file.print(state.plcTime);
    file.print(",");
    file.println(state.serverTime);

    fileSize = file.position(); // file.position() es al final del archivo, corresponde al tamaño

    file.close();
    return true;
}

bool crear_archivo_csv(uint16_t& fileId){

    char filePath[12];

    snprintf(filePath, sizeof(filePath), "/%05u.csv", static_cast<unsigned int>(fileId));

    // Si ya existe, no hacemos nada.
    if (SD.exists(filePath)){return true;}

    File file = SD.open(filePath, FILE_APPEND);

    if (!file){
        Serial.print("ERROR creando ");
        Serial.println(filePath);
        return false;
    }

    file.close();
    

    return true;
}

bool obtener_size_datos(size_t& size){
   
    if (!SD.exists("/datos.csv")){ // Si no existe historico
        size = 0;
        return true;
    }

    File file = SD.open("/datos.csv", FILE_READ);

    if (!file){
        Serial.println("Error abriendo datos.csv");
        return false;
    }

    size = file.size();
    file.close();
    return true;
}


size_t leer_bloque_datos(size_t offset, uint8_t* buffer, size_t maxBytes){
    File file = SD.open("/datos.csv", FILE_READ);

    if (!file){
        Serial.println("Error abriendo datos.csv");
        return 0;
    }

    if (!file.seek(offset)){
        Serial.println("Error haciendo seek en datos.csv");
        file.close();
        return 0;
    }

    size_t bytesRead = file.read(buffer, maxBytes);
    file.close();
    return bytesRead;
}


float ver_espacio_libre(void){
    uint64_t totalSize =  SD.totalBytes();
    uint64_t usedSize =  SD.usedBytes();

    return 1.0f - ((float)usedSize / (float)totalSize);
}


bool deleteCSV(){
    const char* filePath = "/datos.csv";

    if (!SD.exists(filePath)){
        Serial.println("El archivo no existe");
        return false;
    }

    if (SD.remove(filePath)){
        Serial.println("CSV borrado correctamente");
        return true;
    }

    Serial.println("Error al borrar el CSV");
    return false;
}


