	.include "cpm.inc"
	
	SEGMENT code
	.assume adl=1
	
	xref __low_romboot
	xref __low_boot
	xref __len_boot

	xref __low_romccp
	xref __low_ccp
	xref __len_ccp

	xref __low_rombdos
	xref __low_bdos
	xref __len_bdos

	xref __low_rombios
	xref __low_bios
	xref __len_bios

	xdef _loadcpm ; char* loadcpm(char *mem)
_loadcpm:
	push	ix
	ld		ix,0
	add		ix,sp
	
	ld		hl,(ix+6)
	ld  	de, __low_ccp
	add		hl,de
	ex		de,hl
	ld		hl,	__low_romccp
	ld		bc, __len_ccp
	ldir

	ld		hl,(ix+6)
	ld  	de, __low_bdos
	add		hl,de
	ex		de,hl
	ld		hl,	__low_rombdos
	ld		bc, __len_bdos
	ldir

	ld		hl,(ix+6)
	ld  	de, __low_bios
	add		hl,de
	push	hl
	ex		de,hl
	ld		hl,	__low_rombios
	ld		bc, __len_bios
	ldir
	ld		a,(ix+8)
	ld  	mb,a
	pop		hl		
	pop		ix
	ret
	
	xdef _CPM22Task;void CPM22Task(char* mem) 
_CPM22Task:
	push	ix
	ld		ix,0
	add		ix,sp
ifdef RDSK
	ld de, _BOOTSTRAP							
	ld hl, (ix+6)								
	ld bc,80h									
	add hl,bc									
	ld bc, _BOOTSTRAPEND-_BOOTSTRAP				
	ex de,hl									
	ldir										
	ld a,(ix+8)									
	ld mb,a										
	jp.s 080h									
else
	ld hl, (ix+6)
	push	hl
	call	_loadcpm
	pop		hl
	jp.s	__low_bios
endif	
	pop		ix
	ret
	
	.assume adl=0
_BOOTSTRAP:										
	XOR	A					; Drive A			
	LD  HL,0				; def IBM 3740		
	EXBIOS FDIO, 0, FDCD	; selct drive		
	EXBIOS FDIO, 1, FDCST	; get status		
	OR	A 					; selected?			
	JR	Z,LSEC				; yes try loading	
EXMON:											
	EXBIOS ROMBOOT, 0, 0	; no disk run monitor
LSEC: 											
 	LD	BC,0									
	EXBIOS DMAIO, 0, DMABC	; set DMA			
	EXBIOS FDIO, 0, FDCTBC	; set track 0		
	INC C 										
	EXBIOS FDIO, 0, FDCSBC	; set sector 1		
	XOR A 					; read cmd			
	EXBIOS FDIO, 0, FDCOP	; execut FD-command	
	EXBIOS FDIO, 1, FDCST	; get status		
	OR	A 					; if successful?	
	JR	NZ,EXMON			; no go monitor		

	LD  HL,7Fh              ; end of boot code  
	LD  BC,_BOOTSTRAPEND-ID ; size of bl ident  
	LD  DE,_BOOTSTRAPEND-_BOOTSTRAP +7Fh  ; start	
CMP:                                           	
	LD  a,(DE)                                  
	DEC DE                                      
	CPD                                         
	JR  NZ,EXMON                                
	JP  PO,0                                   	
	JR  CMP      
ID:	DB  'EZ80F91',0
_BOOTSTRAPEND: 									
	.ASSUME ADL=1
