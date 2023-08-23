	; Z80 ICPLAN Monitor
	; Jens Dietrich - Germany 
	; Version 1.01
	; 2.4576 Mhz 30-09-2001
	; mit CTC Interrupt bei Next
ifdef CPM22
	; IO Adressen
	; es werden nur 04,06,08 verwendet
		include HWC.INC
	;sioad	equ CONDAT		;SIO A Daten
	;siobd	equ 05h		;SIO B Daten
	;sioas	equ CONSTA		;SIO A Steuerung
	;siobs	equ 07h		;SIO B Steuerung
	;ctc0	equ 08h		;CTC Kanal 0
	;ctc1	equ 09h		;CTC Kanal 1
	;ctc2	equ 0ah		;CTC Kanal 2
	;ctc3	equ 0bh		;CTC Kanal 3
	
    include CPM.INC	
	;Monitor Hauptprogramm
	;RAM 0-FFh wird vom Monitor verwendet

	segment MINIMON_SEG
		.assume adl=0

MINIM:
	JR 	kalt
inmo:			;initialisierung
	LD 	HL,004Dh	;RAM loeschen
	LD 	DE,004Eh
	LD 	(HL),00h
	LD 	BC,0015h
	LDIR
kalt:
	LD 	SP,00B0h	;Stack setzen
	LD 	A,C3h
	LD 	(0020h),A	;RST20 Sprungbefehl
	LD 	HL,anfrs20
	LD 	(0021h),HL	;RST20 Sprungadresse
	call	init_ctc_sio	;CTC und SIO initialisieren
	call	zeiloesch
	LD 	HL,workram	;Arbeitsbereich RAM fuellen
	LD 	DE,0033h
	LD 	BC,000Ch
	LDIR
	RST 	20h
	defb 	02h		;Starttext ausgeben
	defb 	0Ch		;CLS	
	defb 	0Dh		;enter
	defb 	0Dh		;enter
	defb 	"ICPLAN Z80 / V1.01 30-09-2001"
	defb 	0ah,0Dh+80h	;enter
	LD 	HL,0090h	;Registerrettebereich
	LD 	(0063h),HL
	IM 	2
	JR 	start2
start1:
	LD 	SP,00B0h
	RST 	20h
	defb 	02h
	defb 	BFh			;Fragezeichen + Bit 7
start2:
	RST 	20h
	defb 	05h
	LD 	DE,(0016h)		;Anfangsadresse Eingabezeile
	CALL 	inc_de_nospace
	LD 	B,A			;1 Zeichen in Akku B
	INC 	DE
	LD 	A,(DE)
	LD 	C,A
	PUSH 	BC
	INC 	DE
	RST 	20h
	defb 	03h
	JR 	NZ,START3
	LD 	A,(DE)
	CP 	3Ah
	JR 	Z,START4
START3:
	LD 	(001Bh),HL
	RST 	20h
	defb 	03h
	LD 	(001Dh),HL
	RST 	20h
	defb 	03h
	LD 	(0023h),HL
START4:
	POP 	BC
	EX 	AF,AF'
	LD 	(0025h),DE
	LD 	HL,JUMPTAB	;Tabelle Monitorcodes
START5:
	LD 	A,(HL)
	CP 	B
	JR 	Z,START6		;Tabellenende?
	INC 	HL
	INC 	HL
	INC 	HL
	OR 	A
	JR 	NZ,START5
	LD 	A,B
	CP 	40h		;@ Zeichen fuer Erweiterung
	JR 	NZ,start1
	LD 	HL,00B0h	;Tabelle im RAM Bereich
	LD 	B,C
	JR 	START5
START6:
	INC 	HL
	LD 	E,(HL)
	INC 	HL
	LD 	D,(HL)
	EX 	DE,HL
	EX 	AF,AF'
	LD 	BC,start2
	PUSH 	BC
	JP 	(HL)
