#include "Arduino.h"
#define OBD9141_DEBUG 1
#include "OBD9141.h"

#define RX_PIN 0
#define TX_PIN 1
#define EN_PIN 2

/*
  This example is untested
*/

OBD9141 obd;


void setup(){
    Serial.begin(9600);
    delay(2000);

    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, HIGH);

    obd.begin(Serial1, RX_PIN, TX_PIN);

}
    
void loop(){
    Serial.println("Looping");

    bool init_success =  obd.init();
    Serial.print("init_success:");
    Serial.println(init_success);

    //init_success = true;
    // Uncomment this line if you use the simulator to force the init to be
    // interpreted as successful. With an actual ECU; be sure that the init is 
    // succesful before trying to request PID's.

    uint8_t dtc_buf[5];

    if (init_success){
        uint8_t res;
        while(1){
            res = obd.readTroubleCodes();
            if (res){
                Serial.print("Read ");
                Serial.print(res);
                Serial.print(" codes:");
                for (uint8_t index=0; index < res; index++)
                {
                  // convert the DTC bytes from the buffer into readable string
                  OBD9141::decodeDTC(obd.getTroubleCode(index), dtc_buf);
                  Serial.write(dtc_buf, 5);
                  Serial.println();
                }
            }
            Serial.println();

            delay(200);
        }
    }
    delay(3000);
}




