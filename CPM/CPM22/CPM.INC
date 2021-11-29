;	CBIOS for eZ80 Z80-Mixed-Mode
;
;	Copyright (C) 1998-2021 by Juergen Sievers
;
MEM:		EQU	64	;cp/m version memory size in kilobytes
;
;	"bias" is address offset from 3400H for memory systems
;	than 16K (referred to as "b" throughout the text).
;
BIAS:	EQU	(MEM-20)*1024
CCP:		EQU	3400H+BIAS	;base of ccp
BDOS:	EQU	CCP+806H	;base of bdos
BIOS:	EQU	CCP+1600H	;base of bios
CDISK:	EQU	0004H		;current disk number 0=A,...,15=P


;
;	I/O ports
;
MONITOR: EQU 30h

CONIO:	EQU 31h		; CONOLE, PRT, AUX Device
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

PORTOUT:EQU	0
PORTINP:EQU	1

; Execute ADL=1 mode code from ADL=0 mode (plain Z80-mode) code by casing a trap exception.
; Macro to tram ez80f91 on z80 mixed mode
; for simulat io
; $cb, device ($30..$34), direction (0=in,1=out), port
EXBIOS  MACRO device, direction, port
	db	0cbh, device, direction, port
	ENDM

;
;   Set control character equates.
;
CNTRLC	EQU	3		;control-c
CNTRLE	EQU	05H		;control-e
BS		EQU	08H		;backspace
TAB		EQU	09H		;tab
LF		EQU	0AH		;line feed
FF		EQU	0CH		;form feed
CR		EQU	0DH		;carriage return
CNTRLP	EQU	10H		;control-p
CNTRLR	EQU	12H		;control-r
CNTRLS	EQU	13H		;control-s
CNTRLU	EQU	15H		;control-u
CNTRLX	EQU	18H		;control-x
CNTRLZ	EQU	1AH		;control-z (end-of-file mark)
DEL		EQU	7FH		;rubout

IOBYTE	EQU	3		;i/o definition byte.
TDRIVE	EQU	4		;current drive name and user number.
ENTRY	EQU	5		;entry point for the cp/m bdos.
TFCB	EQU	5CH		;default file control block.
TBUFF	EQU	80H		;i/o buffer and command line storage.
TBASE	EQU	100H		;transiant program storage area.

    define CPM22,space=ROM
SERIALNO macro
		db	0,22,0,0,0,0
		endmac