JUMPTAB:
	defb 	'1'
	dw 	test		;kill RAM wieder entfernen !!!
	defb 	'1'
	dw 	break
	defb 	'1'
	dw 	comp
	defb 	'1'
	dw 	disp
	defb 	'1'
	dw 	exec
	defb 	'1'
	dw 	kill
	defb 	'1'
	dw 	gogo
	defb	'1'
	dw	help
	defb 	'1'
	dw 	inmo
	defb 	'1'
	dw 	jump
	defb 	'1'
	dw 	down
	defb 	'1'
	dw 	mody
	defb 	'1'
	dw 	next
	defb 	'1'
	dw 	port
	defb 	'1'
	dw 	remo
	defb 	'1'
	dw 	snext		;Schleife Next
	defb 	'1'
	dw 	trans
	defb 	'1'
	dw 	wind
	NOP

anfrs20:		;RST20 Adresse decodieren
	EX 	(SP),HL
	PUSH 	AF
	LD 	A,(HL)
	LD 	(0003h),A
	INC 	HL
	POP 	AF
	EX 	(SP),HL
	PUSH 	HL
	PUSH 	BC
	PUSH 	AF
	LD 	HL,RST20TAB	;RST20 Sprungtabelle
	LD 	A,(0003h)
	SLA 	A
	LD 	C,A
	LD 	B,00h
	ADD 	HL,BC
	LD 	A,(HL)
	INC 	HL
	LD 	H,(HL)
	LD 	L,A
	POP 	AF
	POP 	BC
	EX 	(SP),HL
	RET

RST20TAB:		;RST 20 Tabelle
	dw 	outch		;0
	dw 	inch		;1
	dw 	prst7		;2
	dw 	inhex		;3
	dw 	inkey		;4
	dw 	inlin		;5
	dw 	outhx		;6
	dw 	outhl		;7
	dw 	outsp		;8-Fehlbelegung
	dw 	outsp		;9-Fehlbelegung
	dw 	mody		;a		
	dw 	wind		;b-geht nicht!
	dw 	othls		;c
	dw 	outdp		;d
	dw 	outsp		;e
	dw 	trans		;f
	dw 	instr		;10
	dw 	kill		;11

workram:
	defb 	D0h,07h,E0h,03h,00h,C3h
	dw 	start1			;RST38 Adresse
	defb 	00h,ECh,00h,F0h

LESEPAR:
	LD 	HL,(001Bh)
	LD 	DE,(001Dh)
	LD 	BC,(0023h)
	RET

kill:			;KILL db11
	CALL 	LESEPAR		;HL,DE,BC mit Parametern
	LD 	(HL),C
	PUSH 	HL
	XOR 	A
	EX 	DE,HL
	SBC 	HL,DE
	LD 	B,H
	LD 	C,L
	POP 	HL
	LD 	D,H
	LD 	E,L
	INC 	DE
	LDIR
	RET

trans:			;TRANS db0F
	CALL 	LESEPAR		;HL,DE,BC mit Parametern
	XOR 	A
	PUSH 	HL
	SBC 	HL,DE
	POP 	HL
	JR 	C,trans1
	LDIR
	RET
trans1:
	ADD 	HL,BC
	EX 	DE,HL
	ADD 	HL,BC
	EX 	DE,HL
	DEC 	HL
	DEC 	DE
	LDDR
	RET

UPR01:
	LD 	(0013h),SP
	LD 	SP,0061h
	PUSH 	IX
	PUSH 	IY
	PUSH 	AF
	PUSH 	BC
	PUSH 	DE
	PUSH 	HL
	EXX
	EX 	AF,AF'
	PUSH 	AF
	PUSH 	BC
	PUSH 	DE
	PUSH 	HL
	JR 	UPR03
UPR02:
	LD 	(0013h),SP
	LD 	SP,004Dh
	POP 	HL
	POP 	DE
	POP 	BC
	POP 	AF
	EXX
	EX 	AF,AF'
	POP 	HL
	POP 	DE
	POP 	BC
	POP 	AF
	POP 	IY
	POP 	IX
UPR03:
	LD 	SP,(0013h)
	RET
