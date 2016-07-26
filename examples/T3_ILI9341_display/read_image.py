#!/usr/bin/env python3

# The MIT License (MIT)
#
# Copyright (c) 2016 Ivor Wanders
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


"""
    This is a quick script to store the image currently being shown on an ILI9341 screen, it is dumped from the 
    microcontroller with the following code:

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
"""

import serial
import sys
import time
import argparse
import struct

def captureScreen(s, width, height, dumpcommand):
    # total screen size in bytes:
    max_count = width * height * 2

    # flush current serial buffer.
    s.read(s.inWaiting())

    # buffer to fill with screen data.
    buf = b''
    # send the command, it delays 1s after receiving the command.
    s.write(dumpcommand)

    # wait a bit and read any remaining data from the serial port.
    time.sleep(0.5)
    s.read(s.inWaiting())

    # now we _should_ be reading the image.
    while (len(buf) < (max_count)):
        # read the buffer.
        if s.inWaiting():
            num = s.inWaiting()
            buf += s.read(num)
        else:
            time.sleep(0.001)

    return buf

def C565_to_RGB(a):
    b = ((a >> 11) / (2**5)) * 255
    g = (((a >> 5) & 0x3F) / (2**6)) * 255
    r = (((a >> 0) & 0x1F) / (2**5)) * 255
    return r, g, b

def write_image(data, width, height, filename):
    import scipy.misc
    img = scipy.zeros((width, height, 3))
    for y in range(height):
        for x in range(width):
            offset = (x * height + y)
            this_pixel = data[offset * 2:offset * 2+2]
            r, g, b = C565_to_RGB(struct.unpack("<H", this_pixel)[0])
            img[x, y, 0] = r
            img[x, y, 1] = g
            img[x, y, 2] = b
    scipy.misc.toimage(img, cmin=0, cmax=255, channel_axis=2).save(filename)


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Save an image from the ILI93141 display.')
    parser.add_argument('--width', type=int, default=240, help='Width of the screen')
    parser.add_argument('--height', type=int, default=320, help='Height of the screen')
    parser.add_argument('serial_port', help='Serial port to connect to.')
    parser.add_argument('--dumpscreen', help='Write the retrieved output to this file.', default=False)
    parser.add_argument('--image', help='Write the image to this file.', default=False)
    
    args = parser.parse_args()
    cmd = b'd'

    s = serial.Serial(sys.argv[1])
    data = captureScreen(s, width=args.width, height=args.height, dumpcommand=cmd)
    s.close()

    if (args.dumpscreen):
        with open(args.dumpscreen, 'wb') as f:
            f.write(data)

    if (args.image):
        write_image(data, width=args.width, height=args.height, filename=args.image)
    