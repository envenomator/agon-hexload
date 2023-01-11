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
#define MOS102_FP_SIZE			     26		// 26 bytes to check from the SET_VECTOR address, kludgy, but works for now

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "mos-interface.h"
#include "uart.h"
#include "vdp.h"

typedef void * rom_set_vector(unsigned int vector, void(*handler)(void));

//extern char[26] mos102_fingerprint;

extern char mos102_fingerprint;

void hexload_uart1(void);
void hexload_vdp(void);
CHAR hxload(void);
void hxload_vdp(void);

int main(int argc, char * argv[]) {

	UINT8 x,y;
	
	//if((argc == 2) && (strcmp(argv[1],"vdp") == 0)) hexload_vdp();
	hexload_uart1();
	//hexload_vdp();
	//printf("\r\n");

	//x = vdp_cursorGetXpos();
	//y = vdp_cursorGetYpos();
	//printf("x = %d, y = %d\r\n",x,y);
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

void hexload_uart1(void)
{
	CHAR c;
	CHAR *fpptr,*chkptr;
	void *oldvector;
	
	rom_set_vector *set_vector = (rom_set_vector *)MOS102_SETVECTOR;	
	UART 	pUART;

	pUART.baudRate = 384000;
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

	printf("UART1 receiving Intel HEX records\r\n");

	c = hxload();
	
	if(c == 0) printf("OK\r\n");
	else printf("%d error(s)\r\n",c);

	// disable UART1 interrupt, set previous vector
	set_vector(UART1_IVECT, oldvector);
}

