	.include "cpm.inc"
	
	xref	_loader	
	
	SEGMENT BSS
	
	.align 10000h
	xdef _cpmram
_cpmram:	ds 10000h
	
	SEGMENT code
	.assume adl=0
bootstrap:	LD		SP,_loader
			CALL	cprn - $ +3
			DB 		"BOOTSTRAP v1.0.0",CR,LF,0
			;XOR		A				; a=0 select drive A
			SBC		HL,HL			; HL=0 IBM 3740 default dph
			LD		BC,HL			
			EXBIOS  FDIO, 0, FDCD	; select drive a
			EXBIOS  FDIO, 1, FDCST  ; get status of fdc
			OR		A,A
			JR		NZ,$F			; build-in monitor
			EXBIOS  FDIO, 0, FDCTBC	; set track 0
			INC		C
			EXBIOS  FDIO, 0, FDCSBC	; set sector 1
			LD		BC,_loader		
			EXBIOS  DMAIO, 0, DMABC	; set dma at tbase
			EXBIOS  FDIO, 0, FDCOP	; a=0 => read sector
			EXBIOS  FDIO, 1, FDCST  ; get status of fdc
			OR      A               ; read successful ?
			JP		Z,_loader
$$:			NOP
			EXBIOS  MONITOR, 0, 0	
cprn:		POP		HL
$$:			LD		A,(HL)
			INC		HL
			OR		A
			JR		Z,$F
			EXBIOS	CONIO, 0, CONDAT
			JR		$B	
$$:			PUSH	HL
			RET
			
bootmsg:	
bootstrapend:
	
	.assume adl=1

	xdef _CPM22Task;void CPM22Task(char* mem) 
_CPM22Task:	push	ix
			ld		ix,0
			add		ix,sp
			ld 		hl, bootstrap
			ld 		de, (ix+6)								
			ld 		bc, bootstrapend - bootstrap
			ldir										
			ld 		a,(ix+8)									
			ld 		mb,a									
			ld 		hl, (ix+6)									
			jp.s	(hl)