UPR04:
	CALL 	UPR01
	POP 	HL
	LD 	(0063h),SP
	LD 	SP,00B0h
	DEC 	HL
	DEC 	HL
	DEC 	HL
	LD 	(0061h),HL
	LD 	DE,(000Bh)
	LD 	HL,000Dh
	LD 	BC,0003h
	LDIR
	CALL 	REMO1
	JP 	start2

break:			;breakpoint-hp
	LD 	HL,(001Bh)
	LD 	(000Bh),HL
	LD 	DE,000Dh
	LD 	BC,0003h
	LDIR
	CALL 	REMO1
	RET

exec:			;execute
	LD 	HL,(000Bh)
	LD 	(HL),CDh
	INC 	HL
	LD 	DE,UPR04
	LD 	(HL),E
	INC 	HL
	LD 	(HL),D

jump:			;jump
	LD 	HL,(001Bh)
	LD 	(0061h),HL
	LD 	SP,(0063h)
	PUSH 	HL
	JP 	UPR02

gogo:			;go
	LD 	HL,(0061h)
	LD 	(001Bh),HL
	LD 	DE,(000Bh)
	XOR 	A
	SBC 	HL,DE
	JR 	NZ,exec
	JR 	jump ; todo: 
GOGO1:
	RST 	20h
	defb 	02h
	defb 	31h,A0h
	RET
GOGO2:
	JR 	NZ,GOGO1
	RST 	20h
	defb 	02h
	defb 	30h,A0h
	RET

remo:			;register modify
	CP 	3Ah
	JP 	NZ,REMO5
REMO1:
	RST 	20h
	defb 	02h
	defb 	0Dh,42h,D0h
	LD 	HL,000Ch
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	42h,53h,BAh
	LD 	B,03h		;naechsten 3 Datenbyte zeigen
	LD 	HL,000Dh
REMO2:
	LD 	A,(HL)
	RST 	20h
	defb 	06h
	INC 	HL
	DJNZ 	REMO2
	RST 	20h
	defb 	02h
	defb 	20h,20h,20h,53h,20h,5Ah,20h,43h,A0h
	LD 	A,(005Bh)
	LD 	L,A
	BIT 	7h,L
	CALL 	GOGO2	;Flag S zeigen
	BIT 	6h,L
	CALL 	GOGO2	;Flag Z zeigen
	BIT 	0h,L
	CALL 	GOGO2	;Flag C zeigen
	LD 	HL,0064h
	LD 	B,02h
	RST 	20h
	defb 	02h
REMO3:
	defb 	53h,D0h	;SP
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	50h,C3h	;PC
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	49h,D8h	;IX
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	49h,D9h	;IY
	RST 	20h
	defb 	0Dh
REMO4:
	RST 	20h
	defb 	02h
	defb 	41h,C6h	;AF
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	42h,C3h	;BC
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	44h,C5h	;DE
	RST 	20h
	defb 	0Dh
	RST 	20h
	defb 	02h
	defb 	48h,CCh	;HL
	RST 	20h
	defb 	0Dh
	djnz 	REMO4
	rst 	20h		;Enter
	defb 	02h,0dh,8ah
	LD 	HL,(002Bh)
	DEC 	HL
	LD 	(HL),27h
	RET
REMO5:
	LD 	BC,0400h
	LD 	HL,(0016h)	;Anf.Adresse Eingabezeile
	INC 	HL
	INC 	HL
	LD 	DE,REMO3
REMO6:
	LD 	A,(DE)
	CP 	(HL)
	JR 	Z,REMO9
	INC 	DE
REMO7:
	PUSH 	HL
	LD 	HL,0005h
	ADD 	HL,DE
	EX 	DE,HL
	POP 	HL
	INC 	C
	DJNZ 	REMO6
	LD 	B,04h
	LD 	A,C
	CP 	08h
	JR 	NZ,REMO6
	POP 	AF
	RST 	38h
REMO8:		
	DEC 	HL
	JR 	REMO7
REMO9:
	INC 	DE
	INC	HL
	LD 	A,(DE)
	AND 	7Fh
	CP 	(HL)
	JR 	NZ,REMO8
	INC 	HL
	LD 	A,(HL)
	CP 	27h
	LD 	A,C
	JR 	NZ,REMO10
	ADD 	A,04h
