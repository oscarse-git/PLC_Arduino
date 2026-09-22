#include <Arduino.h>
#include <SD.h>

#include <customMeas.h>
#include <customTCP.h>
#include <customSD.h>

bool escribir_dato_a_SD(Measurement& meas){
    File file = SD.open("/datos.csv", FILE_APPEND);

    if (!file) {
        Serial.println("Error abriendo datos.csv");
        return false;
    }

    file.print("DATA,");
    file.print(meas.timestamp);
    file.print(",");
    file.println(meas.pinState);

    file.close();
    return true;
}


bool escribir_sync_a_SD(SyncState& state){

    File file = SD.open("/datos.csv", FILE_APPEND);

    if (!file){
        Serial.println("Error abriendo datos.csv");
        return false;
    }

    state.plcTime = state.startTime + (state.endTime - state.startTime) / 2;

    file.print("SYNC,");
    file.print(state.plcTime);
    file.print(",");
    file.println(state.serverTime);

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


