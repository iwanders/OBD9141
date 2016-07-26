/*
  The MIT License (MIT)
  Copyright (c) 2016 Ivor Wanders
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/*
  This is a more elaborate example; it uses an ILI9341 (320x240) screen to display various PID values, polling these
  at independent intervals.

  The optimised library for Teensy 3 is used: https://github.com/PaulStoffregen/ILI9341_t3/
*/

#include "Arduino.h"
#include "SPI.h"
#include "ILI9341_t3.h"
#include "OBD9141.h"

// load the images for the fuel system control loop state.
#include "img_OL_temperature.c"
#include "img_CL_O2.c"
#include "img_OL_decel.c"
#include "img_OL_fail.c"
#include "img_CL_fault.c"

// Connections between the screen and the Teensy 3.2
#define TFT_DC      20
#define TFT_CS      15
#define TFT_RST    255  // 255 = unused, connect to 3.3V
#define TFT_MOSI    11
#define TFT_SCLK    14
#define TFT_MISO    12
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

// OBD connections.
#define RX_PIN 0
#define TX_PIN 1
#define EN_PIN 2
OBD9141 obd;

// Use random values to display.
#define DEBUG 1

// typedefs for the OBD entries.
typedef struct{
  uint8_t pollid;
  uint8_t length;
  uint8_t type; // 1 = uint8, 2 = uint16
  uint16_t interval; // in ms.
  uint16_t xpos;
  uint16_t ypos;
  uint8_t fontsize;
  uint8_t baroffset;
  uint16_t barminvalue;
  uint16_t barmaxvalue;
} obd_unit;

typedef struct{
  uint32_t old_value;
  elapsedMillis timesince;
} obd_unit_state;

// state.
uint8_t obd_entry_count = 0;
obd_unit entries[8] = {};
obd_unit_state entry_states[8];

// function to add an OBD PID to the list of PID's that will be polled.
void add_entry(uint8_t pollid, uint8_t length, uint8_t type, uint16_t interval, uint16_t xpos,uint16_t  ypos,uint8_t  fontsize,uint8_t  baroffset,uint16_t  barminvalue,uint16_t  barmaxvalue){
  entries[obd_entry_count].pollid = pollid;
  entries[obd_entry_count].length = length;
  entries[obd_entry_count].type = type;
  entries[obd_entry_count].interval = interval;
  entries[obd_entry_count].xpos = xpos;
  entries[obd_entry_count].ypos = ypos;
  entries[obd_entry_count].fontsize = fontsize;
  entries[obd_entry_count].baroffset = baroffset;
  entries[obd_entry_count].barminvalue = barminvalue;
  entries[obd_entry_count].barmaxvalue = barmaxvalue;

  entry_states[obd_entry_count].old_value = 0;
  entry_states[obd_entry_count].timesince = 0;
  obd_entry_count++;
  
}



void setup() {
  Serial.begin(9600);
  SPI.setSCK(TFT_SCLK);
 
  tft.begin();
  tft.setRotation(3); // Horizontal screen, header to the left.
  tft.fillScreen(ILI9341_BLACK); // black background.

  // OBD initialisation
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);
  obd.begin(Serial1, RX_PIN, TX_PIN);

  // add_entry(uint8_t pollid, uint8_t length, uint8_t type, uint16_t interval, uint16_t xpos,uint16_t  ypos,uint8_t  fontsize,uint8_t  baroffset, min, max);
  // // 1 = uint8, 2 = uint16
  uint16_t x = 0;
  uint16_t y = 0;

  // if bar max is 0, no bar is displayed.
  add_entry(0x0C, 2, 3,     50, x, y, 7, 50, 0, 5000); y += 75;// 0x0C, 2 long, RPM. (uint16/4, so type 3)

  add_entry(0x0D, 1, 1,     200, x, y, 5, 36, 0, 150); y += 50;// 0x0C, 1 long, speed. (uint8, so type 1)
  
  add_entry(0x04, 1, 5,     100, x, y, 3, 24, 0, 100);y += 30;// 0x04, 1 long, engine load %
  // add_entry(0x11, 1, 5,     50, x+tft.width()/2, y, 3, 24, tft.width() / 2 + 60, 0);  y += 30; // 0x11, 1 long, throttle position

  y+= 20;

  // if barmax = 0; barmin is label x pos and no bar is shown.
  add_entry(0x05, 1, 4,     1000, x, y, 3, 24, 60, 0);  // 0x05, 1 long, coolant temp. type 4 = uint8 - 40
  add_entry(0x0F, 1, 4,     1000, x+tft.width()/2, y, 3, 24, tft.width() / 2 + 60, 0);  y += 30; // 0x05, 1 long, coolant temp. type 4 = uint8 - 40

  add_entry(0x03, 2, 1, 250, tft.width() - img_CL_O2.width-2, tft.height() - img_CL_O2.height-4, 0, 0, 0, 0); // fuel system status


}

