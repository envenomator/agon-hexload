			.ASSUME	ADL = 1
			SEGMENT CODE
			
			XREF	_uart1_getch
			XDEF	_hxload
			
LOAD_HLU_DEFAULT	.EQU	04h		; 0x040000 default load address

_hxload:
			PUSH	DE
			PUSH	HL
			PUSH	BC
			
			XOR		A,A
			LD		(hexload_error),A		; clear error counter
			LD		A,LOAD_HLU_DEFAULT		; default upper byte load address 0x040000 in HLU
			LD		(hexload_address+2),A	; in case record 04 goes missing
hxline:
			CALL	_uart1_getch
			CP		':'
			JR		NZ, hxline				; ignore everything except ':' to start the line
			LD		E,0						; reset the checksum byte
			CALL	getbyte					; record length count - D
			LD		D,A
			CALL	getbyte					; relative load address - H
			LD		H,A
			CALL	getbyte					; relative load address - L
			LD		L,A
			CALL	getbyte					; record field type
			CP		0
			JR		Z, hexdata
			CP		1
			JR		Z, hexendfile
			CP		4
			JR		Z, hex_address
			JR		hxline					; ignore all other record types, find next line

hex_address:
			CALL	getbyte					; load and ignore address[31:24] / upper byte
			CALL	getbyte					; load address[23:16]
			LD		(hexload_address + 2),A	; store HLU
			XOR		A,A
			LD		(hexload_address + 1),A	; clear address[15:0] - this will be loaded from record type 0
			LD		(hexload_address),A
			LD		HL, hexload_address
			LD		HL,(HL)					; load assembled address from memory into HL as a pointer
			CALL	getbyte					; load checksum
			LD		A,E
			OR		A
			JR		NZ, hexckerr			; checksum error on this line
			JR		hxline
hexdata:
			CALL	getbyte
			LD		(HL),A
			INC		HL
			DEC		D
			JR		NZ, hexdata
			CALL	getbyte					; load checksum byte
			LD		A,E
			OR		A						; sum of all data incl checksum should be zero
			JR		NZ, hexckerr			; checksum error on this line
			JR		hxline
hexckerr:
			LD		A,(hexload_error)
			INC		A
			LD		(hexload_error),A		; increment error counter
			JR		hxline
	
hexendfile:	
			LD		A,(hexload_error)		; return error counter to caller
			POP		BC
			POP		HL
			POP		DE
			RET

getbyte:
			CALL	getnibble
			RLCA
			RLCA
			RLCA
			RLCA
			LD		B,A
			CALL	getnibble
			OR		B						; combine two nibbles into one byte
			LD		B,A						; save byte to B
			ADD		A,E						; add to checksum
			LD		E,A						; save checksum
			LD		A,B
			RET

getnibble:
			CALL	_uart1_getch
			SUB		'0'
			CP		10
			RET		C						; A<10
			SUB		7						; subtract 'A'-'0' (17) and add 10
			CP		16
			RET		C						; A<'G'
			AND		11011111b				; toUpper()
			RET								; leave any error character in input, checksum will catch it later
			
	
			SEGMENT DATA
hexload_address		DEFB	3	; 24bit address
hexload_error		DEFB	1	; error counter				