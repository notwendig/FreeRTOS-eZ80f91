	include CPM.INC

	xref	BOOT
SIZE:    EQU     (10000h-CCP)  	; size of cp/m system
SECTS:   EQU     32h			;(SIZE / 128)   ; # of sectors to load

	SEGMENT CPM22
	.assume adl=0
	ORG	CCP-128
	
	xdef	_loader
_loader:LD		SP,80h
		CALL	cprn
		DB 		"CP/M 2.2 Loader v1.0 ",0
		XOR		A,A				; a =0 select drive A
		SBC		HL,HL			; HL=0 IBM 3740 default dph
		EXBIOS  FDIO, 0, FDCD	; select drive a
		EXBIOS  FDIO, 1, FDCST  ; get status of fdc

		LD      L,2             ; h=track 0, l=sector 2
		LD      D,SECTS         ; d=# sectors to load
 		LD      IX,CCP          ; base transfer address
		OR		A				; selected?
		JR		NZ,EXMON		; yes read system
;
;       load the next sector
;
NEXTS:	LD		A,'.'
		EXBIOS	CONIO, 0, CONDAT
		ADD		IX,SP           ; 128 bytes per sector
		INC     L               ; sector = sector + 1
		LD      A,L
		CP      27              ; last sector of track ?
		JR      C,$F	        ; no, go read another
;
;       end of track, increment to next track
;
		LD      L,1             ; sector = 1
		INC     H               ; track = track + 1
$$: 	XOR     A
		LD		B,A
		LD		C,H				; set track
		EXBIOS  FDIO, 0, FDCTBC
		LD      C,L             ; set sector
		EXBIOS  FDIO, 0, FDCSBC
		PUSH	IX
		POP		BC				; set dma
		EXBIOS  DMAIO, 0, DMABC
		EXBIOS  FDIO, 0, FDCOP	; a=0 => read sector
		EXBIOS  FDIO, 1, FDCST  ; get status of fdc
		OR      A               ; read successful ?
		JR      NZ,EXMON        ; no, start buildin monitor

		DEC     D               ; sects--
		JR      NZ,NEXTS        ; head for the bios
		CALL	cprn
		DB 		"OK",CR,LF,0
		JP		BOOT

		
EXMON:	EXBIOS  MONITOR, 0, 0	; build-in monitor

cprn:	POP		HL
$$:		LD		A,(HL)
		INC		HL
		OR		A
		JR		Z,$F
		EXBIOS	CONIO, 0, CONDAT
		JR		$B	
$$:		JP		(HL)
loaderend:		

	END
