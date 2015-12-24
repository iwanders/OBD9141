#include "Arduino.h"
#include "OBD9141.h"

#define RX_PIN 0
#define TX_PIN 1


OBD9141 obd;


void setup(){
    Serial.begin(9600);
    delay(2000);

    // The EN pin for my SN65HVDA195 breakout is connected to pin 2.
    // To enable the chip I set this pin to high.
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);


    obd.begin(Serial1, RX_PIN, TX_PIN);

}
    
void loop(){
    Serial.println("Looping");
    Serial.print("OBD9141_WAIT_FOR_ECHO_TIMEOUT: ");Serial.println(OBD9141_WAIT_FOR_ECHO_TIMEOUT);
    Serial.print("OBD9141_INTERSYMBOL_WAIT:      ");Serial.println(OBD9141_INTERSYMBOL_WAIT);

    bool init_success =  obd.init();
    Serial.print("init_success: ");
    Serial.println(init_success);

    // init_success = true; // force succes when using simulator

    // with Teensy I'd use elapsedMillis, but use millis() for Arduino
    // compatibility.

    uint32_t start_time;
    bool res;
    while(init_success){
        start_time = millis();
        res = obd.getCurrentPID(0x11, 1);
        if (res){
            Serial.print("Throttle: ");
            Serial.print(obd.readUint8());
        }
        if (!res){
            Serial.println("Reading 0x11, throttle failed, reinitializing bus.");
            init_success = false; // ensure reconfigure if bus failed.
        }
        Serial.print(" [");
        Serial.print(millis() - start_time);
        Serial.println(" ms]");
        
        
        start_time = millis();
        res = obd.getCurrentPID(0x0C, 2);
        if (res){
            Serial.print("RPM:      ");
            Serial.print(obd.readUint16()/4);
        }
        if (!res){
            Serial.print("Reading 0x0C failed, reinitializing bus.");
            init_success = false; // ensure reconfigure if bus failed.
        }
        Serial.print(" [");
        Serial.print(millis() - start_time);
        Serial.println(" ms]");


    }

    delay(3000);
}




