# HEXLOAD utility
## Purpose
Playing microSD jockey during assemble/compile/transfer/test/debug cycles is no fun at all. I'd like to shorten this cycle significantly by removing the need to physically bring over new binaries to the Agon.

With the hexload command you are able to transmit Intel I32 hex files to one of the Agon serial ports and run your code immediately from memory.

## Options
This utility provides two options:
1. The UART1 serial port can be used at the external GPIO serial pins PC0/TxD1, PC1/RxD1 and GND. Connect to external serial interfaces (3.3v), like for example a USB-Serial FTDI adapter
![Multiple transfers using different baudrates and addresses](https://github.com/envenomator/agon-hexload/blob/master/uarttransfer.png?raw=true)

2. The ESP USB port that powers the Agon can be used, using the serial-over-USB interface it provides, without requiring a separate interface. This requires patching the VDP source and flashing it to the ESP32.
![A single transfer over the VDP serial](https://github.com/envenomator/agon-hexload/blob/master/vdptransfer.png?raw=true)

## Installation
Copy 'hexload.bin' to the \mos\ directory on the microSD card. If no such directory exists, create it first. This allows the usage of arguments to the utility and loading it using it's base name.

The utility requires at least MOS version 1.02

## Usage
### Commandline
    hexload <uart1 [baudrate] | vdp>

### Baudrates
The uart1 option has a selectable baudrate like 115200. If no baudrate is given, the default of 384000 is selected. The vdp option uses 115200 by default. The actual baudrate used is echoed to the user at startup.

### Feedback differences during transmission
During reception of Intel Hex files on the UART1, no feedback is given to the user before transmission is terminated by a 01 record. Using the VDP allows for feedback to the user during transmission. The VDP informs the user of every address record sent.

### Sending files
Intel Hex files can be sent in different ways, in textformat, over one of the serial interfaces. I have provided an example send.py python script to automate this process; set your serial port and speed as needed and provide the Intel Hex file as argument.

In some cases using the VDP, the first time the ESP serial is used after a reboot, it can trigger the boot-mode from the ESP. Just press reset and try again, or set the ESP boot jumper to disabled.

## Start address
The hexload utility will load the Intel Hex file to memory, according to the specified 'Extended Lineair Address' records in the file itself. If you want to load a program to a specific address, you'll need make sure that whatever creates your Intel Hex file sets the addresses appropriately for the Agon platform.
The hexload utility doesn't check for invalid address ranges like the flash, or MOS ranges.

When no address records are found in the Intel Hex file; the default load address 0x040000 will be used.

## VDP Patch details
### Patchfiles and binaries
A supplied unix script clones Quark VPD from the Git repo, copies over the new sourcefiles and includes the necessary #include statements in the appropriate locations.

For people using this from Windows, consider installing the Windows Subsystem for Linux (WSL) and use the script from there.

Binaries from a compiled patch, including a (Windows)batchfile are provided in the repository, to flash the patch immediately to the Agon. You need to edit the batchfile with the correct COM port setting before running it. Binaries from the original VDP 1.02 are also included for quick a recovery.

### The sequence of VDP iHex file transfer:
0. The first time after a cold reboot, an extra push to the reset button is needed. Otherwise the ESP32 is set to firmware update mode during opening of the serial interface. 
1. Enter command 'hexload' in MOS, which requests a serial-receive function in VDP. The VDP then accepts Intel HEX files, sent as text to the serial-over-USB interface, decodes the content, marks line CRC faults and sends a contiguous bytestream back to the ez80 CPU in chunks of max 255 bytes. The VDP gives an overview of each line received over the serial port and the status
2. Send the actual file in text format to the serial-over-USB interface. You can use the example send.py script for this. You need to edit this to match the serial port you are using
3. Both VDP and CPU calculate a checksum over the sent/received datastream. Success/failure is reported by the VDP.
4. On success, you might expect to be able to JMP or RUN to the appropriate memory location. Default is 0x40000, unless your iHex file specifies a different start address. 

### Changes
- The MOS requests transfer using VDU 23,28 currently. I might need to move this in the future, but for now, this sequence seems unused
- Because of serial speed differences and lack of flowcontrol on both serial interfaces (inbound serial-usb and VDP->CPU), the VDP having more power to decode the iHex stream, the z80 CPU just receives the inbound stream in chunks. At the end of chunk transmissions, both sides calculate checksum and the CPU reports the value to the VDP which then matches the results
- In video.ino, the DBGSerial is instanciated to (0), but that creates problems referencing this externally, whereas the default Arduino Serial is already mapped. So DBGSerial is renamed to Serial. The speed is reduced from 500000 to 115200; the MOS currently has no inbound hardware flowcontrol and is overrun at higher speeds than this. In order to achieve this speed at all, some code was written in z80 assembly.
- To avoid a complete overhaul and additional code, just to transfer bytes differently than in the current serial packet protocol, the PACKET_KEYCODE is used for transfer of all byte values. In order to transfer '0' as a value and still be able to differentiate a receipt at the CPU, the keyboard modifier byte is used as an escape sequence.