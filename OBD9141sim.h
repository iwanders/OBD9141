/*
 *  Copyright (c) 2015, Ivor Wanders
 *  MIT License, see the LICENSE.md file in the root folder.
*/

#ifndef OBD9141sim_H
#define OBD9141sim_H


#include "Arduino.h"

#define OBD9141_DEBUG
#include "OBD9141.h"


#define OBD9141SIM_ANSWER_LIST 8
// length of the list containing potential answers

#define OBD9141SIM_AFTER_REQUEST_DELAY 20


// It is possible to simulate the handshake, however, this handshake simulation
// is not tested thoroughly, it appears to work. Unnecessary for testing it is
// disabled by default. It requires a singleton instance.
// If you wish to enable simulating the init sequence, uncomment this line:
// #define OBD9141SIM_SIMULATE_INIT

#ifdef OBD9141SIM_SIMULATE_INIT
    #define OBD9141SIM_INIT_ATTEMPT_TIMEOUT 2500
    // atleast 3000 ms... otherwise we do not reset init attempt with 3s bus high.

    #define OBD9141SIM_WAIT_AFTER_5BAUD 300
    // 200 ms minimum (stop bit), then 300 ms timeout

    #define OBD9141SIM_WAIT_BEFORE_V1 10
    // time before sending V1

    #define OBD9141SIM_WAIT_BEFORE_0xCC 20
    // time before sending 0xCC
#endif


// struct to hold relevant data for answers.
typedef struct{
    uint8_t mode;
    uint8_t pid;
    uint8_t len;
    union {
        uint32_t answer;
        uint8_t data[4];
    };
} odb_answer_entry;



class OBD9141sim{
    private:
        odb_answer_entry answers[OBD9141SIM_ANSWER_LIST];

        OBD_SERIAL_DATA_TYPE* serial;
        uint8_t tx_pin;
        uint8_t rx_pin;

        bool initialised;
        bool force_init_state;
        uint8_t init_hold_value;
        uint8_t init_state;
        uint32_t last_change;

        void write(uint8_t b);
        // writes a byte and removes the echo.

        void write(void* b, uint8_t len);
        // writes an array and removes the echo.

        void handleRequests();
        void answerRequest(uint8_t mode, uint8_t pid);

        #ifdef OBD9141SIM_SIMULATE_INIT
        void handleInit();
        static void rx_interrupt ();
        // static version which calls processInterrupt in on our singleton.
        void processInterrupt ();
        static OBD9141sim * OBD9141_sim_instance;
        #endif


  
  public:
        OBD9141sim();
        // Advised usage:
        //  OBD9141sim car;
        //  car.begin(Serial1, RX_PIN, TX_PIN);
        //  car.keep_init_state(true);
        //  car.initialize();
        //
        // Usage with init enabled (set OBD9141SIM_SIMULATE_INIT).
        // OBD9141sim car = OBD9141sim(Serial1, RX_PIN, TX_PIN);

        void begin(OBD_SERIAL_DATA_TYPE & serial_port, uint8_t rx_pin, uint8_t tx_pin);
        // begin method which allows setting the serial port and pins.

        // Answers are kept based on their unique (mode, pid) tuple.

        void setAnswer(uint8_t mode, uint8_t pid, uint8_t answer);
        // sets the answer to request at mode, pid to the uint8_t answer value.

        void setAnswer(uint8_t mode, uint8_t pid, uint16_t answer);
        // sets the answer to request at mode, pid to the uint16_t answer value.

        void setAnswer(uint8_t mode, uint8_t pid, uint8_t len, uint32_t answer);
        // sets the answer to request at mode, pid to the first len bytes of
        // answer.

        void loop();
        // needs to be called often enough to process serial information or the
        // init state machine.

        void uninitialize();
        // no communication possible, waiting for initialisation handshake.
        // if the handshake is successful the state is switch to initialized.

        void initialize();
        // puts the simulation to the initialised state, communication possible.
        // not listening for the handshake.

        void keep_init_state(bool force_init_state);
        // keep the state to the one set with the initialize and uninitialize
        // methods.
};


#endif