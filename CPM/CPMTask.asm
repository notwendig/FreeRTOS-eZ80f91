	.include "cpm.inc"
	.include "hwc.inc"
	
	xref _cpm22img
	SEGMENT TEXT
err_memalign:
	ascii 0dh,0ah,"CP/M Memory misaligned.",0dh,0ah,0
	
	SEGMENT BSS
	
	.align 10000h
	xdef _cpmram
_cpmram:	ds 10000h
	
	SEGMENT code
	.assume adl=0
	
bootstrap:	ld		sp,0
			xor		a,a			; a =0 select drive A
			sbc		hl,hl		; HL=0 IBM 3740 default dph
			push	hl
			EXBIOS  FDIO, 0, FDCD	; select drive a
			EXBIOS  FDIO, 1, FDCST  ; get status of fdc
			or		a,a
			pop		bc
			jr		nz,monitor
			EXBIOS  FDIO, 0, FDCTBC
			inc		c
			EXBIOS  FDIO, 0, FDCSBC
			ld		bc,LOADER	; set dma
			EXBIOS  DMAIO, 0, DMABC
			EXBIOS  FDIO, 0, FDCOP	; a=0 => read sector
			EXBIOS  FDIO, 1, FDCST  ; get status of fdc
			or		a,a
			jp		z,LOADER
monitor:	EXBIOS  MONITOR, 0, 0	; build-in monitor
			halt
endbootstrap:
	
	.assume adl=1


	xdef _CPM22Task;void CPM22Task(char* mem) 
_CPM22Task:	push	ix
			ld		ix,0
			add		ix,sp
			ld 		hl, bootstrap
			ld 		de, (ix+6)								
			ld			a,d
			or			a,e
			jr			z,$F
			
$$:		ld 		bc, endbootstrap-bootstrap
			ldir										
			ld 		a,(ix+8)									
			ld 		mb,a									
			jp.s	0
			
	END 