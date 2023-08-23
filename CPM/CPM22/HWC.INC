; eZ80F91 FreeRTOS Z80 CP/M 2.2 Hardware simulation

;
;	I/O ports
;
MONITOR: EQU 30h

CONIO:	    EQU    31h		; CONOLE, PRT, AUX Device
CONSTA:	EQU	01h		;console status port
CONDAT:	EQU	02h		;console data port
PRTSTA:	EQU	03h		;printer status port
PRTDAT:	EQU	04h		;printer data port
AUXDAT:	EQU	05h		;auxiliary data port

FDIO:	EQU 32h		; FLOPPY Device
FDCD:	EQU	01h		;fdc-port: # of drive in A. 0 = A, 1 = B ... HL points to DPH
FDCTBC:	EQU	02h		;fdc-port: # of track in BC
FDCSBC:	EQU	03h		;fdc-port: # of sector in BC
FDCOP:	EQU	04h		;fdc-port: command	A == 0 write, 1 read
FDCST:	EQU	05h		;fdc-port: status	Disk Status to A

DMAIO:	EQU 33h		; DMA Device
DMABC:	EQU	01h		;dma-port: dma address BC

ROMBOOT:EQU	34h		; boot buildin CP/M 2.2
BDOSCALL:EQU 35h

PORTOUT:EQU	0
PORTINP:EQU	1

; Execute ADL=1 mode code from ADL=0 mode (plain Z80-mode) code by casing a trap exception.
; Macro to tram ez80f91 on z80 mixed mode
; for simulat io
; $cb, device ($30..$35), direction (0=in,1=out), port
EXBIOS  MACRO device, direction, port
	db	0cbh, device, direction, port
	ENDM

EXBDOS MACRO
	db BDOSCALL
	ENDM

