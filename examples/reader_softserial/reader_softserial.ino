#include "Arduino.h"
// Be sure that the AltSoftSerial library is available, download it from http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html"
#include <AltSoftSerial.h>


#include "OBD9141.h"

#define RX_PIN 8
#define TX_PIN 9

AltSoftSerial altSerial;

OBD9141 obd;


void setup(){
    Serial.begin(9600);
    delay(2000);

    obd.begin(altSerial, RX_PIN, TX_PIN);

}
    
void loop(){
    Serial.println("Looping");

    bool init_success =  obd.init();
    Serial.print("init_success:");
    Serial.println(init_success);

    init_success = true; // assume that the init was a success (for example with the simulator)

    if (init_success){
        bool res;
        while(1){
            res = obd.getCurrentPID(0x11, 1);
            if (res){
                Serial.print("Result 0x11 (throttle): ");
                Serial.println(obd.readUint8());
            }
            
            res = obd.getCurrentPID(0x0C, 2);
            if (res){
                Serial.print("Result 0x0C (RPM): ");
                Serial.println(obd.readUint16()/4);
            }


            res = obd.getCurrentPID(0x0D, 1);
            if (res){
                Serial.print("Result 0x0D (speed): ");
                Serial.println(obd.readUint8());
            }
            Serial.println();

            delay(200);
        }
    }
    delay(3000);
}




