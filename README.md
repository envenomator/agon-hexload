# HEXLOAD utility
## Purpose
Playing microSD jockey during assemble/compile/transfer/test/debug cycles is no fun at all. I'd like to shorten this cycle significantly by removing the need to physically bring over new binaries to the Agon.

With the hexload command you are able to transmit Intel I32 hex files to one of the Agon serial ports and run your code immediately from memory.

## Installation
Copy 'hexload.bin' and 'hexload.dll' to the \mos\ directory on the microSD card. If no such directory exists, create it first. This allows the usage of arguments to the utility and loading it using it's base name.

The utility requires at least MOS version 1.02. Transfering data over the ESP USB port requires at least MOS version 1.04.

## Usage

    hexload <uart1 [baudrate] | vdp> [filename]

This utility provides two options for transfering data:
1. The UART1 serial port can be used at the external GPIO serial pins PC0/TxD1, PC1/RxD1 and GND. Connect to external serial interfaces (3.3v), like for example a USB-Serial FTDI adapter
![Multiple transfers using different baudrates and addresses](https://github.com/envenomator/agon-hexload/blob/master/media/uarttransfer.png?raw=true)

2. The ESP USB port that powers the Agon can be used, using the serial-over-USB interface it provides, without requiring a separate interface. This requires patching the VDP source and flashing it to the ESP32.
![A single transfer over the VDP serial](https://github.com/envenomator/agon-hexload/blob/master/media/vdptransfer.png?raw=true)

### Baudrate
The uart1 option has a selectable baudrate. If no baudrate is given, the default of 384000 is selected. The vdp option uses 115200 by default. The actual baudrate used is echoed to the user at startup.

**Please be aware** that **achievable** baudrates in **your** setup may be much lower than this. Some users report baud rates well below 115200. The **achievable** baudrate without errors during transmission in your setup, will depend on factors like quality of your FTDI adapter, which IC is on it, which driver is loaded into the operating system for it to function, your operating system itself, but also which USB interface you are plugging it into.

Upon first use, it appears prudent to set an initially LOW baudrate value, potentially as low as 57600, or even 9600, to verify transmission. And only after this initial verification ramp up the speed an next attempts to see where you end up with your specific setup.

### Dump to file
The transfered data is dumped to a file when a filename is appended as last option. 
### Difference in feedback during transmission
During reception of Intel Hex files on the UART1, no feedback is given to the user before transmission is terminated by a 01 record. There is not enough CPU available to handle output while also processing the input at high speed.
Using the VDP does allow for feedback to the user during transmission. The VDP informs the user of every address record sent.

### Sending files
Intel Hex files must be sent in textformat, over one of the serial interfaces. I have provided an example send.py python script to automate this process, converting binary files to hex files on the fly before transfer. Edit your serial port and speed in the script as needed and provide the Intel Hex file as argument.

    send.py filename <port> <baudrate>

In some cases using the VDP, the first time the ESP serial is used after a reboot, it can trigger the boot-mode from the ESP. Just press reset and try again, or set the ESP boot jumper to disabled.

### UART1 pinout
- Tx: GPIO pin PC0 - connect to your adapter serial Rx
- Rx: GPIO pin PC1 - connect to your adapter serial Tx

## Start address
The hexload utility will load the Intel Hex file to memory, according to the specified 'Extended Lineair Address' records in the file itself. If you want to load a program to a specific address, you'll need make sure that whatever creates your Intel Hex file sets the addresses appropriately for the Agon platform.
The hexload utility doesn't check for invalid address ranges like the flash, or MOS ranges.

When no address records are found in the Intel Hex file; the default load address 0x040000 will be used.