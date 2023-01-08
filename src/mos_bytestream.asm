;
; Title:		AGON MOS - MOS hexload assembly code
; Author:		Jeroen Venema
; Created:		23/10/2022
; Last Updated:	26/11/2022
;
; Modinfo:
; 23/10/2022:	Initial assembly code
; 24/10/2022:	0x00 escape code now contained in keymods, resulting in faster upload baudrate
; 26/11/2022:   Adapted as loadable command

	.include "mos_api.inc"
			
	SEGMENT CODE
	.ASSUME	ADL = 1
			
	XDEF	_hxload_vdp
	XDEF	_getTransparentByte
	XREF	__putch
	
; receive a single keycode/keymods packet from the VDU
; keycode can't be 0x00 - so this is escaped with keymods code 0x01
; return transparent byte in a
_getTransparentByte:
	push iy
	push ix

	call getSysvars
	call getTransparentByte
	
	pop ix
	pop iy
	ret

; get pointer to sysvars in ixu
getSysvars:
	ld a, mos_sysvars		; MOS call for mos_sysvars
	rst.lil 08h				; returns pointer to sysvars in ixu
	ret

; call here when ixu has been set previously for performance
getTransparentByte:
	ld a, (ix+sysvar_keycode)
	or a
	jr z, getTransparentByte
	ld c,a
	xor a				; acknowledge receipt of key / debounce key
	ld (ix+sysvar_keycode),a
	ld a,c

	cp a,01h			; check for escape code 0x01
	jr nz, getbyte_done ; no escape code received
	ld c,a				; save received byte
	ld a,(ix+sysvar_keymods)		; is byte escaped with >0 in _keymods?
	or a
	jr z, getbyte_nesc  ; not escaped: normal transmission of 0x01
	ld c,0				; escaped 0x01 => 0x00 meant
getbyte_nesc:
	ld a,c				; restore to a
getbyte_done:
	ret

; hxload_vdp received records from the VDP
; each record has
; start - byte - X:Address/Data record, 0:end transmission
; HLU   - byte - high byte address
; H     - byte - medium byte address
; L     - byte - low byte address
; N     - byte - number of databytes to read
; 0     - N-1 - bytes of data
; 
; Last record sent has N == 0, but will send address bytes, no data bytes

_hxload_vdp:
	push	de
	push	bc
	push	ix						; safeguard main ixu	
	call	getSysvars				; returns pointer to sysvars in ixu
blockloop:
	ld		d,0						; reset checksum
	call	getTransparentByte		; ask for start byte
	or		a
	jr		z, rbdone				; end of transmission received

	add		a,d
	ld		d,a
	call	getTransparentByte		; ask for byte HLU
	ld		(hexload_address+2),a	; store
	add		a,d
	ld		d,a
	call	getTransparentByte		; ask for byte H
	ld		(hexload_address+1),a	; store
	add		a,d
	ld		d,a
	call	getTransparentByte		; ask for byte L
	ld		(hexload_address),a		; store
	add		a,d
	ld		d,a
	call	getTransparentByte		; ask for number of bytes to receive
	ld		b,a						; loop counter
	add		a,d
	ld		d,a

	ld		hl, hexload_address		; load address of pointer
	ld		hl, (hl)				; load the (assembled) pointer from memory
$$:
	call	getTransparentByte		; receive each byte in a
	ld		(hl),a					; store byte in memory
	add		a,d
	ld		d,a
	inc		hl						; next address
	djnz	$B						; next byte
	ld		a,d
	neg								; compute 2s complement from the total checksum						
	rst.lil	10h 					; return to VDP
	jp		blockloop
	
rbdone:
	pop ix
	pop bc
	pop de
	ret
	
	
	SEGMENT DATA

; Storage for pointer to sysvars

sysvarptr:			DS 3
hexload_address		DEFB	3	; 24bit address
