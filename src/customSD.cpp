#include <Arduino.h>
#include <SD.h>
#include <customMeas.h>


bool escribir_dato_a_SD(Measurement& meas){
    File file = SD.open("/datos.csv", FILE_APPEND);

    if (!file) {
        Serial.println("Error abriendo datos.csv");
        return false;
    }

    file.print(meas.timestamp);
    file.print(",");
    file.println(meas.pinState);

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