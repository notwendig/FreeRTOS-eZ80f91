; CP/M 2.2 hardware I/O map. Peripherals decode the low eight port bits.
; IN/OUT (C) still addresses the complete BC register on the CPU.
HCONSTA EQU 08Ch             ; bit 0: input ready, bit 1: output ready
HCONDAT EQU 08Dh             ; console character
HFSCMD  EQU 08Eh             ; filesystem command / status
HFSDAT  EQU 08Fh             ; filesystem packet stream
FDCBASE EQU 018h
FDCSTAT EQU FDCBASE          ; command / status
FDCTRK  EQU FDCBASE+1        ; track low
FDCSEC  EQU FDCBASE+2        ; sector
FDCDATA EQU FDCBASE+3        ; data / seek destination
FDCDRV  EQU FDCBASE+4        ; drive
FDCREQ  EQU FDCBASE+5        ; DRQ / INTRQ
FDCTHI  EQU FDCBASE+6        ; track high