REMO10:
	SLA 	A
	LD 	C,A
	LD 	B,00h
	LD 	HL,0064h
	SBC 	HL,BC
	LD 	B,H
	LD 	C,L
	RST 	20h
	defb 	0Ch
	CALL 	inlin
	LD 	DE,(0016h)
	CALL 	inhex
	JR 	NZ,REMO11
	LD 	A,(DE)
	CP 	3Bh
	RET 	Z
REMO11:
	EX 	DE,HL
	PUSH 	BC
	POP 	HL
	LD 	(HL),D
	DEC 	HL
	LD 	(HL),E
	JP 	REMO1

wind:			;WIND db0B
	CALL 	WIND1
	JR 	C,WIND3
	LD 	(0035h),HL
	LD 	(003Bh),BC
	LD 	HL,(001Dh)
	LD 	(003Dh),HL
	LD 	HL,(002Bh)
	LD 	(HL),20h
	LD 	(002Bh),BC
	RET
WIND1:
	LD 	A,(001Ch)
	CP 	ECh
	RET 	C
	LD 	A,(001Bh)
	AND 	E0h
	LD 	(001Bh),A
	LD 	A,(001Dh)
	AND 	E0h
	LD 	(001Dh),A
	LD 	HL,(001Dh)
	LD 	BC,(001Bh)
	SBC 	HL,BC
	RET 	C
	JR 	Z,WIND2
	DEC 	HL
	LD 	A,03h
	CP 	H
	RET 	C
	INC 	HL
	LD 	DE,0040h
	SBC 	HL,DE
	RET 	C
	LD 	DE,0020h
	ADD 	HL,DE
	RET
WIND2:
	SCF
	RET
WIND3:
	POP 	AF
	RST 	38h

next:			;next
	LD 	A,89h		;high ISR-Vektor-Adresse
	;LD 	I,A
	;DI
	LD	A,b0h		;CTC low-Vektor
	;OUT	(ctc0),a
	LD	A,10000001b	;CTC Interrupt einschalten
	;OUT	(ctc0),a
	LD 	HL,(000Bh)		;Speicher Break Adresse
	DEC 	HL
	LD 	A,(HL)
	LD 	(0069h),A
	LD 	(HL),FBh		;EI Code
	LD 	(006Ah),SP
	LD 	SP,(0063h)
	PUSH 	HL
	JP 	UPR02

isr:			;Interuptprogramm CTC
	DI
	CALL 	UPR01
	LD	A,00000001b	;CTC Interrupt ausschalten
;	OUT	(ctc0),a
	LD 	HL,(000Bh)
	DEC 	HL
	LD 	A,(0069h)
	LD 	(HL),A
	POP 	HL
	LD 	(000Bh),HL
	LD 	(0061h),HL
	LD 	(0063h),SP
	LD 	SP,(006Ah)
	LD 	DE,000Dh
	LD 	BC,0003h
	LDIR
	LD 	HL,REMO1
	PUSH 	HL
	RETI

	; Memory bearbeite
	; R=zurueck,Space und Enter naechste Adresse
	; 2 Zeichen Hexzahl schreibt,'1'=Ende
mody:
	ld	hl,(001bh)	;Adresse lesen
	ld	c,00h		;loeschen
	ld	d,02h		;2.Zeichen dann speichern
m00:
	call	outhl		;Adresse ausgeben
	call	outsp		;Leerzeichen ausgeben
	ld	a,(hl)		;Adressinhalt
	call	outhx		;ausgeben
	call	prst7
	defb	20h,'1',A0h	; # ausgeben
m99:
	call	inch		;Zeichen lesen
	call	outch
	ld	b,a		;Zeichen in b retten
	sub	30h
	jr	c,m01		;nicht 0-9,a-f
	cp	0ah
	jr	c,m10		;Sprung bei 0-9
	sub	07h
	cp	0ah
	jr	c,m01		;nicht 0-9,a-f
	cp	10h
	jr	nc,m01		;nicht 0-9,a-f
	jr	m10
