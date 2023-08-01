
	.include "eZ80F91.inc"
	.include "intvect.inc"



	xdef _vPortYieldFromTick
    xdef _xTickCount
    xdef _xTickCountHeigh

	xref _set_vector
	xref _pxCurrentTCB
	xref _xTaskIncrementTick
	xref _vTaskSwitchContext
	xref _vTaskSetThreadLocalStoragePointer
	xref _criticalcounter

    segment data

_xTickCount:      dw24    0
_xTickCountHeigh: dw24    0

	segment code
	.assume adl=1
;UINT32 portFreeRTOS_htonl( UINT32 ulIn ) 											
;      L H U E    L H U E
; ix+6 a.b.c.d -> d.c.b.a
	xdef _portFreeRTOS_htonl
_portFreeRTOS_htonl:
		push	ix
		ld		ix,0
		add		ix,sp
		ld		hl,(ix+5)
		ld		e,(ix+6)
		ld		h,(ix+8)
		ld		l,(ix+9)
		pop		ix
		ret
;UINT16 portFreeRTOS_htons( UINT16 usIn ) 											
	xdef _portFreeRTOS_htons
_portFreeRTOS_htons:
		push	ix
		ld		ix,0
		add		ix,sp	
		ld		h,(ix+6)
		ld		l,(ix+7)
		pop		ix
		ret
		
tasktrap:
$$:	nop
	jr		$B

	xdef switch
	xdef _firstTASK

 _vPortYieldFromTick:
	in0		a,(TMR0_IIR)
	or      a,a
	sbc     hl,hl
    ld      de,(_xTickCount)
    ex      de,hl
    scf
    adc     hl,de
    ld      (_xTickCount),hl
    ld      hl,(_xTickCountHeigh)
    adc     hl,de
    ld      (_xTickCountHeigh),hl
	call	_xTaskIncrementTick
	or		a,a
	adc		hl,hl
	ret		z
	nop
switch:
	di
	ld		a,(_criticalcounter)
	push	af
	LD		IX,	0
	ADD		IX,	SP
	LD		HL,	(_pxCurrentTCB)
	LD		(HL),IX
	call	_vTaskSwitchContext
_firstTASK:
	LD		HL,	(_pxCurrentTCB)
	LD		HL,	(HL)
	LD		SP,	HL
	pop		af
	ld		(_criticalcounter),a
$$:	ret

	END
