# Installing updated firmware to your chip
1. Find esptools folder
2. The new firmware is contained within this folder
3. Hook up an FTDI directly to ESP8266.
4. Connect a few things

NOTE: Pinout with the bulk of the module pointing down and pins facing towards you
Where pin 4 is VCC and 5 is GND
-----------
- 1 2 3 4 -
- 5 6 7 8 -
-----------

5. Connect Pin 3 to ground
6. Momentarily connect Pin 7 to ground (resestting the chip)
7. Run the flashing tool
 './esptool.py -p <serial port (/dev/ttyUSB0 etc)> write_flash 0x000000 AI-v0.9.5.0\ AT\ Firmware.bin'