void loop(){
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(0, 100);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.println("Trying to connect....");

  // Test the images.
  // tft.writeRect(0, 0, img_OL_temperature.width, img_OL_temperature.height, (uint16_t*)(img_OL_temperature.pixel_data));
  // tft.writeRect(0, img_OL_temperature.height*1, img_CL_O2.width, img_CL_O2.height, (uint16_t*)(img_CL_O2.pixel_data));
  // tft.writeRect(0, img_OL_decel.height*2, img_OL_decel.width, img_OL_decel.height, (uint16_t*)(img_OL_decel.pixel_data));
  // tft.writeRect(0, img_OL_FAIL.height*3, img_OL_FAIL.width, img_OL_FAIL.height, (uint16_t*)(img_OL_FAIL.pixel_data));
  // tft.writeRect(0, img_CL_FAULT.height*4, img_OL_FAIL.width, img_OL_FAIL.height, (uint16_t*)(img_CL_FAULT.pixel_data));
  // delay(10000);

  Serial.println("Trying to connect");
  bool got_success =  obd.init();
  if (!got_success){
    #ifndef DEBUG
    return;
    #endif
    delay(3000);
  }
  #ifdef DEBUG
    got_success = true;
  #endif
  tft.fillScreen(ILI9341_BLACK);

  // Draw the text which shows which number is which for each entry.
  for (size_t i=0; i < obd_entry_count; i++){
    tft.setTextSize(2);
    if ((entries[i].barmaxvalue == 0)){
      tft.setCursor(entries[i].barminvalue, entries[i].ypos);
    } else {
      tft.setCursor(tft.width() / 2, entries[i].ypos);
    }
    tft.setTextColor(ILI9341_YELLOW);


    if (entries[i].fontsize == 0){
      continue; // do not display any text.
    }
    
    switch(entries[i].pollid){
      case 0x0C:
        tft.print(" rpm");
        break;
      case 0x0D:
        tft.print(" km/hr");
        break;
      case 0x04:
        tft.print(" % (load)");
        break;
      case 0x05:
        tft.print(" deg");
        break;
      case 0x11:
        tft.print(" throttle");
        break;
      case 0x0F:
        tft.print(" deg air");
        break;
      default:
        // we do not know this PID, just show the HEX value for this.
        tft.print("[0x");
        tft.print(entries[i].pollid, HEX);
        tft.print("]");
        break;
        
    }
  }

  while (got_success){ // if we fail getting a result, we drop back to the start of this function.
    Serial.println("Connected, looping");
    for (size_t i=0; i < obd_entry_count; i++){

      // check if we have to do this entry at this time.
      if (entry_states[i].timesince > entries[i].interval){
        Serial.println("Should display");
        entry_states[i].timesince = 0;

        // retrieve the value.
        got_success = obd.getCurrentPID(entries[i].pollid, entries[i].length);
        #ifdef DEBUG
          // if debug, force succes
          got_success = true;
        #endif
        if (!got_success){
          break; // break the loop.
        }

        // retrieve the right value from the internal buffer of the OBD9141 object.
        uint32_t value;
        switch (entries[i].type){
          case 1:
            value = obd.readUint8();
            break;
          case 2:
            value = obd.readUint16();
            break;
          case 3:
            value = obd.readUint16()/4;
            break;
          case 4:
            value = obd.readUint8() - 40;
            break;
          case 5:
            value = (obd.readUint8()*2)/5;
            break;
        }

        // if we are running in DEBUG mode, fake results in the right range.
        #ifdef DEBUG
          switch (entries[i].pollid){
            case 0x0C:
              value = ((millis() * 3) % 4000) + ((millis()>>4) % 10) + 1000;
              break;
            case 0x0D:
              value = (((millis() * 3) % 1300) + ((millis()>>4) % 10)) / 10;
              break;
            case 0x04:
              value = (((millis() * 3) % 1000) + ((millis()>>4) % 10)) / 10;
              break;
            case 0x05:
              value = ((((millis() * 3) % 500) + ((millis()>>4) % 10)) / 10) + 50;
              break;
            case 0x0F:
              value = (((millis() * 3) % 300) + ((millis()>>4) % 10)) / 10;
              break;
            case 0x03:
              value = 1 << ((millis() % 200) % (5));
              break;
          }
        #endif

        // the previous value was the same as this value, lets not redraw it to prevent flashing numbers.
        if (entry_states[i].old_value == value){
            continue;
        }

        // fuel status state, needs special handling as we have pretty pictures for this.
        if (entries[i].pollid == 0x03){
          /*
          Mode 1 PID 03[edit]
            A request for this PID returns 2 bytes of data. The first byte describes fuel system #1.

            Value	Description
            1	Open loop due to insufficient engine temperature
            2	Closed loop, using oxygen sensor feedback to determine fuel mix
            4	Open loop due to engine load OR fuel cut due to deceleration
            8	Open loop due to system failure
            16	Closed loop, using at least one oxygen sensor but there is a fault in the feedback system
          */
          switch(value & 0b11111){
            case (1<<0):
              tft.writeRect(entries[i].xpos, entries[i].ypos, img_OL_temperature.width, img_OL_temperature.height, (uint16_t*)(img_OL_temperature.pixel_data));
              break;
            case (1<<1):
              tft.writeRect(entries[i].xpos, entries[i].ypos, img_CL_O2.width, img_CL_O2.height, (uint16_t*)(img_CL_O2.pixel_data));
              break;
            case (1<<2):
              tft.writeRect(entries[i].xpos, entries[i].ypos, img_OL_decel.width, img_OL_decel.height, (uint16_t*)(img_OL_decel.pixel_data));
              break;
            case (1<<3):
              tft.writeRect(entries[i].xpos, entries[i].ypos, img_OL_fail.width, img_OL_fail.height, (uint16_t*)(img_OL_fail.pixel_data));
              break;
            case (1<<4):
              tft.writeRect(entries[i].xpos, entries[i].ypos, img_CL_fault.width, img_CL_fault.height, (uint16_t*)(img_CL_fault.pixel_data));
              break;
          }
          continue; // do not set the text in this case.
        }


        // if we have to draw a bar below the text, clear the old one and draw a new one.
        if ((entries[i].barmaxvalue != 0)){
          //tft.width(), tft.height()
          uint16_t width = map(value, entries[i].barminvalue, entries[i].barmaxvalue, 0, tft.width());
          for(uint16_t y=0; y<5; y+=1) tft.drawFastHLine(0, entries[i].ypos + entries[i].baroffset+y, tft.width(), ILI9341_BLACK);
          for(uint16_t y=0; y<5; y+=1) tft.drawFastHLine(0, entries[i].ypos + entries[i].baroffset+y, width, ILI9341_GREEN);
        }

        // finally, we get to drawing the text.

        // Clear the old text by drawing the old text with black.
        tft.setCursor(entries[i].xpos, entries[i].ypos);
        tft.setTextColor(ILI9341_BLACK);
        tft.setTextSize(entries[i].fontsize);
        tft.print(entry_states[i].old_value);

        // Write the new text with white.
        tft.setCursor(entries[i].xpos, entries[i].ypos);
        tft.setTextColor(ILI9341_WHITE);
        tft.setTextSize(entries[i].fontsize);
        tft.print(value);

        // store the new old_value.
        entry_states[i].old_value = value;

      } // end of PID this interval.
    } // end of loop over PID's to query.
    
    #ifdef DEBUG
      // this is only used to obtain a dump of the image on the screen.
      if (Serial.available()){
        uint8_t instruction = Serial.read();
        if (instruction == 'd'){
          // Dump instruction...
          //void readRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t *pcolors);
          delay(1000);
          uint16_t block_y_count = tft.height();
          uint16_t block_w = tft.width();
          uint16_t block_h = tft.height() / block_y_count;
          for (uint16_t yblock=0; yblock < block_y_count; yblock++){
            uint16_t buf[block_w * block_h];
            tft.readRect(0, yblock * block_h, block_w, block_h, buf);
            Serial.write((uint8_t*) &buf, block_w * block_h * 2);
          }
        }
      }
    #endif
  }
  delay(1000);
}