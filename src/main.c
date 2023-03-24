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
 * 23/02/2023:		Bugfix - DEFB used, should have been DS at end of hxload.asm
 *                  Option to save as file to SD card
 */

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

extern char mos102_setvector_fingerprint; // needs to be extern to C, in assembly DB with a label, or the ZDS C compiler will put it in ROM space

// external assembly routines
extern CHAR		hxload_uart1(void);
extern VOID		hxload_vdp(void);
extern UINT8	mos_save(char *filename, UINT24 address, UINT24 nbytes);
extern UINT8	mos_del(char *filename);
// external variables
extern volatile UINT24 startaddress;
extern volatile UINT24 endaddress;

int errno; // needed by stdlib

UINT16 tester(void) {
	return 16;
}

void write_file(char *filename) {
	if(filename) {
		printf("Writing to \"%s\"...\r\n", filename);
		mos_del(filename);
		if(mos_save(filename, startaddress, (endaddress-startaddress)) == 0)
			printf("%d bytes\r\n", (endaddress-startaddress));
		else printf("File error\r\n");
	}
}

void handle_hexload_vdp(void)
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


void handle_hexload_uart1(UINT24 baudrate)
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
	chkptr = &mos102_setvector_fingerprint;
	if(memcmp(fpptr,chkptr,MOS102_FP_SIZE) != 0) // needs exact fingerprint match
	{
		printf("Incompatible MOS version\r\n");
		return;
	}
	
	oldvector = set_vector(UART1_IVECT, uart1_handler);	// register interrupt handler for UART1
	init_UART1();										// set the Rx/Tx port pins
	open_UART1(&pUART);									// Open the UART, set interrupt 

	// Only feedback during transfer - we have no time to output to VDP or even UART1 between received bytes
	printf("Receiving Intel HEX records - UART1:%d 8N1\r\n",baudrate);
	c = hxload_uart1();
	
	if(c == 0) printf("OK\r\n");
	else printf("%d error(s)\r\n",c);

	// close UART1, so no more interrupts and default port pins Rx/Tx
	close_UART1();
	// disable UART1 interrupt, set previous vector (__default_mi_handler in MOS ROM, might change on every revision)
	set_vector(UART1_IVECT, oldvector);
}

int main(int argc, char * argv[]) {
	UINT24 baudrate = 0;
	char *end;
	char *filename = NULL;
	
	if(argc == 1)
	{
		printf("Usage: hexload <uart1 [baudrate]| vdp> [filename]\r\n");
		return 0;
	}

	if(strcmp("uart1",argv[1]) == 0)
	{
		if(argc >= 3) {
			baudrate = strtol(argv[2], &end, 10);
			if(*end) { 
				baudrate = 0;
				filename = argv[2];
			}
			if(argc == 4) {
				if(filename) baudrate = strtol(argv[3], &end, 10);
				else filename = argv[3];
			}
			if(argc > 4) {
				printf("Too many arguments\r\n");
				return 0;
			}
		}
		if(baudrate <= 0) baudrate = DEFAULT_BAUDRATE;
		handle_hexload_uart1(baudrate);
		write_file(filename);
		return 0;
	}

	if(strcmp("vdp",argv[1]) == 0)
	{
		if(argc > 3) {
			printf("Too many arguments\r\n");
			return 0;
		}
		if((argc == 3) && (argv[2])) filename = argv[2];
		handle_hexload_vdp();
		write_file(filename);
		return 0;
	}			
	return 0;
}
