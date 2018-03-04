/*
 *  Copyright (c) 2015, Ivor Wanders
 *  MIT License, see the LICENSE.md file in the root folder.
*/
#include "OBD9141.h"

OBD9141::OBD9141(){};

void OBD9141::begin(OBD_SERIAL_DATA_TYPE & serial_port, uint8_t rx_pin, uint8_t tx_pin){
    this->serial = &serial_port;
    this->tx_pin = tx_pin;
    this->rx_pin = rx_pin;

    // Enable the pullup on the Rx Pin, this is not changed by set_port.
    pinMode(this->rx_pin, INPUT);
    digitalWrite(this->rx_pin, HIGH);
    this->set_port(true); // prevents calling this->serial->end() before start.
}

void OBD9141::set_port(bool enabled){
    if (enabled){
        // Work around the incorrect pinmode configuration in Due.
        #ifdef ARDUINO_SAM_DUE
          g_APinDescription[this->rx_pin].pPort -> PIO_PDR = g_APinDescription[this->rx_pin].ulPin;
          g_APinDescription[this->tx_pin].pPort -> PIO_PDR = g_APinDescription[this->tx_pin].ulPin;
        #endif
        this->serial->begin(OBD9141_KLINE_BAUD);
    } else {
        this->serial->end();
        #ifdef ARDUINO_SAM_DUE
          g_APinDescription[this->rx_pin].pPort -> PIO_PER = g_APinDescription[this->rx_pin].ulPin; 
          g_APinDescription[this->tx_pin].pPort -> PIO_PER = g_APinDescription[this->tx_pin].ulPin; 
        #endif
        pinMode(this->tx_pin, OUTPUT);
        digitalWrite(this->tx_pin, HIGH);
    }
}

void OBD9141::kline(bool enabled){
    digitalWrite(this->tx_pin, enabled);
}


uint8_t OBD9141::checksum(void* b, uint8_t len){
    uint8_t ret = 0;
    for (uint8_t i=0; i < len; i++){
        ret += reinterpret_cast<uint8_t*>(b)[i];
    }
    return ret;
}

