;*****************************************************************************
; vectors24.asm
;
; eZ80's Reset, RST and 2nd generation interrupt arrangement
;*****************************************************************************
; Copyright (C) 2005 by ZiLOG, Inc.  All Rights Reserved.
;*****************************************************************************

        XREF __init
		XDEF _intrap
		XDEF _clrtrap
        XDEF _reset
        XDEF __default_nmi_handler
        XDEF __default_mi_handler
        XDEF __nvectors
        XDEF _init_default_vectors
        XDEF __init_default_vectors
        XDEF _set_vector
        XDEF __set_vector
        XDEF __vector_table
		

NVECTORS EQU 64                ; number of potential interrupt vectors

; Save Interrupt State
SAVEIMASK MACRO
    ld a, i                    ; sets parity bit to value of IEF2
    push af
    di                         ; disable interrupts while loading table 
    MACEND

; Restore Interrupt State
RESTOREIMASK MACRO
    pop af
    jp po, $+5                 ; parity bit is IEF2
    ei
    MACEND


;*****************************************************************************
; Reset and all RST nn's
;  1. diaable interrupts
;  2. clear mixed memory mode (MADL) flag
;  3. jump to initialization procedure with jp.lil to set ADL
        DEFINE .RESET, SPACE = ROM
        SEGMENT .RESET
		.assume adl=0
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
_reset:
_rst0:
    ;di
    ;rsmix
	jp.lil __trap
	ds	4
_rst8:
    di
    rsmix
    jp.lil __init
_rst10:
    di
    rsmix
    jp.lil __init
_rst18:
    di
    rsmix
    jp.lil __init
_rst20:
    di
    rsmix
    jp.lil __init
_rst28:
    di
    rsmix
    jp.lil __init
_rst30:
    di
    rsmix
    jp.lil __init
_rst38:
    di
    rsmix
    jp.lil __init
    DS %26
_nmi:
    jp.lil __default_nmi_handler


;*****************************************************************************
; Startup code
        DEFINE .STARTUP, SPACE = ROM
        SEGMENT .STARTUP       ; This should be placed properly
        .ASSUME ADL=1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; number of vectors supported
__nvectors:
        DW NVECTORS            ; extern unsigned short _num_vectors;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Default Non-Maskable Interrupt handler
__default_nmi_handler:
    retn.l

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Default Maskable Interrupt handler
__default_mi_handler:
    ei
    reti.l

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Initialize all potential interrupt vector locations with a known
; default handler.
;
; void _init_default_vectors(void);
__init_default_vectors:
_init_default_vectors:
    push af
    SAVEIMASK
    ld hl, __default_mi_handler
    ld a, 0
    ld (__vector_table), hl    ; load default maskable irq handler
    ld (__vector_table + 3), a ; load a one byte filler
    ld hl, __vector_table
    ld de, __vector_table + 4
    ld bc, NVECTORS * 4 - 4
    ldir
    im 2                       ; interrtup mode 2
    ld hl, ivector >> 8
    ld i, hl                   ; load interrupt vector base
    RESTOREIMASK
    pop af
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Installs a user interrupt handler in the vector table
;
; void * _set_vector(unsigned int vector, void(*handler)(void));
__set_vector:
_set_vector:
    push iy
    ld iy, 0                   ; standard prologue
    add iy, sp
    push af
    SAVEIMASK
    ld hl, (iy+6)              ; load vector offset
    ld bc, __vector_table      ; load base address for vector table
    add hl, bc                 ; calculate vector location
    ld bc, (iy+9)              ; handler
    ld de, (hl)                ; save previous handler
    ld (hl), bc                ; store new vector address
    push de
    pop hl                     ; return previous handler
    RESTOREIMASK
    pop af
    ld sp, iy                  ; standard epilogue
    pop iy
    ret

	
;*****************************************************************************
; This segment must be aligned on a 512 byte boundary anywhere in RAM
; Each entry will be a 3-byte address in a 4-byte space 
DEFIVECT	macro	inum
		SEGMENT .IVECTS
		dw24	_i&inum
		db		0
		SEGMENT TRAMP
		.assume adl=1
_i&inum:push	ix
		ld		ix,(__vector_table + inum * 4)
		call	irq
		pop		ix
		ei
		reti.l
		endmacro DEFIVECT

        DEFINE .IVECTS, SPACE = ROM, ALIGN = 200h
		SEGMENT .IVECTS
