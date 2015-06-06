# ########################################################################################################
#						#		#					#
#	AUTHOR: TIM HADWEN			# PINOUT	#	THIS IS AN UNCODED PACKET	#
#	DATE: 10th October 2014			#  10 - MOSI	#					#
#	WEBSITE: www.timhadwen.com		#   9 - MISO	#	WILL SEND PACKETS EVERY SECOND	#
#	LANGUAGE: PYTHON 3.3			#  11 - SCLK	#	AND DISPLAY RECEIVED PACKETS	#
#						#   8 - CSN	#	IN HEXIDECIMAL FORMAT		#
#						#  18 - RADIOCE	#					#
#						#   5 - IRQ	#					#
#########################################################################################################

import spidev
import time
import sys
import RPi.GPIO as GPIO

# You will need to install spidev and the RPI.GPIO libs
# to run this python program

# Sends a payload on every channel (Used for debugging)
def spamchan():
    chan = 0x00
    while (1):
        chan += 1
        setrfch(chan)
        if (chan > 0xff):
            chan = 0
        sendPayload([0x12])

# Predefined base address
baseaddr = [0xA7, 0x65, 0x43, 0x21]

# Sets the RF_CH register over SPI
def setrfch(chan):
    GPIO.output(18, 0)
    time.sleep(1)
    print "Setting RF_CH to " + str(chan)
    resp = spi.xfer2([0x20 | 0x05, chan])
    time.sleep(1)
    GPIO.output(18, 1)


# Puts the radio into RX mode
def rxmode():
    print "Rx Mode Set"
    time.sleep(0.01)
    spi.xfer2([0b00100000, 0b00110011])
    GPIO.output(18, 1)


# Puts the radio into TX mode
def txmode():
    print "Tx Mode Set"
    GPIO.output(18, 0)
    time.sleep(0.01)
    spi.xfer2([0b00100000, 0b00110010])


# Prints the payload in hex
def printpayload(payload):
    return ''.join('0x{:02x} '.format(x) for x in payload)


# Sends the payload given in data
def sendPayload(data):
    # print "Data Send: " + printpayload([0xA0, 0x20] + baseaddr + [0x68, 0x34, 0x78, 0x24] + data + [0x02] + baseaddr + [0x68, 0x34, 0x78, 0x24] + data)

    # Send the payload with 0x02 as the preample, student number (8 bytes) base addr(8 bytes) and data (8 bytes)

    # This is currently doubled to allow the data to send (need 32 byte)
    resp = spi.xfer2([0b10100000, 0x00, 0x2B, 0x1D, 0x2B, 0x1D, 0x2B, 0xFF, 0xFF, 0xD4, 0xE2, 0xA5, 0xD4, 0xB8, 0xE2, 0xE2, 0xFF)];

    # Wait some time to allow SPI to catch up
    time.sleep(0.001)

    # Pulse the CE pin to signal data to be sent
    GPIO.output(18, 0)
    GPIO.output(18, 0)
    GPIO.output(18, 1)
    GPIO.output(18, 1)
    GPIO.output(18, 0)

# Returns a 32 byte payload of 0x00
def bytepayload32():
    array = []
    for i in range(0, 34):
        array.append(0x00)
    return array

# Prints status register bytes
# For debugging
def printstatus(resp):
    RX_DR = (resp & 0b01000000) >> 6
    TX_FULL = (resp & 0x01)
    TX_DS = (resp & 0b00100000) >> 5
    MAX_RT = (resp & 0b00010000) >> 4
    RXP = (resp & 0b00001110) >> 1
    print "=== Status Reg ==="
    print "REG: " + bin(resp) + " MAX_RT: " + str(MAX_RT) + " RX_DR: " + str(RX_DR) + " TX_FULL: " + str(TX_FULL) + " TX_DS: " + str(TX_DS) + " RX_P_NO " + bin(RXP)