void OBD9141::write(uint8_t b){
    // OBD9141print("w: "); OBD9141println(b);
    this->serial->write(b);
    
    this->serial->setTimeout(OBD9141_REQUEST_ECHO_MS_PER_BYTE * 1 + OBD9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[1]; // temporary variable to read into.
    this->serial->readBytes(tmp, 1);
}

void OBD9141::write(void* b, uint8_t len){
    for (uint8_t i=0; i < len ; i++){
        // OBD9141print("w: ");OBD9141println(reinterpret_cast<uint8_t*>(b)[i]);
        this->serial->write(reinterpret_cast<uint8_t*>(b)[i]);
        delay(OBD9141_INTERSYMBOL_WAIT);
    }
    this->serial->setTimeout(OBD9141_REQUEST_ECHO_MS_PER_BYTE * len + OBD9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[len]; // temporary variable to read into.
    this->serial->readBytes(tmp, len);
}

bool OBD9141::request(void* request, uint8_t request_len, uint8_t ret_len){
    uint8_t buf[request_len+1];
    memcpy(buf, request, request_len); // copy request

    buf[request_len] = this->checksum(&buf, request_len); // add the checksum

    this->write(&buf, request_len+1);

    // wait after the request, officially 30 ms, but we might as well wait
    // for the data in the readBytes function.
    
    // set proper timeout
    this->serial->setTimeout(OBD9141_REQUEST_ANSWER_MS_PER_BYTE * ret_len + OBD9141_WAIT_FOR_REQUEST_ANSWER_TIMEOUT);
    memset(this->buffer, 0, ret_len+1);
    
    //OBD9141print("Trying to get x bytes: "); OBD9141println(ret_len+1);
    if (this->serial->readBytes(this->buffer, ret_len+1)){
        // for (uint8_t i=0; i< (ret_len+1); i++){
            // OBD9141print(this->buffer[i]);OBD9141print(" ");
        // };OBD9141println();
        
        return (this->checksum(&(this->buffer[0]), ret_len) == this->buffer[ret_len]);// have data; return whether it is valid.
    } else {
        OBD9141println("Timeout on reading bytes.");
        return false; // failed getting data.
    }
}

uint8_t OBD9141::request(void* request, uint8_t request_len){
    bool success = true;
    // wipe the entire buffer to ensure we are in a clean slate.
    memset(this->buffer, 0, OBD9141_BUFFER_SIZE);

    // create the request with checksum.
    uint8_t buf[request_len+1];
    memcpy(buf, request, request_len); // copy request
    buf[request_len] = this->checksum(&buf, request_len); // add the checksum

    // manually write the bytes onto the serial port
    // this does NOT read the echoes.
    OBD9141print("W: ");
#ifdef OBD9141_DEBUG
    for (uint8_t i=0; i < (request_len+1); i++){
        OBD9141print(buf[i]);OBD9141print(" ");
    };OBD9141println();
#endif
    for (uint8_t i=0; i < request_len+1 ; i++){
        this->serial->write(reinterpret_cast<uint8_t*>(buf)[i]);
        delay(OBD9141_INTERSYMBOL_WAIT);
    }

    // next step, is to read the echo from the serial port.
    this->serial->setTimeout(OBD9141_REQUEST_ECHO_MS_PER_BYTE * 1 + OBD9141_WAIT_FOR_ECHO_TIMEOUT);
    uint8_t tmp[request_len+1]; // temporary variable to read into.
    this->serial->readBytes(tmp, request_len+1);

    OBD9141print("E: ");
    for (uint8_t i=0; i < request_len+1; i++)
    {
#ifdef OBD9141_DEBUG
        OBD9141print(tmp[i]);OBD9141print(" ");
#endif
      // check if echo is what we wanted to send
      success &= (buf[i] == tmp[i]);
    }

    // so echo is dealt with now... next is listening to the reply, which is a variable number.
    // set the timeout for the first read to include the wait for answer timeout
    this->serial->setTimeout(OBD9141_REQUEST_ANSWER_MS_PER_BYTE * 1 + OBD9141_WAIT_FOR_REQUEST_ANSWER_TIMEOUT);

    uint8_t answer_length = 0;
    // while readBytes returns a byte, keep reading.
    while (this->serial->readBytes(&(this->buffer[answer_length]), 1) && (answer_length < OBD9141_BUFFER_SIZE))
    {
      answer_length++;
      this->serial->setTimeout(OBD9141_REQUEST_ANSWER_MS_PER_BYTE * 1);
    }

    OBD9141println();OBD9141print("A (");OBD9141print(answer_length);OBD9141print("): ");
#ifdef OBD9141_DEBUG
    for (uint8_t i=0; i < min(answer_length, OBD9141_BUFFER_SIZE); i++){
        OBD9141print(this->buffer[i]);OBD9141print(" ");
    };OBD9141println();
#endif

    // next, calculate the checksum
    bool checksum = (this->checksum(&(this->buffer[0]), answer_length-1) == this->buffer[answer_length - 1]);
    OBD9141print("C: ");OBD9141println(checksum);
    OBD9141print("S: ");OBD9141println(success);
    OBD9141print("R: ");OBD9141println(answer_length - 1);
    if (checksum && success)
    {
      return answer_length - 1;
    }
    return 0;
}

/*
    No header description to be found on the internet?

    raw request: {0x68, 0x6A, 0xF1, 0x01, 0x0D}
        returns:  0x48  0x6B  0x11  0x41  0x0D  0x00  0x12 
        returns 1 byte
        total of 7 bytes.

    raw request: {0x68, 0x6A, 0xF1, 0x01, 0x00}
        returns   0x48  0x6B  0x11  0x41  0x00  0xBE  0x3E  0xB8  0x11  0xCA
        returns 3 bytes
        total of 10 bytes

    Mode seems to be 0x40 + mode, unable to confirm this though.

*/


bool OBD9141::getPID(uint8_t pid, uint8_t mode, uint8_t return_length){
    uint8_t message[5] = {0x68, 0x6A, 0xF1, mode, pid};
    // header of request is 5 long, first three are always constant.

    bool res = this->request(&message, 5, return_length+5);
    // checksum is already checked, verify the PID.

    if (this->buffer[4] != pid){
        return false;
    }
    return res;
}

bool OBD9141::getCurrentPID(uint8_t pid, uint8_t return_length){
    return this->getPID(pid, 0x01, return_length);
}

bool OBD9141::clearTroubleCodes(){
    uint8_t message[4] = {0x68, 0x6A, 0xF1, 0x04};
    // 0x04 without PID value should clear the trouble codes or
    // malfunction indicator lamp.

    // No data is returned to this request, we expect the request itself
    // to be returned.
    bool res = this->request(&message, 4, 4);
    return res;
}

uint8_t OBD9141::readTroubleCodes()
{
  uint8_t message[4] = {0x68, 0x6A, 0xF1, 0x03};
  uint8_t response = this->request(&message, 4);
  if (response >= 4)
  {
    // OBD9141print("T: ");OBD9141println((response - 4) / 2);
    return (response - 4) / 2;  // every DTC is 2 bytes, header was 4 bytes.
  }
  return 0;
}

uint8_t OBD9141::readPendingTroubleCodes()
{
  uint8_t message[4] = {0x68, 0x6A, 0xF1, 0x07};
  uint8_t response = this->request(&message, 4);
  if (response >= 4)
  {
    // OBD9141print("T: ");OBD9141println((response - 4) / 2);
    return (response - 4) / 2;  // every DTC is 2 bytes, header was 4 bytes.
  }
  return 0;
}

uint8_t OBD9141::readUint8(){
    return this->buffer[5];
}

uint16_t OBD9141::readUint16(){
    return this->buffer[5]*256 + this->buffer[6]; // need to reverse endianness
}

uint8_t OBD9141::readUint8(uint8_t index){
    return this->buffer[5 + index];
}

uint8_t OBD9141::readBuffer(uint8_t index){
  return this->buffer[index];
}

uint16_t OBD9141::getTroubleCode(uint8_t index)
{
  return *reinterpret_cast<uint16_t*>(&(this->buffer[index*2 + 4]));
}

bool OBD9141::init(){
    // this function performs the ISO9141 5-baud 'slow' init.
    this->set_port(false); // disable the port.
    this->kline(true);
    delay(OBD9141_INIT_IDLE_BUS_BEFORE); // no traffic on bus for 3 seconds.
    OBD9141println("Before magic 5 baud.");
    // next, send the startup 5 baud init..
    this->kline(false); delay(200); // start
    this->kline(true); delay(400);  // first two bits
    this->kline(false); delay(400); // second pair
    this->kline(true); delay(400);  // third pair
    this->kline(false); delay(400); // last pair
    this->kline(true); delay(200);  // stop bit
    // this last 200 ms delay could also be put in the setTimeout below.
    // But the spec says we have a stop bit.

    // done, from now on it the bus can be treated ad a 10400 baud serial port.

    OBD9141println("Before setting port.");
    this->set_port(true);
    OBD9141println("After setting port.");
    uint8_t buffer[1];

    this->serial->setTimeout(300+200);
    // wait should be between 20 and 300 ms long

    // read first value into buffer, should be 0x55
    if (this->serial->readBytes(buffer, 1)){
        OBD9141print("First read is: "); OBD9141println(buffer[0]);
        if (buffer[0] != 0x55){
            return false;
        }
    } else {
        OBD9141println("Timeout on read 0x55.");
        return false;
    }
    // we get here after we have passed receiving the first 0x55 from ecu.


    this->serial->setTimeout(20); // w2 and w3 are pauses between 5 and 20 ms

    uint8_t v1=0, v2=0; // sent by car:  (either 0x08 or 0x94)

    // read v1
    if (!this->serial->readBytes(buffer, 1)){
        OBD9141println("Timeout on read v1.");
        return false;
    } else {
        v1 = buffer[0];
        OBD9141print("read v1: "); OBD9141println(v1);
    }

    // read v2
    if (!this->serial->readBytes(buffer, 1)){
        OBD9141println("Timeout on read v2.");
        return false;
    } else {
        v2 = buffer[0];
        OBD9141print("read v2: "); OBD9141println(v2);
    }
    
    OBD9141print("v1: "); OBD9141println(v1);
    OBD9141print("v2: "); OBD9141println(v2);

    // these two should be identical according to the spec.
    if (v1 != v2){
        return false;
    }

    // we obtained w1 and w2, now invert and send it back.
    // tester waits w4 between 25 and 50 ms:
    delay(30);
    this->write(~v2);
    this->serial->setTimeout(50); // w5 is same as w4...  max 50 ms

    // finally, attempt to read 0xCC from the ECU, indicating succesful init.
    if (!this->serial->readBytes(buffer, 1)){
        OBD9141println("Timeout on 0xCC read.");
        return false;
    } else {
        // OBD9141print("read 0xCC?: "); OBD9141println(buffer[0], HEX);
        if ((buffer[0] == 0xCC)){ // done if this is inverse of 0x33
            delay(OBD9141_INIT_POST_INIT_DELAY);
            // this delay is not in the spec, but prevents requests immediately
            // after the finishing of the init sequency.

            return true; // yay! we are initialised.
        } else {
            return false;
        }
    }
}


void OBD9141::decodeDTC(uint16_t input_bytes, uint8_t* output_string){
  const uint8_t A = reinterpret_cast<uint8_t*>(&input_bytes)[0];
  const uint8_t B = reinterpret_cast<uint8_t*>(&input_bytes)[1];
  const static char type_lookup[4] = {'P', 'C', 'B', 'U'};
  const static char digit_lookup[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  // A7-A6 is first dtc character, error type:
  output_string[0] = type_lookup[A >> 6];
  // A5-A4 is second dtc character
  output_string[1] = digit_lookup[(A >> 4) & 0b11];
  // A3-A0 is third dtc character.
  output_string[2] = digit_lookup[A & 0b1111];
  // B7-B4 is fourth dtc character
  output_string[3] = digit_lookup[B >> 4];
  // B3-B0 is fifth dtc character
  output_string[4] = digit_lookup[B & 0b1111];
}