m01:
	ld	d,02h		;2 Zeichen erwarten !
	ld	a,b		;Zeichen laden
	cp	'1'		;Adresse verkleinern?
	jr	nz,m03
	dec	hl
m02:
	call	prst7
	defb	0ah,8dh		;Zeilenschaltung
	jr	m00
m03:
	cp	0dh		;Enter?
	jr	nz,m04	
	call	outch
	call	inch
	call	outch
	inc	hl
	jr	m00
m04:
	cp	'1'		;Abbruchzeichen?
	jr	nz,m05
	call	prst7
	defb	0dh,8ah
	ret
m05:
	cp	'1'		;Space
	jr	nz,m02
	inc	hl		;naechste Adresse
	jr	m02
m10:
	sla	c		;7<- C <-0
	sla	c
	sla	c
	sla	c
	add	a,c
	ld	c,a
	dec	d		;2.Zeichen?
	jr	nz,m99
	ld	d,02h		;nachladen mit 2
	ld	(hl),a		;speichern
	cp	(hl)
	jr	z,m12
	call	prst7
	defb	"<-ERROR",0dh,8ah	;Zeilenschaltung
	jr	m00
m12:
	jr	m99

	; Speicherbereiche vergleichen
	; C ANFAD1 ANFAD2 LAENGE
	; Vergleich stoppt wenn Fehler gefunden wurde
	; Enter=weitervergleichen,'1'=Ende 
comp:
	ld	hl,(001bh)	;1.Vergleichsadresse
	ld	de,(001dh)	;2.Vergleichsadresse
	ld	bc,(0023h)	;Laenge
com00:
	ld	a,b		;laenge null?
	or	c
	jr	nz,com01
	call	prst7
	defb	0dh,8ah
	ret
com01:
	ld	a,(de)
	cp	(hl)
	jr	nz,com02
	dec	bc
	inc	hl
	inc	de
	jr	com00
com02:
	call	prst7		;neue zeile
	defb	0dh,8ah
	call	outhl		;hl ausgeben
	call	outsp
	ld	a,(hl)
	call	outhx		;Inhalt hl
	call	prst7
	defb	" -",a0h
	push	hl		;de und hl tauschen
	push	de
	pop	hl
	call	outhl		;hl (de) ausgeben
	call	outsp
	ld	a,(hl)
	call	outhx		;inhalt hl (de) ausgeben
	pop	hl		;hl wiederherstellen
com03:
	dec	bc
	inc	hl
	inc	de
com04:
	call	inch		;Tastatur lesen
	cp	0dh		;andere Zeichen brechen ab
	jr	nz,com05
	call	inch		;Zeichen 0a vernichten
	jr	com00		;neu 8 Zeilen zeigen
com05:
	call	outch
	call	prst7		;neue zeile
	defb	0dh,8ah
	ret

	; Display - Speicher anzeigen 256 Byte
	; D ANFAD
	; Enter=naechster Block,'1'=Ende
disp:
	ld	hl,(001bh)	;Adresse laden
	ld	a,f0h
	and	l
	ld	l,a
disp01:
	ld	c,10h		;16 Zeilen ausgeben (256Byte)
disp02:
	call	prst7		;neue zeile
	defb	0dh,8ah
	call	outhl		;Adresse ausgeben
	call	outsp
	ld	b,10h		;16 Byte je Zeile
	push	hl		;Adresse retten
disp03:
	ld	a,(hl)
	call	outhx		;Inhalt hex ausgeben
	call	outsp		;Space
	inc	hl
	djnz	disp03
	call	outsp		;2 Leerzeichen
	call	outsp
	ld	b,10h
	pop	hl		;Adresse laden
disp04:
	ld	a,(hl)		;Zeichen 20-7a anzeigen!
	cp	20h		;unter 20 hex?
	jr	nc,disp05
	ld	a,'1'
disp05:
	cp	7bh		;ueber 7a=z ?
	jr	c,disp06
	ld	a,'1'
