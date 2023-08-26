ifdef CPM22
include CPM.INC
include HWC.INC

	define _BOOT,space=ram,org=80h
	segment _BOOT
	.assume adl=0


_loader:LD		SP,$
		CALL	cprn
		DB 		"CP/M 2.2 Loader v1.0 ",0
		XOR		A,A				; a =0 select drive A
		SBC		HL,HL			; HL=0 IBM 3740 default dph
		EXBIOS  FDIO, 0, FDCD	; select drive a
		EXBIOS  FDIO, 1, FDCST  ; get status of fdc
		LD		H,BIOSSEC / 26	; h=track start of BIOS
		LD      L,BIOSSEC % 26  ; l=sector 2 V
		LD      D,BIOSSECN	    ; d=# sectors to load
 		LD      IX,BIOS         ; BIOS transfer address

$next:	OR		A				; DSK error
		JR		NZ,EXMON		; yes go to buildin monitor
		LD		A,'.'
		EXBIOS	CONIO, 0, CONDAT
		LD		B,0
		LD		C,H				; set track
		EXBIOS  FDIO, 0, FDCTBC
		LD      C,L             ; set sector
		EXBIOS  FDIO, 0, FDCSBC
		PUSH	IX
		POP		BC				; set dma
		EXBIOS  DMAIO, 0, DMABC
		XOR		A,A
		EXBIOS  FDIO, 0, FDCOP	; a=0 => read sector
		EXBIOS  FDIO, 1, FDCST  ; get status of fdc
		ADD		IX,SP
		INC		L
		DEC		D
		JR 		NZ, $next
		CALL	cprn
		DB 		"OK",CR,LF,0
		JP		BOOT

EXMON:	EXBIOS  MONITOR, 0, 0	; build-in monitor

cprn:	POP		HL
$pchar:	LD		A,(HL)
		INC		HL
		OR		A
		JR		Z,$ret
		EXBIOS	CONIO, 0, CONDAT
		JR		$pchar
$ret:	JP		(HL)
		DS		8
_LOADEREND	equ	$
endif  ; ifdef CPM22
	END
0
