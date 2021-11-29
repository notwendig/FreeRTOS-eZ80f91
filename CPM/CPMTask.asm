	.include "cpm.inc"
	
	SEGMENT BSS
	.align 10000h
	
	xdef _cpmram
_cpmram:	ds 10000h
	
	SEGMENT code
	.assume adl=1
	xref	_COLD

	xdef _CPM22Task;void CPM22Task(char* mem) 
_CPM22Task:
	push	ix
	ld		ix,0
 	add		ix,sp
	ld 		de, _COLD
	ld 		hl, (ix+6)								
	adc		hl,de
	ex		de,hl
	ld 		bc,80h									
	ldir										
	ld 		a,(ix+8)									
	ld 		mb,a										
	jp.s	_COLD