disp06:
	call	outch
	inc	hl
	djnz	disp04
	dec	c
	jr	nz,disp02
	call	inch		;Tastatur lesen
	cp	0dh		;andere Zeichen brechen ab
	jr	nz,disp07
	call	inch		;Zeichen 0a vernichten
	call	prst7		;Leerzeile
	defb	0dh,8ah
	jr	disp01		;neu 8 Zeilen zeigen
disp07:
	call	outch
	call	prst7		;neue zeile
	defb	0dh,8ah
	ret

	; Ports lesen und schreiben
	; N=naechste,R=zurueck,Enter neu lesen
	; 2 Zeichen Hexzahl beschreibt Port,'1' Ende
port:
	ld	hl,(001bh)	;Portadresse laden
	ld	h,00h		
	ld	d,02h		;2 Zeichenzaehler
port01:
	push	bc
	ld	c,l
	in	a,(c)		;Port lesen
	pop	bc
	call	outhl		;Adresse ausgeben
	call	outsp
	call	outhx		;Portinhalt
	call	prst7
	defb	20h,'1',A0h	; # ausgeben
port015:
	call	inch		;Tastatur lesen
	cp	0dh		;Enter?
	jr	nz,port03
	call	inch		;0a vernichten
port02:
	call	prst7		;neue zeile
	defb	0dh,8ah
	ld	d,02h		;2 Zeichenzaehler
	jr	port01		;aktualisieren
port03:
	cp	'1'		;Ende?
	jr	nz,port04
	call	prst7		;neue zeile
	defb	0dh,8ah
	ret
port04:
	cp	'1'		;zurueck?
	jr	nz,port05
	call	outch		;anzeigen
	dec	l
	jr	port02
port05:
	cp	'1'		;naechstes?
	jr	nz,port06
	call	outch		;anzeigen
	inc	l
	jr	port02
port06:			;ASCII-Zeichentest
	call	outch
	sub	30h
	jr	c,port015	;nicht 0-9,a-f
	cp	0ah
	jr	c,port07	;Sprung bei 0-9
	sub	07h
	cp	0ah
	jr	c,port015	;nicht 0-9,a-f
	cp	10h
	jr	nc,port015	;nicht 0-9,a-f
port07:
	sla	c		;7<- C <-0
	sla	c
	sla	c
	sla	c
	add	a,c
	ld	c,a
	dec	d		;2.Zeichen?
	jr	nz,port015
	ld	d,02h		;nachladen mit 2
	push	bc
	ld	c,l
	out	(c),a		;Port schreiben
	pop	bc
	call	outsp		;Trennzeichen
	jr	port015

help:
	;Befehlshilfe anzeigen
	call	prst7
	defb	0ah,0dh
	defb	"B XXXX           Breakpoint set to XXXX",0ah,dh
	defb	"C XXXX YYYY ZZZZ Compare XXXX with YYYY length ZZZZ",0ah,dh
	defb	"D XXXX           Display Memory from XXXX",0ah,dh
	defb	"E XXXX           Execute with Breakpoint",0ah,dh
	defb	"F XXXX YYYY ZZ   Fill Memory XXXX to YYYY with ZZ",0ah,dh
	defb	"G XXXX           Go to next Breakpoint",0ah,dh
	defb	"H                Help display this page",0ah,dh
	defb	"I                Init Monitor Memory and Stack",0ah,dh
	defb	"J XXXX           Jump without Breakpoint",0ah,dh
	defb	"L                Load INTELHEX File to RAM",0ah,dh 
	defb	"M XXXX           Memory modify RAM",0ah,dh
	defb	"N                Next Step (Start with E !) ",0ah,dh
	defb	"P XX             Port IN/OUT read/write",0ah,dh
	defb	"R XX             Register modify (Example R AF)",0ah,dh
	defb	"S                Next Step with simple ENTER Key",0ah,dh
	defb	"T XXXX YYYY ZZZZ Transfer XXXX to YYYY length ZZZZ",0ah,dh
	defb	0ah,8dh
	ret

