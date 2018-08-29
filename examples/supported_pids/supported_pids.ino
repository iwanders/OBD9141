#include "Arduino.h"
#include "OBD9141.h"

/**
 * This example polls the 0x00 PID in mode 01, this retrieves a bitmask
 * indicating which PID's are supported. The example attempts to produce
 * a list of all supported PID's for the ECU it's communicating with.
 *
 * Note: Only tested on the first block (0x00-0x20), the car used to test this
 * did not support any PIDs from the second block.
 */

// OBD connections, you likely have to modify these.
// Tested on Teensy, pins are alternates.
#define RX_PIN 21
#define TX_PIN 5
#define EN_PIN 17
#define OBD_SERIAL Serial1

OBD9141 obd;


bool printSupportedPID(uint32_t value, uint8_t offset)
{
  const uint8_t* v = reinterpret_cast<const uint8_t*>(&value);
  bool is_more = false;
  for (uint8_t i = 0; i < 4; i++)
  {
    const uint8_t b = v[i];
    // Serial.print("Now handling b: "); Serial.println(b, HEX);
    for (int8_t j = 7; j >= 0; j--)
    {
      uint8_t PID = offset + (i + 1) * 8 - j;
      bool supported = b & (1 << j);
      Serial.print("0x"); Serial.print(PID, HEX); Serial.print(": "); Serial.println(supported);
      is_more = supported;  // last read bit is always whether the next block is supported.
    }
  }
  return is_more;
}

void setup()
{
  Serial.begin(9600);
  delay(2000);

  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  OBD_SERIAL.setTX(TX_PIN);
  OBD_SERIAL.setRX(RX_PIN);
  obd.begin(OBD_SERIAL, RX_PIN, TX_PIN);

  // Test from https://en.wikipedia.org/wiki/OBD-II_PIDs#Service_01_PID_00
  // Serial.println("Test:");
  // const uint8_t resp[] = {0xBE, 0x1F, 0xA8, 0x13};
  // printSupportedPID(*reinterpret_cast<const uint32_t*>(resp), 0);
  // Serial.println("Test done, now going into actual retrieval.");
}
void loop()
{
  Serial.println("Looping");

  bool init_success =  obd.init();
  Serial.print("init_success:");
  Serial.println(init_success);

  //init_success = true;
  // Uncomment this line if you use the simulator to force the init to be
  // interpreted as successful. With an actual ECU; be sure that the init is 
  // succesful before trying to request PID's.

  uint8_t offset = 0;
  if (init_success)
  {
    bool is_more = true;
    while(is_more)
    {
      bool res = obd.getCurrentPID(offset, 4);
      if (res)
      {
        Serial.print("PID 0x");
        Serial.print(offset, HEX);
        uint32_t sup_block = obd.readUint32();
        Serial.print("  -> 0x");
        Serial.println(sup_block, HEX);
        is_more = printSupportedPID(sup_block, offset);
        if (is_more)
        {
          offset += 0x20;
        }
        else
        {
          offset = 0;
        }
      }
      delay(200);
    }
  }
}

