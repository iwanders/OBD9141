/*
 *  Copyright (c) 2015, Ivor Wanders
 *  MIT License, see the LICENSE.md file in the root folder.
*/

#include "OBD9141sim.h"


OBD9141sim::OBD9141sim(){};

void OBD9141sim::begin(OBD_SERIAL_DATA_TYPE & serial_port, uint8_t rx_pin, uint8_t tx_pin){
    this->serial = &serial_port;
    this->tx_pin = tx_pin;
    this->rx_pin = rx_pin;
    this->initialised = false;
    this->init_hold_value = 0;
    this->init_state = 0;
    this->serial->begin(OBD9141_KLINE_BAUD);
    for (uint8_t i=0; i< OBD9141SIM_ANSWER_LIST; i++){
        this->answers[i].mode = 0;
        this->answers[i].pid = 0;
    }
    // pinMode(this->rx_pin, INPUT);
    #ifdef OBD9141SIM_SIMULATE_INIT
        OBD9141_sim_instance = this;
        attachInterrupt(this->rx_pin, rx_interrupt, CHANGE); 
    #endif
}

void OBD9141sim::uninitialize(){
    OBD9141print("[SIM] uninitialising");
    this->last_change = millis();
    this->initialised = false;
    this->init_hold_value = 0;
    this->init_state = 0;
    this->serial->end();
    #ifdef OBD9141SIM_SIMULATE_INIT
        attachInterrupt(this->rx_pin, rx_interrupt, CHANGE); 
    #endif
}

void OBD9141sim::keep_init_state(bool force_init_state){
    this->force_init_state = force_init_state;
}

void OBD9141sim::initialize(){
    this->serial->begin(OBD9141_KLINE_BAUD);
    this->initialised = true;
}

void OBD9141sim::initializeKWP(){
    this->use_kwp_ = true;
    this->serial->begin(OBD9141_KLINE_BAUD);
    this->initialised = true;
}

void OBD9141sim::setAnswer(uint8_t mode, uint8_t pid, uint8_t answer){
    this->setAnswer(mode, pid, 1, answer);
}
void OBD9141sim::setAnswer(uint8_t mode, uint8_t pid, uint16_t answer){
    this->setAnswer(mode, pid, 2, answer);
}
void OBD9141sim::setAnswer(uint8_t mode, uint8_t pid, uint8_t len, uint32_t answer){
    for (uint8_t i=0; i < OBD9141SIM_ANSWER_LIST; i++){
        if ((this->answers[i].mode == 0) or ((this->answers[i].mode == mode) and (this->answers[i].pid == pid))){
            // empty one... fill it.
            this->answers[i].mode = mode;
            this->answers[i].pid = pid;
            this->answers[i].len = len;
            this->answers[i].answer = answer;
            return;
        }
    }
}

void OBD9141sim::answerRequest(uint8_t mode, uint8_t pid){
    //    raw request: {0x68, 0x6A, 0xF1, 0x01, 0x0D}
    //    returns:      0x48  0x6B  0x11  0x41  0x0D  0x00  0x12
    odb_answer_entry* answer = 0;
    for (uint8_t i=0; i < OBD9141SIM_ANSWER_LIST; i++){
        if ((this->answers[i].mode == mode) and (this->answers[i].pid == pid)){
            answer = &(this->answers[i]);
            break;
        }
    }

    if (answer == 0){
        // return not supported message?
        return;
    }

    
    uint8_t message_length = 3+2+answer->len;
    OBD9141print("[SIM] have an answer to request, mode: ");
    OBD9141print(mode);OBD9141print(" , pid:");
    OBD9141print(pid);OBD9141print(", length:");
    OBD9141println(message_length+1);
    uint8_t message[message_length+1];
    message[0] = 0x48;
    message[1] = 0x6B;
    message[2] = 0x11;
    message[3] = 0x40 + mode; // is this actually what happens!?
    message[4] = pid;
    message[5] = 0;
    if (use_kwp_){
        // now we modify the message for KWP, the payload is the request_len - 3 header bytes
        message[0] = 0x82 + (answer->len );
        message[1] = 0xF1;
    }
    //memcpy(&(message[5]), &(answer->answer), answer->len);
    // cannot memcpy, need to swap the answer 
    for (uint8_t i=0; i < (answer->len) ; i++){
        message[answer->len-1 - i +5] = answer->data[i];
    }
    
    message[message_length] = OBD9141::checksum(&(message[0]), message_length);
    // for (uint8_t i=0; i < (message_length+1); i++){
        // OBD9141print("[SIM] message[");OBD9141print(i);OBD9141print("]:"); OBD9141println(message[i]);
    // }
    OBD9141print("[SIM] calcd checksum:"); OBD9141println(message[message_length]);
    delay(OBD9141SIM_AFTER_REQUEST_DELAY+1);
    this->write(&(message[0]), message_length+1);
    

}