snext:
	;Spezialbefehle für Next in Schleife
	;mit Enter Befehl widerholen
	call	next
	call	inch		;Tastatur lesen
	cp	0dh		;andere Zeichen brechen ab
	jr	nz,snext1
	call	inch		;Zeichen 0a vernichten
	call	prst7		;Leerzeile
	defb	0dh,8ah
	jr	snext		;naechstes Next
snext1:
	call	outch
	call	prst7		;neue zeile
	defb	0dh,8ah
	ret

inkey:
	;Tastatur einlesen, ist kein
	;Zeichen da = Akku A 00
	ld	a,00h
	EXBIOS CONIO, PORTOUT, CONSTA	;out	(sioas),a
	EXBIOS CONIO, PORTINP, CONSTA ;in	a,(sioas)
	and	00000001b
	ret	z
	EXBIOS CONIO, PORTINP, CONDAT	;in	a,(sioad)
	ret

inch:
	;auf Zeichen von sio a
	;warten -> Akku A
	ld      a,00h
	EXBIOS CONIO, PORTOUT, CONSTA ;out     (sioas),a
	EXBIOS CONIO, PORTINP, CONSTA ;in      a,(sioas)
	and     00000001b
	jr      z,inch
	EXBIOS CONIO, PORTINP, CONDAT	;in      a,(sioad)
	ret

inhex:
	;DE zeigt auf Anfangsadresse einer
	;maximal 4 stelligen Zeichenkette
	;Ergebnis der letzten 4 Zeichen
	;steht dann in HL
	push	bc
	call	inhex10
	ld	b,h
	ld	c,l
	ld	l,(hl)
	inc	bc
	ld	a,(bc)
	ld	h,a
	or	l
	pop	bc
	ret
inhex10:
	call	inc_de_nospace
	xor	a
	ld	hl,0013h
	ld	(hl),a
	inc	hl
	ld	(hl),a
inhex11:
	ld	a,(de)
	dec	hl
	sub	30h
	ret	m
	cp	0ah
	jr	c,inhex12
	sub	07h
	cp	0ah
	ret	m
	cp	10h
	ret	p
inhex12:
	inc	de
	rld
	inc	hl
	rld
	jr	inhex11

inc_de_nospace:
	;DE solange erhoehen, bis Inhalt
	;von (DE) nicht Space ist
	ld	a,(de)
	cp	20h
	ret	nz
	inc	de
	jr	inc_de_nospace

zeiloesch:
	;Eingabezeile loeschen
	;Cursor und Zeilenadresse setzen
	push af
	push hl
	ld	a,20h		;32 Zeichen loeschen
	ld	hl,00e0h	;Eingabezeile RAM !	
zeiloesch1:
	ld	(hl),20h
	inc	hl
	dec	a
	jr	nz,zeiloesch1
	ld	hl,00e0h
	ld	(002bh),hl
	ld	(0016h),hl
	pop	hl
	pop	af
	ret

outch:
	;Zeichen in A ueber sio a 
	;ausgeben
	push	af
outch1:
	ld	a,00h
	EXBIOS CONIO, PORTOUT, CONSTA ;out	(sioas),a
	EXBIOS CONIO, PORTINP, CONSTA ;in	a,(sioas)
	and	00000100b
	jr	z,outch1
	pop	af
	EXBIOS CONIO, PORTOUT, CONDAT	 ;out	(sioad),a
	ret

outhx:
	;Akku A als 2 Byte hex 
	;ausgeben
	push	af
	and	f0h
	srl	a
	srl	a
	srl	a
	srl	a
	call	binhex
	call	outch
	pop	af
	and	0fh
	call	binhex
	call	outch
	ret

outhl:
	;Register HL mit 4 Byten
	;hex ausgeben
	push	af
	ld	a,h
	call	outhx
	ld	a,l
	call	outhx
	pop	af
	ret

othls:
	;Adresse von von HL in
	;2 Byten (4Z) hex ausgeben
	;dann ein Leerzeichen
	push	af
	ld	a,(hl)
	call	outhx
	dec	hl
	ld	a,(hl)
	call	outhx
	dec	hlv
	ld	a,20h
	call	outch
	pop	af
	ret

