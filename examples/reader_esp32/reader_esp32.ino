#include <Arduino.h>
#include "OBD9141.h"

// ESP32 example, courtesy of gauix4; https://github.com/iwanders/OBD9141/issues/38

#define RX_PIN 16 // connect to transceiver Rx for ESP32
#define TX_PIN 17 // connect to transceiver Tx

bool init_success;

OBD9141 obd;

void setup(){
    Serial.begin(115200);
    obd.begin(Serial2, RX_PIN, TX_PIN);
    delay(2000);
    Serial.println("initialization");
    Serial.println("");
  
    while (!obd.init()){
        return;
    }
}

void loop(){
    if (obd.getCurrentPID(0x11, 1)){
        Serial.print("Result 0x11 (throttle): ");
        Serial.println(obd.readUint8());
    }

    if (obd.getCurrentPID(0x0C, 2)){
        Serial.print("Result 0x0C (RPM): ");
        Serial.println(obd.readUint16()/4);
    }

    if (obd.getCurrentPID(0x0D, 1)){
        Serial.print("Result 0x0D (speed): ");
        Serial.println(obd.readUint8());
    }
    Serial.println();
}
