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

#include <stdio.h>
#include <ctype.h>
#include "mos-interface.h"
#include "uart.h"
#include <string.h>

typedef void * rom_set_vector(unsigned int vector, void(*handler)(void));

CHAR hxload(void);

int main(int argc, char * argv[]) {
	CHAR c;
	void *oldvector;
	
	rom_set_vector *set_vector = (rom_set_vector *)MOS102_SETVECTOR;	
	UART 	pUART;

	pUART.baudRate = 384000;
	pUART.dataBits = 8;
	pUART.stopBits = 1;
	pUART.parity = PAR_NOPARITY;

	oldvector = set_vector(UART1_IVECT, uart1_handler);
	init_UART1();
	open_UART1(&pUART);								// Open the UART 

	printf("Receiving Intel hex file\r\n");
	c = hxload();
	if(c == 0) printf("OK\r\n");
	else printf("%d error(s)\r\n",c);

	// disable UART1 interrupt, set previous vector
	set_vector(UART1_IVECT, oldvector);
	
	return 0;
}

