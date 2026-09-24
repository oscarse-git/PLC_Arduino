#include <customNVS.h>

#include <Preferences.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>


namespace{
    Preferences preferences;
    SemaphoreHandle_t nvsMutex = nullptr;
    
    constexpr const char* NVS_NAMESPACE = "plc_storage";

    constexpr const char* KEY_OLDEST_FILE = "oldest";
    constexpr const char* KEY_CURRENT_FILE = "current";
    constexpr const char* KEY_LAST_SENT    = "last_sent";

    constexpr uint16_t DEFAULT_OLDEST_FILE = 0;
    constexpr uint16_t DEFAULT_CURRENT_FILE = 0;
    constexpr uint32_t DEFAULT_LAST_SENT_TIMESTAMP = 0;
}


bool init_NVS(){
    nvsMutex = xSemaphoreCreateMutex();

    if (nvsMutex == nullptr){
        Serial.println("ERROR,CREANDO_nvsMutex");
        return false;
    }

    if (!preferences.begin(NVS_NAMESPACE, false)){
        Serial.println("ERROR inicializando NVS");
        return false;
    }

    Serial.println("NVS Inicializado");
    return true;
}

bool cargar_storage_state(StorageState& state){

    xSemaphoreTake(nvsMutex, portMAX_DELAY);

    // Inicializar valores si no existen
    if (!preferences.isKey(KEY_OLDEST_FILE)){

        if (preferences.putUShort(KEY_OLDEST_FILE, DEFAULT_OLDEST_FILE) == 0){
            
            xSemaphoreGive(nvsMutex);
            Serial.println("ERROR creando oldest_file en NVS");
            return false;
        }
    }


    if (!preferences.isKey(KEY_CURRENT_FILE)){

        if (preferences.putUShort(KEY_CURRENT_FILE, DEFAULT_CURRENT_FILE) == 0){

            xSemaphoreGive(nvsMutex);
            Serial.println("ERROR creando current_writing_file en NVS");
            return false;
        }
    }


    if (!preferences.isKey(KEY_LAST_SENT)){

        if (preferences.putUInt(KEY_LAST_SENT, DEFAULT_LAST_SENT_TIMESTAMP) == 0){

            xSemaphoreGive(nvsMutex);
            Serial.println("ERROR creando last_sent_timestamp en NVS");
            return false;
        }
    }

    // Cargar a RAM
    state.oldest_file = preferences.getUShort(KEY_OLDEST_FILE, DEFAULT_OLDEST_FILE);
    state.current_writing_file = preferences.getUShort(KEY_CURRENT_FILE, DEFAULT_CURRENT_FILE);
    state.last_sent_timestamp = preferences.getUInt(KEY_LAST_SENT, DEFAULT_LAST_SENT_TIMESTAMP);

    xSemaphoreGive(nvsMutex);
    return true;
}

bool guardar_oldest_file(uint16_t& fileId){

    xSemaphoreTake(nvsMutex, portMAX_DELAY);
    size_t bytesWritten = preferences.putUShort(KEY_OLDEST_FILE, fileId);
    xSemaphoreGive(nvsMutex);


    if (bytesWritten == 0){
        Serial.println("ERROR guardando oldest_file en NVS");
        return false;
    }

    return true;
}

bool guardar_current_writing_file(uint16_t& fileId){
    xSemaphoreTake(nvsMutex, portMAX_DELAY);
    size_t bytesWritten = preferences.putUShort(KEY_CURRENT_FILE, fileId);
    xSemaphoreGive(nvsMutex);

    if (bytesWritten == 0){
        Serial.println("ERROR guardando current_writing_file en NVS");
        return false;
    }

    return true;
}

bool guardar_last_sent_timestamp(uint32_t& timestamp){
    xSemaphoreTake(nvsMutex, portMAX_DELAY);
    size_t bytesWritten = preferences.putUInt(KEY_LAST_SENT, timestamp);
    xSemaphoreGive(nvsMutex);

    if (bytesWritten == 0){
        Serial.println("ERROR guardando last_sent_timestamp en NVS");
        return false;
    }

    return true;
}