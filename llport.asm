
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
