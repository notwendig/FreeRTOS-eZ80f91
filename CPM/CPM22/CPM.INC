
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
CCP:	EQU	3400H+BIAS	;base of ccp
BDOS:	EQU	CCP+800H+6	;base of bdos
BIOS:	EQU	CCP+1600H	;base of bios

;    define MINIMON_, space=rom,org=8000h


CDISK:	EQU	0004H		;current disk number 0=A,...,15=P

;
;   BIOS entrys
;
	XREF BOOT
	XREF WBOOT
	XREF CONST
	XREF CONIN
	XREF CONOUT
	XREF LISTN
	XREF PUNCH
	XREF READER
	XREF HOME
	XREF SELDSK
	XREF SETTRK
	XREF SETSEC
	XREF SETDMA
	XREF READ
	XREF WRITE
	XREF LISTST
	XREF SECTRN
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


SERIALNO macro
		db	0,22,0,0,0,0
		endm
