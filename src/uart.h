#ifndef UART_H
#define UART_H

#define DATABITS_8				8						//!< The default number of bits per character used in the driver.
#define DATABITS_7				7
#define DATABITS_6				6
#define DATABITS_5				5

#define STOPBITS_1				1
#define STOPBITS_2				2						//!< The default number of stop bits used in the driver.

#define PAR_NOPARITY			0	    	        	//!< No parity.
#define PAR_ODPARITY			1	    	        	//!< Odd parity.
#define PAR_EVPARITY			3	    	        	//!< Even parity.

#define UART_IER_RECEIVEINT				((unsigned char)0x01)		//!< Receive Interrupt bit in IER.
#define UART_IER_TRANSMITINT			((unsigned char)0x02)		//!< Transmit Interrupt bit in IER.
#define UART_IER_LINESTATUSINT			((unsigned char)0x04)		//!< Line Status Interrupt bit in IER.
#define UART_IER_MODEMINT				((unsigned char)0x08)		//!< Modem Interrupt bit in IER.
#define UART_IER_TRANSCOMPLETEINT		((unsigned char)0x10)		//!< Transmission Complete Interrupt bit in IER.
#define UART_IER_ALLINTMASK				((unsigned char)0x1F)		//!< Mask for all the interrupts listed above for IER.

#if !defined(UART1_IVECT) 								//!< If it is not defined 
#define UART1_IVECT				0x1A		        	//!< The UART0 interrupt vector.
#endif

#endif UART_H