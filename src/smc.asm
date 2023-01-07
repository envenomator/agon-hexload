	XDEF _copySelfReset
	XDEF _endcopySelfReset
	
	segment CODE
	.assume ADL=1

_copySelfReset:
	push	ix
	ld		ix,0
	add		ix,sp
		
	ld		de, (ix+6)	; destination address
	ld		hl, (ix+9)	; source
	ld		bc, (ix+12)	; number of bytes to write to flash
	ldir

	rst		0
	; will never get here
	
_endcopySelfReset:
	
end