	.include "ez80F91.inc"
	
	segment CODE
	.assume	adl=1

	xref _xTaskGenericNotifyFromISR
	xref _tty
	
eIncrement equ 2

	xdef _ttyisr
	
_ttyisr:
	PUSH	AF
	PUSH	BC
	PUSH	DE
	PUSH	HL		
	PUSH	IX
	PUSH	IY
	
	ld	bc, UART0_IIR
	in	a, (bc)
	rla
	rla
	rla
	rla
	and	a, 0F0h
	ld	d, a			; 0-3 to 4-7 Interrupt status code 

	ld	bc, UART0_IER
	in	a, (bc)
	or	a, d
	ld	d, a
	xor	a, a
	out	(bc), a
	ld	bc, UART0_SPR
	ld	a, d
	out	(bc), a
	sbc	hl,hl
	push hl  			; pxHigherPriorityTaskWoken ignored
	push hl				; pulPreviousNotifyValue ignored
	ld	 de,eIncrement
	push de				; eAction
	push hl				; ulValue is ignored
	push hl
	push hl				; value	ignored
	ld   hl, (_tty+22)
	PUSH hl				; task
	call _xTaskGenericNotifyFromISR
	pop	 hl
	pop	 hl
	pop	 hl
	pop	 hl
	pop	 hl
	pop	 hl
	pop	 hl
	POP	IY
	POP IX
	POP	HL
	POP	DE
	POP	BC
	POP	AF 
	RET
	
end
