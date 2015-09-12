#include "WProgram.h"
#include "OBD9141sim.h"

#define RX_PIN 0
#define TX_PIN 1


extern "C" int main(void) {
    Serial.begin(9600);
    delay(2000);
    
    Serial.println("Startup.");

    const bool dynamic = true;

    // setup the car object.
    OBD9141sim car = OBD9141sim(Serial1, RX_PIN, TX_PIN);
    car.keep_init_state(true); // force it to hold init state.
    car.initialize(); // set it to initialised.

    pinMode(13, OUTPUT);


    // set default answers.
    uint8_t throttle = 75;
    car.setAnswer(0x01, 0x11, throttle);
    uint16_t rpm = 2200*4;
    car.setAnswer(0x01, 0x0C, rpm);

    // Set supported flags for these two answers
    car.setAnswer(0x01, 0x00, 4, (uint32_t) ((1<<0x11) + (1 <<0x0c)));


    Serial.println("Looping simulation.");
    while (1){
        digitalWrite(13,!digitalRead(13)); // toggle led.
        car.loop(); // check if we have requests

        // update the answers based on the summation of two sines.
        if (dynamic){
            float x = millis();
            x /= 100; // make the timescale more fun :)
            float v = -sin(2*3.1415 * (1.0/50.0)*x) + sin(2*3.1415 * (1.0/40.0)*x);
            // varies between -2 and 2....
            car.setAnswer(0x01, 0x11, (uint8_t) (((v + 2 + 2) / 4.0) * 100.0));
            car.setAnswer(0x01, 0x0C, (uint16_t) ((((v + 2) / 4.0) * 200.0) *4));
        }
    }
}




