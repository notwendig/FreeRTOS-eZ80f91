
; eZ80 (Z80 ADL=1 Mixed-mode) trap to monito
; See https://github.com/notwendig/FreeRTOS-eZ80f91

	org 100h

	db 0cbh, 30h, 0, 0
	jp	0
	end
