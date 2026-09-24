#pragma once

#include <Arduino.h>
#include <SD.h>

#include <customMeas.h>
#include <customTCP.h>
#include <customNVS.h>


constexpr size_t MAX_CSV_SIZE = 10UL * 1024UL * 1024UL;


bool escribir_dato_a_SD(Measurement& meas, uint16_t& fileId, size_t& fileSize);
bool escribir_sync_a_SD(SyncState& state, uint16_t& fileId, size_t& fileSize);
bool obtener_size_datos(size_t& size);
bool crear_archivo_csv(uint16_t& fileId);
bool borrar_archivo_csv(uint16_t& fileId);

size_t leer_bloque_datos(size_t offset, uint8_t* buffer, size_t maxBytes);

float ver_espacio_usado(void);