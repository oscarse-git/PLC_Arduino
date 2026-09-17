#include <Arduino.h>
#include <SD.h>
#include <customMeas.h>


bool escribir_dato_a_SD(Measurement& meas);
bool obtener_size_datos(size_t& size);
size_t leer_bloque_datos(size_t offset, uint8_t* buffer, size_t maxBytes);