# Prints config register bytes
# For debugging
def printconfig(resp):
    PRIM_RX = (resp & 0x01)
    PWR_UP = (resp & 0x02) >> 1
    # CRCO = (resp & 0x04) >> 2
    EN_CRC = (resp & 0x08) >> 3
    # MASK_MAX_RT = (resp & 0x10) >> 4
    MASK_TX_DS = (resp & 0x20) >> 5
    MASK_RX_DR = (resp & 0x40) >> 6

    print "=== Config Reg ==="
    print "PRIM_RX: " + str(PRIM_RX) + " PWR: " + str(PWR_UP) + " MASK_RX_DR: " + str(MASK_RX_DR) + " EN_CRC: " + str(EN_CRC) + " MASK_TX_DS: " + str(MASK_TX_DS)
    print ""


# START OF MAIN

# Init GPIO and SPI
GPIO.setwarnings(False)
spi = spidev.SpiDev()
spi.open(0, 0)
GPIO.setmode(GPIO.BCM)

# Init CE Pin at Pin number 18
# Changed from 15 due to serial
GPIO.setup(18, GPIO.OUT)

GPIO.output(18, 0)

# IRQ LINE init
GPIO.setup(5, GPIO.IN)

# Set to rx mode to start with
rxmode()

# Setup EN_RXADDR to enable just 1 data pipe
resp = spi.xfer2([0b00100010, 0b00000001])
print "Set EN_RXADDR."

# Set RX Address width
spi.xfer2([0x11 | 0x20, 32])

# Setup RF_CH
setrfch(48)

# Disable AutoACK
spi.xfer2([0x21, 0x00])

# Set RF_SETUP
spi.xfer2([0x26, 0x06])

# Write config register (Probably not required)
#spi.xfer2([0x20 | 0x00, 0b00110011])

# Write TX_ADDR
#spi.xfer2([0x20 | 0x10, 0x7A, 0x56, 0x34, 0x12, 0x00])
spi.xfer2([0x30, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E])

# Write RX_ADDR
spi.xfer2([0x2A, 0x7A, 0x56, 0x34, 0x12, 0x00])

# Read TX_ADDR
resp = spi.xfer2([0x00 | 0x10, 0x00, 0x00, 0x00, 0x00, 0x00])
print hex(resp[1]) + " " + hex(resp[2]) + " " + hex(resp[3]) + " " + hex(resp[4]) + " " + hex(resp[5])

GPIO.output(18, 1)

while True:
    #print "========================================="

    # Put radio in idle mode to check status regs
    GPIO.output(18, 0)

    # Use this trick to get both status and config regs
    resp = spi.xfer2([0x00, 0x00])

    # Print the config reg
    printconfig(resp[1])

    # Print the status reg
    printstatus(resp[0])

    # Check the channel on the radio (Prints out in decimal)
    #resp = spi.xfer2([0x05, 0x00])

    resp = spi.xfer2([0x00 | 0x0A, 0x7A, 0x56, 0x34, 0x12, 0x00])
    print "RX_ADDR: " + hex(resp[1]) + " " + hex(resp[2]) + " " + hex(resp[3]) + " " + hex(resp[4]) + " " + hex(resp[5])

    # Put radio back into receive mode
    GPIO.output(18, 1)

    # Print the radio channel on the radio
    #print "Channel on Radio: " + str(resp[1])

    if GPIO.input(5) == 0:
        print "Receiving data..."
        # Data is ready to be received so receive it

        # Set CE to 0, moving into IDLE state
        GPIO.output(18, 0)

        # Set RX_DR flag to 1
        spi.xfer2([0x20 | 0x07, 0b01001110])

        # Read data
        payload = spi.xfer2([0b01100001] + bytepayload32())

        # Set CE to 1, to continue receiving data if required
        GPIO.output(18, 1)

        # Display data
        print printpayload(payload)
    time.sleep(1)