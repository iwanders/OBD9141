OBD9141
======

This is a library to read data from the the ISO 9141-2 (K-line) pin of an [OBD-II][obd2] port found in cars.

There are numerous projects which read data from the diagnostic port and display or record this. However, I found that most of these projects either use an ELM327 chip or the communication code is interwoven with the rest of the program. This makes it hard to extract the communication parts for use in another project.

The goal of the library is to provide a simple class which handles the communication with the port. Additionally a second class has been created which can simulate responses for testing purposes.

Usage
--------
The code has been developed using [Teensy 3][teensy31], the K-line transceiver IC's used were during the development were the [MC33290][mc33290], [SN65HVDA100][SN65HVDA100] and [SN65HVDA195][SN65HVDA195]. All three transceiver IC's worked without problems when the typical application circuit from the datasheet was used. The OBD9141 class itself has been tested on one Kia car build in 2004.

In the logic folder are some recordings made with a [Saleae logic analyzer][saleae], these show the state of the K-line pin using either a Bluetooth OBD-II reader or this library, these might be useful when developing your own hardware. All timing parameters can be tweaked from the header file, by tuning these parameters, performance of up to 20 requests per second has been achieved (on the same car, 6 readings per second was the maximum with the Bluetooth dongle).

Two examples are given in the example folder, one for the simulator and one to read data. For information on how to use the library, refer to to the header file and examples.

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