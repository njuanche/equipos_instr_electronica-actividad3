#include "ComunicacionIR.h"
#include <IRremote.h>

#define PIN_IR 8
extern float tempDeseada;

IRrecv receptorIR(PIN_IR);
decode_results resultado;

void setupComunicacionIR() {
  IrReceiver.begin(PIN_IR, ENABLE_LED_FEEDBACK); 
}

void loopComunicacionIR() {
  if (IrReceiver.decode()) {
    unsigned long valor = IrReceiver.decodedIRData.decodedRawData;
    Serial.print("Código IR recibido: ");
    Serial.println(valor, HEX);

    switch (valor) {
      case 0xFD02FF00:  // Botón signo +
        Serial.println("Botón POWER: subir +1°C");
        if (tempDeseada < 35.0) {
          tempDeseada += 1.0;
          Serial.print("Nueva temperatura deseada: ");
          Serial.println(tempDeseada);
        }
        break;

      case 0x6798FF00:  // Botón signo -
        Serial.println("Botón FUNC/STOP: bajar -1°C");
        if (tempDeseada > 15.0) {
          tempDeseada -= 1.0;
          Serial.print("Nueva temperatura deseada: ");
          Serial.println(tempDeseada);
        }
        break;
    }

    IrReceiver.resume(); // Preparar para siguiente código IR
  }
}
