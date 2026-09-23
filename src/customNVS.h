#pragma once

#include <Arduino.h>


struct StorageState{
    uint16_t oldest_file;
    uint16_t current_writing_file;
    uint32_t last_sent_timestamp;
};


// Inicializar acceso a NVS
bool init_NVS();


// Cargar el estado completo.
// Si alguna variable todavía no existe en NVS,
// se crea automáticamente con su valor inicial.
bool cargar_storage_state(StorageState& state);


// Guardar variables individuales
bool guardar_oldest_file(uint16_t& fileId);

bool guardar_current_writing_file(uint16_t& fileId);

bool guardar_last_sent_timestamp(uint32_t& timestamp);