#include "Arduino.h"
#include "OBD9141.h"

/*
  On the Arduino DUE (version 1.6.8) using the Serial port pins as digital IO
  pins is problematic. In particular, it seems you cannot use the port as a
  serial port after using pinMode() on the pins.

  We fix this by using an extra pin; we use this extra pin to create the slow
  5 baud init pattern while the Serial pins remain in use for the serial port.

  So connect any digital pin to the Tx pin of the serial port and update the 
  number accordingly.
*/

// In this example, Serial port 'Serial1' is used. The native USB port is used
// to provide information.

// The real Rx pin of the serial port, such that we can enable the pullup:
#define RX_PIN 19

// An extra pin connected to the Tx pin of the Serial port used. 
#define TX_PIN 5
// So this pin has a direct connection to pin 18 (TX1)

// The ENable pin for my SN65HVDA195 breakout is connected to pin 2.
#define EN_PIN 2


OBD9141 obd;


void setup(){
    SerialUSB.begin(9600);
    delay(2000);

    pinMode(EN_PIN, OUTPUT);
    digitalWrite(EN_PIN, HIGH);

    obd.begin(Serial1, RX_PIN, TX_PIN);
}
    
void loop(){
    SerialUSB.println("Looping");

    bool init_success =  obd.init();
    SerialUSB.print("init_success:");
    SerialUSB.println(init_success);

    // init_success = true;
    // Uncomment this line if you use the simulator to force the init to be
    // interpreted as successful. With an actual ECU; be sure that the init is 
    // succesful before trying to request PID's.

    if (init_success){
        bool res;
        while(1){
            res = obd.getCurrentPID(0x11, 1);
            if (res){
                SerialUSB.print("Result 0x11 (throttle): ");
                SerialUSB.println(obd.readUint8());
            }
            
            res = obd.getCurrentPID(0x0C, 2);
            if (res){
                SerialUSB.print("Result 0x0C (RPM): ");
                SerialUSB.println(obd.readUint16()/4);
            }


            res = obd.getCurrentPID(0x0D, 1);
            if (res){
                SerialUSB.print("Result 0x0D (speed): ");
                SerialUSB.println(obd.readUint8());
            }
            SerialUSB.println();

            delay(200);
        }
    }
    delay(3000);
}