ivector:
	
        DEFINE TRAMP, SPACE = ROM, ALIGN = 2h
		SEGMENT TRAMP
		.assume adl=1
		
		DEFIVECT	0
		DEFIVECT	1
		DEFIVECT	2
		DEFIVECT	3
		DEFIVECT	4
		DEFIVECT	5
		DEFIVECT	6
		DEFIVECT	7
		DEFIVECT	8
		DEFIVECT	9
		
		DEFIVECT	10
		DEFIVECT	11
		DEFIVECT	12
		DEFIVECT	13
		DEFIVECT	14
		DEFIVECT	15
		DEFIVECT	16
		DEFIVECT	17
		DEFIVECT	18
		DEFIVECT	19
		
		DEFIVECT	20
		DEFIVECT	21
		DEFIVECT	22
		DEFIVECT	23
		DEFIVECT	24
		DEFIVECT	25
		DEFIVECT	26
		DEFIVECT	27
		DEFIVECT	28
		DEFIVECT	29
		
		DEFIVECT	30
		DEFIVECT	31
		DEFIVECT	32
		DEFIVECT	33
		DEFIVECT	34
		DEFIVECT	35
		DEFIVECT	36
		DEFIVECT	37
		DEFIVECT	38
		DEFIVECT	39
		
		DEFIVECT	40
		DEFIVECT	41
		DEFIVECT	42
		DEFIVECT	43
		DEFIVECT	44
		DEFIVECT	45
		DEFIVECT	46
		DEFIVECT	47
		DEFIVECT	48
		DEFIVECT	49
		
		DEFIVECT	50
		DEFIVECT	51
		DEFIVECT	52
		DEFIVECT	53
		DEFIVECT	54
		DEFIVECT	55
		DEFIVECT	56
		DEFIVECT	57
		DEFIVECT	58
		DEFIVECT	59
		
		DEFIVECT	60
		DEFIVECT	61
		DEFIVECT	62
		DEFIVECT	63
	
		SEGMENT code
		.assume adl=1
_clrtrap:
	call	_intrap	
	xor		a,a
	ld		(__trapflg),a
	ret
	
_intrap:
	ld		hl,0
	ld		a,(__trapflg)
	ld		l,a
	ret
	
	
	xref	_trap
	xdef	__trap
__trap:	
	ld		(trapsp),sp
	ld		sp,trapsp
	push	af
	ld		a,i
	di
	push	af
	push	bc
	push	de
	push	hl
	ld		hl,0
	ld		de,(trapsp)
	or		a,a
	adc		hl,de
	jr		nz,$F
	stmix
	jp		__init

$$:	ld		a,(__trapflg)
	or		a
	jr		z,$F
	halt
	
$$:	dec		a
	ld		(__trapflg),a
	inc		de
	push	de		;
	push	ix
	push	iy
	ex		af,af'
	exx
	push	af
	push	bc
	push	de
	push	hl
	ld 		hl,0
	add.s	hl,sp
	push	hl
	ld		hl,0
	add		hl,sp
	push	hl
	call	_trap
	pop		hl
	pop		hl
	ld.s	sp,hl
	pop		hl
	pop		de
	pop		bc
	pop		af
	exx
	ex		af,af'
	pop		iy
	pop		ix
	pop		hl
	pop		hl
	pop		de
	pop		bc
	xor		a,a
	ld		(__trapflg),a
	pop		af
	jp		po,$F
	pop		af
	ld		sp,(trapsp)
	ei
	ret.l
$$:	pop		af
	ld		sp,(trapsp)
	ret.l
	

	
	xref switch
	xdef _vPortYield
_vPortYield:
	push	ix
	ld		ix,switch
	ld		a,i
	call	irq
	pop		ix
	ret		po
	ei
	ret
	
irq:push	af
	push	bc
	push	de
	push	hl
	;push	ix
	push	iy
	ex		af,af'
	exx
	push	af
	push	bc
	push	de
	push	hl
	call	$gotohl
	pop		hl
	pop		de
	pop		bc
	pop		af
	exx
	ex		af,af'
	pop		iy
	;pop		ix
	pop		hl
	pop		de
	pop		bc
	pop		af
	ret
	
$gotohl:
	jp		(ix)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	DEFINE RIVECTS, SPACE = RAM, ALIGN = 4h
    SEGMENT RIVECTS
		
__vector_table:
        DS NVECTORS * 4
__trapflg	db 0

	SEGMENT bss
			ds 1024
			ds 3;		sps
			ds 3;		hl'
			ds 3;		de'
			ds 3;		bc'
			ds 3;		af'
			ds 3;		iy
			ds 3;		ix
			ds 3;		pc
			ds 3;		hl
			ds 3;		de
			ds 3;		bc
			ds 3;		iff
			ds 3;		af
trapsp:		ds 3;		spl

	END
