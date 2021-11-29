
	.include "eZ80F91.inc"
	.include "intvect.inc"

	xdef _shiftDisplay

	xref _dir
	xref _dply
	xref _set_vector

	segment code
	.assume adl=1

; Shift display buffer
; void shiftDisplay()
_shiftDisplay:
		push	bc
		push	de
		push	hl
		push	iy
			
		ld		b,7
		ld		de,0
		ld		hl, dirtbl
		
		ld		a,(_dir)
		ld		e,a
		add		a,a
		adc		a,e
		ld		e,a
		add		hl,de
		ld		iy,_dply
		ld		hl,(hl)
		jp		(hl)		

shift_none:
		ld		a,(iy+1)
		ld		(iy),a
		lea		iy,iy+2
		djnz	shift_none
		jr		shift_ret

shift_left:
		rl		(iy+1)
		rl		(iy)
		lea 	iy,iy+2
		djnz 	shift_left
		jr		shift_ret

shift_up:
		ld		c,(iy+1)
		inc		b
$$:		ld		a,(iy+2)
		ld		(iy),a
		ld		a,(iy+3)
		ld		(iy+1),a
		lea		iy,iy+2
		djnz	$B
		ld		(iy-2),c
		jr	shift_ret

shift_right:
		rr		(iy+1)
		rr		(iy)
		lea 	iy,iy+2
		djnz 	shift_right
		jr		shift_ret

shift_down:
		lea		iy,iy+14
		ld		c,(iy+1)
		inc		b
$$:		ld		a,(iy-2)
		ld		(iy),a
		ld		a,(iy-1)
		ld		(iy+1),a
		lea		iy,iy-2
		djnz	$B
		ld		(iy+2),c
		
shift_ret:
		pop		iy
		pop		hl
		pop		de
		pop		bc
		ret
	segment text
	
		.align 16
dirtbl:	dw24 	shift_none
		dw24	shift_left
		dw24	shift_up
		dw24	shift_right
		dw24	shift_down
	end
