OBD9141
======

This is a library to read data from the the ISO 9141-2 (K-line) pin of an [OBD-II][obd2] port found in cars.

There are numerous projects which read data from the diagnostic port and display or record this. However, I found that most of these projects either use an ELM327 chip or the communication code is interwoven with the rest of the program. This makes it hard to extract the communication parts for use in another project.

The goal of the library is to provide a simple class which handles the communication with the port. Additionally a second class has been created which can simulate responses for testing purposes.

Usage
--------
The code has been developed using [Teensy 3][teensy31], the K-line transceiver IC's used were during the development were the [MC33290][mc33290], [SN65HVDA100][SN65HVDA100] and [SN65HVDA195][SN65HVDA195]. All three transceiver IC's worked without problems when the typical application circuit from the datasheet was used. The OBD9141 class itself has been tested on one Kia car build in 2004.

For the Teensy 3.x or LC versions it is recommended to use one of the HardwareSerial ports. For use with Arduino the [AltSoftSerial][altsoftserial] library is used by default. The example `reader_softserial` was tested with an Arduino UNO.

A minimal example of how to use the SN65HVDA195 chip mentioned is given by the following schematic:
![Schematic of circuit using SN65HVDA195](/../master/extras/OBD9141_reader/img/OBD9141_reader_cutout.png?raw=true "Schematic of circuit using SN65HVDA195")

The EN pin can either be connected to a pin on the microcontroller or just pulled high in order to always enable the chip.

In the logic folder are some recordings made with a [Saleae logic analyzer][saleae], these show the state of the K-line pin using either a Bluetooth OBD-II reader or this library, these might be useful when developing your own hardware. All timing parameters can be tweaked from the header file, by tuning these parameters, performance of up to 20 requests per second has been achieved (on the same car, 6 readings per second was the maximum with the Bluetooth dongle).

Three examples are given in the example folder. The `reader` and `simulator` examples are to be used with a hardware serial port. The `reader_softserial` example shows how to use it with the [AltSoftSerial][altsoftserial] library. For more information on how to use the library, refer to to the header files.

Timing
------
Several parameters related to timing are given by the specification, some others are beyond our influence. To understand how a request works, lets consider the case the `0x0D` PID is requested, this represents the vehicle's speed.
![Timing diagram of a request](/../master/extras/timing_diagram/timing_diagram.png?raw=true "Timing diagram of a request")

Requesting the value of a PID consists of two phases, the first is the request phase the second the response.

In the first phase, the bytes necessary for the request are written on the Tx line, according to the specification there should be a 5 millisecond delay between each symbol. The duration of this delay is defined by `INTERSYMBOL_WAIT`. It is possible that the ECU still discerns the bytes correctly when this delay is lowered.

The transceiver IC puts the waveform seen on the Tx line on the K-line. However, a echo of this waveform is also provided as output of the transceiver IC and is seen on the Rx of the serial port. Because this is not part of the response we have to deal with this echo. This is done by using the `readBytes` method to read the same number of bytes as were sent during the request. The timeout used for this read is given by (`REQUEST_ECHO_TIMEOUT_MS` * sent_len + `WAIT_FOR_ECHO_TIMEOUT`) milliseconds, where sent_len is the number of bytes sent in the request. Because the serial port used should contain a hardware buffer, very little time is spent in this `readBytes` call, as all the bytes from the echo should already be available and the function returns when the requested number of bytes has been read.

After the echo has been read, the waiting game begins as we wait for the ECU to answer. According to the specification this is atleast 30 milliseconds, this duration (which is the major part of a request duration) is something that cannot be influenced. The timeout set to read the response is given by (`REQUEST_ANSWER_MS_PER_BYTE` * ret_len +  `WAIT_FOR_REQUEST_ANSWER_TIMEOUT`) milliseconds, ret_len represents the number of bytes expected from the ECU. Because this number is known beforehand the `readBytes` method can be used. According to the specification the ECU should also pause between sending the bytes, but this is not necessarily the case.

The number of bytes to be received for each phase is known beforehand so the `readBytes` method can be used; it ensures that we stop reading immediately after the expected amount has been read. The main impact on the performance is given by the time the ECU takes to send a response and the `INTERSYMBOL_WAIT` between the bytes sent on the bus. There is no delay parameter for the minimum duration between two requests, that is up to the user.

Trouble codes
-------------
The library now support to read diagnostic trouble codes from the ECU (the ones associated to the malfunction indicator light). This was made possible with extensive testing by [Produmann](https://github.com/produmann), under [issue #9](https://github.com/iwanders/OBD9141/issues/9).

When trouble codes are read from the ECU the length of the answer is not known beforehand. To accomodate this a method is implemented that handles a variable length request to the ECU, this can be slower than the fixed length one as it has to timeout after the response is finished.

An example on how to read the diagnostic trouble codes is available, see [`readDTC`](examples/readDTC/readDTC.ino). Roughly it requires requesting the trouble codes, this request method returns the number of trouble codes returned. Each troublecode is encoded in two bytes, these can be retrieved and converted into the human readable trouble code with letter and 4 digits (for example `P0113`), which can then be printed to the serial port.

License
------
MIT License, see LICENSE.md.

Copyright (c) 2015 Ivor Wanders


[obd2]:https://en.wikipedia.org/wiki/On-board_diagnostics
[teensy31]:http://www.pjrc.com/teensy/
[mc33290]:http://www.freescale.com/products/archived/iso9141-k-line-serial-link-interface:MC33290
[SN65HVDA195]:http://www.ti.com/product/sn65hvda195-q1
[SN65HVDA100]:http://www.ti.com/product/sn65hvda100-q1
[saleae]:https://www.saleae.com/
[altsoftserial]:https://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