void OBD9141sim::handleRequests(){
    
    //uint8_t message[5] = {0x68, 0x6A, 0xF1, mode, pid};  and checksum
    uint8_t message[6] = {0};
    uint8_t number_of_bytes = 0;
  
    this->serial->setTimeout(OBD9141_INTERSYMBOL_WAIT*7 + 10 + 6*OBD9141_REQUEST_ANSWER_MS_PER_BYTE);
    if (this->serial->available()){
        number_of_bytes = this->serial->readBytes(message, 6);

        if (number_of_bytes == 6){
            OBD9141println("[SIM] Got request");
            if (message[5] == OBD9141::checksum(&message, 5)){
                OBD9141println("[SIM] Checksum matches.");
                this->answerRequest(message[3], message[4]);

            }
        } else {
            OBD9141println("[SIM] Failed reading stuff");
        }
        uint8_t zero_count = 0;
        for (uint8_t i=0; i < 6 ; i++){
            if (message[i] == 0){
                zero_count++;
            }
        }
        if (zero_count == 6){
            
            if (not this->force_init_state){
                this->uninitialize();
            }
        }
    }
}

void OBD9141sim::loop(){
    #ifdef OBD9141SIM_SIMULATE_INIT
    if (not this->initialised){
        this->handleInit();
    } else {
        //we are initialised
        this->handleRequests();
    }
    #else
        this->handleRequests();
    #endif
}
void OBD9141sim::write(uint8_t b){
    // OBD9141print("w: "); OBD9141println(b);
    this->serial->write(b);
    
    this->serial->setTimeout(OBD9141_REQUEST_ECHO_MS_PER_BYTE * 1 + OBD9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[1]; // temporary variable to read into.
    this->serial->readBytes(tmp, 1);
}

void OBD9141sim::write(void* b, uint8_t len){
    for (uint8_t i=0; i < len ; i++){
        // OBD9141print("w: ");OBD9141println(reinterpret_cast<uint8_t*>(b)[i]);
        this->serial->write(reinterpret_cast<uint8_t*>(b)[i]);
        delay(OBD9141_INTERSYMBOL_WAIT);
    }
    this->serial->setTimeout(OBD9141_REQUEST_ECHO_MS_PER_BYTE * len + OBD9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[len]; // temporary variable to read into.
    this->serial->readBytes(tmp, len);
}



#ifdef OBD9141SIM_SIMULATE_INIT

// static ISR method which fires our singleton's method.
void OBD9141sim::rx_interrupt () {
    OBD9141_sim_instance->processInterrupt ();  
}

void OBD9141sim::processInterrupt () {
    bool new_value = digitalRead(this->rx_pin);

    if ((not this->initialised) and (this->init_state==0)){

        this->last_change =millis();
        this->init_hold_value <<= 1;
        this->init_hold_value += new_value;
        OBD9141print("[SIM] init_hold_value: "); OBD9141println(init_hold_value);
    }
}

void OBD9141sim::handleInit(){
    uint8_t v1 = 0x08;
    uint32_t current_time = millis();

    if ((current_time) > (this->last_change + OBD9141SIM_INIT_ATTEMPT_TIMEOUT)){
        this->init_hold_value = 0;
        this->init_state = 0;
    }

    if ((this->init_state == 0) and (this->init_hold_value == 21) and (((millis()) > (this->last_change + OBD9141SIM_WAIT_AFTER_5BAUD)))){
        // send the 0x55 byte.
        OBD9141print("[SIM] dropping to serial port and sending 0x55: ");
        // OBD9141print("[SIM] Serial: ");
        // OBD9141println((uint32_t) this->serial);

        delay(10);
        this->serial->begin(OBD9141_KLINE_BAUD);
        // OBD9141print("[SIM] Below begin... ");
        this->write(0x55);
        this->init_state = 1;
        this->last_change = current_time;
    }
    if ((this->init_state == 1) and (((current_time) > (this->last_change + OBD9141SIM_WAIT_BEFORE_V1)))){
        this->write(v1);
        this->init_state = 2;
        this->last_change = current_time;
        
    }
    if ((this->init_state == 2) and (((current_time) > (this->last_change + OBD9141SIM_WAIT_BEFORE_V1)))){
        this->write(v1);
        this->init_state = 3;
        this->last_change = current_time;
        
    }
    if ((this->init_state == 3) and (this->serial->available())){
        //this->write(0x08);
        uint8_t rec = this->serial->read();
        uint8_t inv = ~rec;
        if (v1 == inv){
            this->init_state = 4;
            this->last_change = current_time;
            OBD9141println("[SIM] Got inverse, going to send 0xcc");
        } else {
            OBD9141println("[SIM] fail");
            this->uninitialize();
        }
    }
    if ((this->init_state == 4) and (((current_time) > (this->last_change + OBD9141SIM_WAIT_BEFORE_0xCC)))){
            this->last_change = millis();
            this->write(0xcc);
            OBD9141print("[SIM] Init done.");
            if (not this->force_init_state){
                this->initialised = true;
            }
    }
}

// pointer to instance.
OBD9141sim * OBD9141sim::OBD9141_sim_instance;

#endif