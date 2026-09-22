#pragma once

#include <Arduino.h>
#include <SD.h>
#include <customMeas.h>
#include <customTCP.h>


bool escribir_dato_a_SD(Measurement& meas);
bool escribir_sync_a_SD(SyncState& state);
bool obtener_size_datos(size_t& size);
size_t leer_bloque_datos(size_t offset, uint8_t* buffer, size_t maxBytes);
float ver_espacio_libre(void);
bool deleteCSV();