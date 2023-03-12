//
// Title:         iHexLoad - Enabling Agon VDP to load Intel Hex files over the serial interface with VDU control
// Author:        Jeroen Venema
// Created:       05/10/2022
// Last Updated:  08/01/2023

#define DEF_LOAD_ADDRESS 0x040000
#define DEF_U_BYTE  ((DEF_LOAD_ADDRESS >> 16) & 0xFF)


#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <HardwareSerial.h>
#include "ihexload.h"
#include "agon.h"

extern void vdu(uint8_t c);
extern void send_packet(uint8_t code, uint8_t len, uint8_t data[]);
extern void printFmt(const char *format, ...);
extern uint8_t readByte();

void ez80SendByte(uint8_t b)
{
  uint8_t packet[2] = {0,0};

  switch(b)
  {
    case 0: // escape with 0x01/0x02 - meaning 0x00
      packet[0] = 1;
      packet[1] = 1;
      send_packet(PACKET_KEYCODE, sizeof packet, packet);                       
      break;
    case 1: // escape with 0x01/0x01 - meaning 0x01
      packet[0] = 1;
      packet[1] = 0;
      send_packet(PACKET_KEYCODE, sizeof packet, packet);                       
      break;
    default:
      packet[0] = b;
      send_packet(PACKET_KEYCODE, sizeof packet, packet);                           
      break;
  }
}

// Receive a single Nibble fro the incoming Intel Hex data
uint8_t getHxNibble(void)
{
  uint8_t c ,val;

  c = 0;
  while(c == 0)
  {
    if(Serial.available() > 0) c = toupper(Serial.read());
  }
  
  if((c >= '0') && c <='9') val = c - '0';
  else val = c - 'A' + 10;
  // illegal characters will be dealt with by checksum later
  return val;
}

// Receive a byte from the incoming Intel Hex data
// as two combined nibbles
uint8_t getHxByte(void)
{
  uint8_t val = 0;

  val = getHxNibble() << 4;
  val |= getHxNibble();

  return val;  
}

void echo_checksum(uint8_t hxchecksum, uint8_t ez80checksum)
{
  // local echo status to the user
  if(hxchecksum) printFmt("X");
  if(ez80checksum) printFmt("x");
  if((hxchecksum == 0) && (ez80checksum == 0)) printFmt(".");
}

void sendFakeCursorPosition() {
	uint8_t packet[] = {
		1,
		1,
	};
	send_packet(PACKET_CURSOR, sizeof packet, packet);	
}

// Hexload engine
//
void vdu_sys_hexload(void)
{
  uint8_t u,h,l;
  uint8_t count;
  uint8_t record;
  uint8_t data;
  uint8_t hxchecksum,ez80checksum;

  bool done,defaultaddress,ez80checksumerror;
  uint16_t errors;
  
  // The client has previously sent a CR/LF command, setting cursor X to 0
  // It then sends VDU 23,0,2 - send cursor position
  // Regular MOS returns the correct position, but we intercept during the hexload call and reply with X=1,Y=1
  readByte(); // 23
  readByte(); //  0
  readByte(); //  2 -> VDU (23,0,2) send cursor position
  // The regular VDP will send X=0, The patched VDP reply differently, so the client can tell if the VDP is patched
  sendFakeCursorPosition();
  delay(5); // allow the ez80 time to process the interrupt and update the X/Y position variables

  printFmt("Receiving Intel HEX records - VDP:115200 8N1\r\n\r\n");
  u = DEF_U_BYTE;
  errors = 0;
  done = false;
  defaultaddress = true;
  while(!done)
  {
    data = 0;
    // hunt for start of record
    while(data != ':') if(Serial.available() > 0) data = Serial.read();

    count = getHxByte();  // number of bytes in this record
    h = getHxByte();      // middle byte of address
    l = getHxByte();      // lower byte of address 
    record = getHxByte(); // record type

    hxchecksum = count + h + l + record;  // init control checksum
    ez80checksum = 1 + u + h + l + count; // to be transmitted as a potential packet to the ez80

    switch(record)
    {
      case 0: // data record
        if(defaultaddress)
        {
          printFmt("\r\nAddress 0x%02x0000 (default)\r\n", DEF_U_BYTE);
          defaultaddress = false;
        }
        ez80SendByte(1);      // ez80 data-package start indicator
        ez80SendByte(u);      // transmit full address in each package  
        ez80SendByte(h);
        ez80SendByte(l);
        ez80SendByte(count);  // number of bytes to send in this package
        while(count--)
        {
          data = getHxByte();
          ez80SendByte(data);
          hxchecksum += data;   // update hxchecksum
          ez80checksum += data; // update checksum from bytes sent to the ez80
        }
        ez80checksum += readByte(); // get feedback from ez80 - a 2s complement to the sum of all received bytes, total 0 if no errors      
        hxchecksum += getHxByte();  // finalize checksum with actual checksum byte in record, total 0 if no errors
        if(hxchecksum || ez80checksum) errors++;
        echo_checksum(hxchecksum,ez80checksum);
        break;
      case 1: // end of file record
        ez80SendByte(0);       // end transmission
        done = true;
        break;
      case 4: // extended lineair address record, only update U byte for next transmission to the ez80
        defaultaddress = false;
        hxchecksum += getHxByte();   // ignore top byte of 32bit address, only using 24bit
        u = getHxByte();
        hxchecksum += u;
        hxchecksum += getHxByte(); // finalize checksum with actual checksum byte in record, total 0 if no errors
        if(hxchecksum) errors++;
        echo_checksum(hxchecksum,0);    // only echo local checksum errors, no ez80<=>ESP packets in this case
        printFmt("\r\nAddress 0x%02x0000\r\n", u);
        break;
      default:// ignore other (non I32Hex) records
        break;
    }
  }
  if(errors)
    printFmt("\r\n%d error(s)\r\n",errors);
  else
    printFmt("\r\nOK\r\n");
  printFmt("VDP done\r\n");   
}
