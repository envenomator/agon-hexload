/*
 * Title:			AGON Hexload code
 * Author:			Jeroen Venema
 * Created:			22/10/2022
 * Last Updated:	04/04/2023
 * 
 * Modinfo:
 * 22/10/2022:		Initial version MOS patch
 * 23/10/2022:		Receive_bytestream in assembly
 * 26/11/2022:		MOS commandline version
 * 07/01/2023:		New UART1 code
 * 11/01/2023:		Release 0.9
 * 23/02/2023:		Bugfix - DEFB used, should have been DS at end of hxload.asm
 *                  Option to save as file to SD card
 * 30/03/2023:		Preparation for MOS 1.03
 * 04/04/2023:		Making use of mos_setintvector, kept own UART1 handler for speed
 */

#define DEFAULT_BAUDRATE 		 384000

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "mos-interface.h"
#include "uart.h"

// external assembly routines
extern VOID	hxload_vdp(void);
extern CHAR	hxload_uart1(void);
extern VOID	uart1_handler(void);

// external variables
extern volatile UINT24 startaddress;
extern volatile UINT24 endaddress;
extern volatile UINT24 datarecords;
extern volatile UINT8  defaultAddressUsed;

// single VDP function needed
UINT8 vdp_cursorGetXpos(void);

int errno; // needed by stdlib

typedef void * rom_set_vector(unsigned int vector, void(*handler)(void));

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
	/*
	// First we need to test the VDP version in use
	printf("\r");	// set the cursor to X:0, Y unknown, doesn't matter
	// No local output, the VDP will handle this
	*/
	// set vdu 23/28 to start HEXLOAD code at VDU
	putch(23);
	putch(28);
	/*
	
	// A regular VDP will have the cursor at X:0, the patched version will send X:1
	if(vdp_cursorGetXpos() != 1)
	{
		printf("Incompatible VDP version\r\n");
		return;
	}
	*/
	// We can't transmit any text during bytestream reception, so the VDU handles this remotely
	hxload_vdp();
}


void handle_hexload_uart1(UINT24 baudrate)
{
	CHAR c;
	void *oldvector;
	UART 	pUART;

	pUART.baudRate = baudrate;
	pUART.dataBits = 8;
	pUART.stopBits = 1;
	pUART.parity = PAR_NOPARITY;
	pUART.flowcontrol = 0;
	pUART.interrupts = UART_IER_RECEIVEINT;

	oldvector = mos_setintvector(UART1_IVECT, uart1_handler);
	
	init_UART1();
	open_UART1(&pUART);
	// Only feedback during transfer - we have no time to output to VDP or even UART1 between received bytes
	printf("Receiving Intel HEX records - UART1:%d 8N1\r\n",baudrate);
	c = hxload_uart1();
	if(c == 0) {
		printf("%d datarecords\r\n",datarecords);
		printf("Start address 0x%06X",startaddress);
		if(defaultAddressUsed) printf(" (default)\r\n");
		else printf("\r\n");
		printf("OK\r\n");
	}
	else printf("%d error(s)\r\n",c);

	// close UART1, so no more interrupts will trigger before removal of handler
	close_UART1();
	// disable UART1 interrupt, set previous vector (__default_mi_handler in MOS ROM, might change on every revision)
	mos_setintvector(UART1_IVECT, oldvector);
}

int main(int argc, char * argv[]) {
	UINT24 baudrate = 0;
	char *end;
	char *filename = NULL;

	if((argc >= 2) && (strcmp("uart1",argv[1]) == 0))
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

	if((argc >= 2) && (strcmp("vdp",argv[1]) == 0)) {
		if(argc > 3) {
			printf("Too many arguments\r\n");
			return 0;
		}
		if((argc == 3) && (argv[2])) filename = argv[2];
		handle_hexload_vdp();
		write_file(filename);
		return 0;
	}			
	printf("Usage: hexload <uart1 [baudrate]| vdp> [filename]\r\n");
	return 0;
}

UINT8 vdp_cursorGetXpos(void)
{
	unsigned int delay;
	
	putch(23);	// VDP command
	putch(0);	// VDP command
	putch(0x82);	// Request cursor position
	
	delay = 255;
	while(delay--);
	return(getsysvar_cursorX());

}
