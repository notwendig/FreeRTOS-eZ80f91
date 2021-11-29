	include CPM.INC

SIZE:    EQU     (10000h-CCP)  	; size of cp/m system
SECTS:   EQU     32h			;(SIZE / 128)   ; # of sectors to load

if _EZ80F91
	DEFINE  BOOTSEG,SPACE=RAM,ORG=0
	SEGMENT BOOTSEG
	.assume adl=0
else
	org 0
endif
		
COLD:	LD		SP,128
		LD		HL,bootmsg
BMSG:	LD		A,(HL)
		OR		A
		JR		Z,LOAD
		EXBIOS	CONIO, 0, CONDAT
		INC		HL
		JR		BMSG
		
LOAD:;	XOR		A				; select drive A
		LD		HL,0			; HL=0 IBM 3740 default dph
		EXBIOS  FDIO, 0, FDCD	; select drive a
		EXBIOS  FDIO, 1, FDCST  ; get status of fdc

		LD      HL,2            ; h=track 0, l=sector 2
		LD      D,SECTS         ; d=# sectors to load
 		LD      IX,CCP          ; base transfer address

		OR		A				; selected?
		JR		Z,LSECT			; yes read system
EXMON:	EXBIOS  MONITOR, 0, 0	; build-in monitor

;
;       load the next sector
;
NEXTS:
		ADD		IX,SP           ; 128 bytes per sector
		INC     L               ; sector = sector + 1
		LD      A,L
		CP      27              ; last sector of track ?
		JR      C,LSECT         ; no, go read another
;
;       end of track, increment to next track
;
		LD      L,1             ; sector = 1
		INC     H               ; track = track + 1
		JR      LSECT           ; for another group


LSECT: 	XOR     A
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
		JP		BIOS
ifdef _EZ80F91
		BLKB	80h - 5dh - 12,ffh
bootmsg:DB 		"BLi-"
else
		DEFS	80h - $ - 12,0ffh
bootmsg:DB 		"BLe-"		
endif
		
ID:
		DB		"EZ80F91",0
ENDID:

	END
