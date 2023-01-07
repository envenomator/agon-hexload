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
 * 07/01/2023:		Removed VDP patch bytestream option, shift to UART1 code
 */

#define MOS_defaultLoadAddress 0x040000		// if no address is given from the transmitted Hex file
#define MOS102_SETVECTOR	   0x000956		// as assembled in MOS 1.02, until set_vector becomes a API call in a later MOS version
#define MOS102_SIZE				  46846
#define MOS102_CRC			 0xfe59e98d

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "mos-interface.h"
#include "uart.h"
#include "crc32.h"

typedef void * rom_set_vector(unsigned int vector, void(*handler)(void));

void hexload_uart1(void);
void hexload_vdp(void);
CHAR hxload(void);
void hxload_vdp(void);

int main(int argc, char * argv[]) {
	
	//if((argc == 2) && (strcmp(argv[1],"vdp") == 0)) hexload_vdp();
	//else hexload_uart1();
	hexload_vdp();
	
	return 0;
}

void hexload_vdp(void)
{
	printf("Receiving Intel HEX records - VDP\r\n");

	// set vdu 23/28 to start HEXLOAD code at VDU
	putch(23);
	putch(28);
	// We can't transmit any text during bytestream reception, so the VDU handles this
	hxload_vdp();
}

void hexload_uart1(void)
{
	INT32 crcresult;
	CHAR c;
	void *oldvector;
	
	rom_set_vector *set_vector = (rom_set_vector *)MOS102_SETVECTOR;	
	UART 	pUART;

	pUART.baudRate = 384000;
	pUART.dataBits = 8;
	pUART.stopBits = 1;
	pUART.parity = PAR_NOPARITY;
	
	// Check for MOS 1.02 first
	printf("Checking MOS version 1.02...\r\n");
	if(crc32((char*)0, MOS102_SIZE) != MOS102_CRC)
	{
		printf("Incompatible version\r\n");
		return;
	}

	oldvector = set_vector(UART1_IVECT, uart1_handler);
	init_UART1();
	open_UART1(&pUART);								// Open the UART 

	printf("Receiving Intel HEX records - UART1\r\n");

	c = hxload();
	
	if(c == 0) printf("OK\r\n");
	else printf("%d error(s)\r\n",c);

	// disable UART1 interrupt, set previous vector
	set_vector(UART1_IVECT, oldvector);
}

