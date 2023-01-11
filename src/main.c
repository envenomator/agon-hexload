/*
 * Title:			AGON Hexload code
 * Author:			Jeroen Venema
 * Created:			22/10/2022
 * Last Updated:	07/01/2023
 * 
 * Modinfo:
 * 22/10/2022:		Initial version MOS patch
 * 23/10/2022:		Receive_bytestream in assembly
 * 26/11/2022:		MOS commandline version
 * 07/01/2023:		New UART1 code
 * 11/01/2023:		Release 0.9
 */

#define MOS_defaultLoadAddress 0x040000		// if no address is given from the transmitted Hex file
#define MOS102_SETVECTOR	   0x000956		// as assembled in MOS 1.02, until set_vector becomes a API call in a later MOS version
#define MOS102_FP_SIZE			     26		// 26 bytes to check from the SET_VECTOR address, kludgy, but works for now
#define DEFAULT_BAUDRATE 		 384000

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "mos-interface.h"
#include "uart.h"
#include "vdp.h"

typedef void * rom_set_vector(unsigned int vector, void(*handler)(void));

extern char mos102_fingerprint; // needs to be extern to C, in assembly  DB with a label, or the ZDS C compiler will put it in ROM space

void hexload_uart1(UINT24 baudrate);
void hexload_vdp(void);
CHAR hxload(void);
void hxload_vdp(void);

int errno; // needed by stdlib

int main(int argc, char * argv[]) {
	UINT24 baudrate = 0;
	
	if(argc == 1)
	{
		printf("Usage: hexload <uart1 [baudrate] | vdp>\r\n");
		return 0;
	}
	else
	{
		if(strcmp("uart1",argv[1]) == 0)
		{
			if(argc == 3) baudrate = atol(argv[2]);
			if(baudrate <= 0) baudrate = DEFAULT_BAUDRATE;
			hexload_uart1(baudrate);
			return 0;
		}

		if(strcmp("vdp",argv[1]) == 0)
		{
			hexload_vdp();
			return 0;
		}			
	}
	return 0;
}

void hexload_vdp(void)
{
	// First we need to test the VDP version in use
	printf("\r");	// set the cursor to X:0, Y unknown, doesn't matter
	// No local output, the VDP will handle this
	// set vdu 23/28 to start HEXLOAD code at VDU
	putch(23);
	putch(28);
	
	// A regular VDP will have the cursor at X:0, the patched version will send X:1
	if(vdp_cursorGetXpos() != 1)
	{
		printf("Incompatible VDP version\r\n");
		return;
	}
	// We can't transmit any text during bytestream reception, so the VDU handles this remotely
	hxload_vdp();				
}

void hexload_uart1(UINT24 baudrate)
{
	CHAR c;
	CHAR *fpptr,*chkptr;
	void *oldvector;
	
	rom_set_vector *set_vector = (rom_set_vector *)MOS102_SETVECTOR;	
	UART 	pUART;

	pUART.baudRate = baudrate;
	pUART.dataBits = 8;
	pUART.stopBits = 1;
	pUART.parity = PAR_NOPARITY;
	
	// Check for MOS 1.02 first
	fpptr = (char *)MOS102_SETVECTOR;
	chkptr = &mos102_fingerprint;
	if(memcmp(fpptr,chkptr,MOS102_FP_SIZE) != 0) // needs exact match
	{
		printf("Incompatible MOS version\r\n");
		return;
	}
	
	oldvector = set_vector(UART1_IVECT, uart1_handler);
	init_UART1();
	open_UART1(&pUART);								// Open the UART 

	printf("Receiving Intel HEX records - UART1:%d 8N1\r\n",baudrate);

	c = hxload();
	
	if(c == 0) printf("OK\r\n");
	else printf("%d error(s)\r\n",c);

	// disable UART1 interrupt, set previous vector
	//set_vector(UART1_IVECT, oldvector);
}

