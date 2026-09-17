#pragma once

#include <M5StamPLC.h>
#include <Arduino.h>

struct Measurement {
    uint32_t timestamp;
    uint8_t pinState;
};

void leerPines(uint8_t& pinState);
bool lecturaEntrada(Measurement& meas);