outsp:
	;ein Space ausgeben
	push	af
	ld	a,20h
	call	outch
	pop	af
	ret

outdp:
	;einen Doppelpunkt ausgeben
	push	af
	ld	a,3Ah
	call	outch
	pop	af
	jr	othls

prst7:
	;Bytekette bis Bit 7 wird
	;ausgegeben
	;0a Zeilenanfang
	;0c Bildschirm loeschen
	;0d Zeile runter 
	ex	(sp),hl
prst71:
	ld	a,(hl)
	inc	hl
	push	af
	and	7fh
	call	outch
	pop	af
	bit	7,a
	jr	z,prst71
	ex	(sp),hl
	ret

inlin:
	;Zeile mit Promtsysmbol und
	;Zeichenkette bis Enter
	call	prst7
	defb	20h,23h,a0h
	jr	instr

instr:
	;Zeichenkette angefordert mit
	;Enter als Endezeichen
	push	hl
	call	zeiloesch
	ld	hl,00e0h
instr1:
	call	inch
	call	outch
	ld	(hl),a
	inc	hl
	cp	0dh
	jr	nz,instr1
	call	inch
	call	outch
	pop	hl
	ret

init_ctc_sio:
	;Initialisierung
	;CTC0 VT=16, ZK=1
	;SIOA 9600 8,N,1 VT=16
	ld	a,00000101b
;	out	(ctc0),a
	ld	a,00000001b
;	out	(ctc0),a
	ld	b,08h
	ld	hl,sioini
ini1:
	ld	a,(hl)
	EXBIOS CONIO, PORTOUT, CONSTA ;out	(sioas),a
	inc	hl
	djnz	ini1
	ret

	;Programm Download
	;INTEL HEX-Datei einlesen
down:
	call	leseb		;Byte lesen
	cp	'1'		;Startzeichen
	jr	nz,down
	call	lesedb		;Doppelbyte Anzahl
	cp	00h
	jr	nz,down1
down0:
	call	prst7		;Zeilenschaltung
	defb	0ah,8dh
	ret
down1:
	ld	d,a		;Anzahl
	call	lesedb
	ld	h,a		;Adresse high
	call	lesedb
	ld	l,a		;Adresse low
	call	lesedb		;verwerfen
down2:
	call	lesedb
	ld	(hl),a		;speichern
	inc	hl
	dec	d
	jr	nz,down2
	ld	a,'1'		;jede Zeile ein #
	call	outch
	jr	down

	;UP Byte lesen
leseb:
	ld	a,00h
	EXBIOS CONIO, PORTOUT, CONSTA ;out	(sioas),a
	EXBIOS CONIO, PORTINP, CONSTA ;in	a,(sioas)
	and	01h
	jr	z,leseb
	EXBIOS CONIO, PORTINP, CONDAT	;in	a,(sioad)
	ret

	;UP Doppelbyte lesen

lesedb:
	call	leseb
	call	hexbin
	and	0fh
	add	a,a
	add	a,a
	add	a,a
	add	a,a
	ld	b,a
	call	leseb
	call	hexbin
	add	a,b
	ret

	;UP Hex -> Bin

hexbin:
	cp	'1'
	jr	c,hexbin1
	add	a,09h
hexbin1:
	and	0fh
	ret	

	;UP Bin -> Hex

binhex:
	cp	0ah
	jr	nc,binhex1
	add	a,30h
	ret
binhex1:
	add	a,37h
	ret

sioini:
	defb	04h,44h,01h,00h,03h,c1h,05h,68h

test:
	LD 	HL,0000h	;kompl RAM loeschen
	LD 	DE,0001h
	LD 	(HL),55h
	LD 	BC,8000h
	LDIR
	ret			;abgestürzt!

	;ISR Tabelle (ACHTUNG ! auf Adressen achten )
	;hier ist Adresse 089B0 fuer die CTC Kanal 0
	dw isr
endif ; ifdef CPM22	
	END	