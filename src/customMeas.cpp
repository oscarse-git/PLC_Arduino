#include <M5StamPLC.h>
#include <customMeas.h>


constexpr uint8_t IN_PIN_LIST[8] = {4, 5, 6, 7, 12, 13, 14, 15};
uint8_t tempPin;


bool lecturaEntrada(Measurement& meas){
    leerPines(tempPin);

    if (tempPin != meas.pinState){
        meas.pinState = tempPin;
        meas.timestamp = millis();
        return true;
    }else{
        return false;
    }
}

void leerPines(uint8_t& pinState){
    auto& aw = M5StamPLC.getIOExpanderB();
    
    // Leo los datos del registro correspondiente
    uint16_t raw = 
        ((uint16_t)aw.readRegister8(0x01) << 8) |
         (uint16_t)aw.readRegister8(0x00);

    // ordeno la salida para sacar el estado de las entradas como un int de 8bits (bit1 = in1)
    for (int i = 0; i < 8; i++){
        if (raw & (1 << IN_PIN_LIST[i])){
            pinState |= (1<<i);
        }else{
            pinState &= ~(1 << i);
        };
    }
}


