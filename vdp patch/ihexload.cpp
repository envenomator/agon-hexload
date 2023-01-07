//
// Title:         iHexLoad
// Author:        Jeroen Venema
// Contributors:  Kimmo Kulovesi (kk_ihex library https://github.com/arkku/ihex)
// Created:       05/10/2022
// Last Updated:  08/10/2022
//
// Modinfo:
//
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

void send_byte(uint8_t b)
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

//
// CRC32 functions
//
uint32_t crc32(const char *s, uint32_t n, bool firstrun)
{
  static uint32_t crc;
  static uint32_t crc32_table[256];
   
  if(firstrun)
  {
    for(uint32_t i = 0; i < 256; i++)
    {
      uint32_t ch = i;
      crc = 0;
      for(uint32_t j = 0; j < 8; j++)
      {
        uint32_t b=(ch^crc)&1;
        crc>>=1;
        if(b) crc=crc^0xEDB88320;
        ch>>=1;
      }
      crc32_table[i] = crc;
    }        
    crc = 0xFFFFFFFF;
  }
  
  for(uint32_t i=0;i<n;i++)
  {
    char ch=s[i];
    uint32_t t=(ch^crc)&0xFF;
    crc=(crc>>8)^crc32_table[t];
  }

  return ~crc;
}

uint8_t get_serialnibble(void)
{
  uint8_t c,val = 0;

  c = toupper(Serial.read());
  
  if((c >= '0') && c <='9') val = c - '0';
  else val = c - 'A' + 10;
  // illegal characters will be dealt with by checksum later
  return val;
}

uint8_t get_serialbyte(void)
{
  uint8_t val = 0;

  val = get_serialnibble() << 4;
  val |= get_serialnibble();
}

void send_address(uint8_t u, uint8_t h, uint8_t l)
{
  send_byte(u);
  send_byte(h);
  send_byte(l);
}

// NEW Ihex engine
//
void vdu_sys_hexload(void)
{
  uint8_t u,h,l;
  uint8_t count;
  uint8_t record;
  uint8_t checksum;
  uint8_t data[256];
  uint8_t *dataptr;
  uint8_t c = 0;
  bool done = false;

  uint32_t crc_sent,crc_rec;
  crc32(0,0,true); // init crc table

  if(Serial.available())
  {
    while(!done)
    {
      do // hunt for start of record
      {
        c = Serial.read();
      } while (c != ':');
      count = get_serialbyte(); // number of bytes in this record
      h = get_serialbyte();     // middle byte of address
      l = get_serialbyte();     // lower byte of address 
      record = get_serialbyte();// record type

      checksum = 0;             // init checksum
      switch(record)
      {
        case 0: // data record
          send_address(u,h,l);
          send_byte(count);
          dataptr = data;
          while(count--)
          {
            *dataptr = get_serialbyte();
            send_byte(*dataptr);
            checksum += *dataptr;   // update checksum
            dataptr++;
          }
          checksum += get_serialbyte(); // finalize checksum with actual checksum byte in record
          break;
        case 1: // end of file record
          send_address(0,0,0);
          send_byte(0);
          done = true;
          break;
        case 4: // extended lineair address record
          checksum += get_serialbyte();   // ignore top byte of 32bit address, only using 24bit
          u = get_serialbyte();
          checksum += u;
          send_address(u,0,0);
          checksum += get_serialbyte(); // finalize checksum with actual checksum byte in record
          break;
        default:// ignore all else for now
          break;
      }
      if(checksum) printFmt("X");
      else printFmt(".");     
    }
  }  
}