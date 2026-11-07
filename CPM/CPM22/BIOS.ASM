ifdef CPM22
; CP/M 2.2 BIOS: direct hardware ports, shared image/directory drives.
    include CPM.INC
    include HWC.INC
    XDEF BOOT,WBOOT,CONST,CONIN,CONOUT,LISTN,PUNCH,READER
    XDEF HOME,SELDSK,SETTRK,SETSEC,SETDMA,READ,WRITE,LISTST,SECTRN
    XDEF HOSTBDOS,HOSTDISPATCH
    XREF FBASE1

    define _BIOS,space=ram,org=BIOS
    segment _BIOS
    .assume adl=0
BOOT:   JP _BOOT
WBOOT:  JP _WBOOT
CONST:  JP _CONST
CONIN:  JP _CONIN
CONOUT: JP _CONOUT
LISTN:  JP _CONOUT
PUNCH:  JP _CONOUT
READER: JP _CONIN
HOME:   JP _HOME
SELDSK: JP _SELDSK
SETTRK: JP _SETTRK
SETSEC: JP _SETSEC
SETDMA: JP _SETDMA
READ:   JP _READ
WRITE:  JP _WRITE
LISTST: LD A,0FFh
        RET
SECTRN: LD H,B
        LD L,C
        RET

_BOOT:
    LD SP,080h
    XOR A
    OUT (HFSCMD),A
    LD A,MB
    OUT (HFSDAT),A
    LD HL,CCP
    LD A,L
    OUT (HFSDAT),A
    LD A,H
    OUT (HFSDAT),A
    XOR A
    OUT (HFSDAT),A
    LD A,8
    OUT (HFSDAT),A
    LD A,9
    OUT (HFSCMD),A       ; Snapshot the 2-KiB CCP for warm starts.
    XOR A
    LD (IOBYTE),A
    LD (CDISK),A
    JR _GOCPM
_WBOOT:
    LD SP,080h
    LD A,10
    OUT (HFSCMD),A       ; Restore CCP without reading disk sectors.
_GOCPM:
    LD A,0C3h
    LD (0),A
    LD HL,WBOOT
    LD (1),HL
    LD (5),A
    LD HL,BDOS
    LD (6),HL
    LD C,26
    LD DE,080h
    CALL HOSTBDOS
    LD A,(CDISK)
    LD C,A
    JP CCP

_CONST:
    IN A,(HCONSTA)
    AND 1
    RET Z
    LD A,0FFh
    RET
_CONIN:
    CALL _CONST
    OR A
    JR Z,_CONIN
    IN A,(HCONDAT)
    RET
_CONOUT:
    LD A,C
    OUT (HCONDAT),A
    RET
_SELDSK:
    LD A,C
    LD (HOSTDRIVE),A
    PUSH BC
    PUSH DE
    XOR A
    OUT (HFSCMD),A
    LD A,C
    OUT (HFSDAT),A
    LD A,11
    OUT (HFSCMD),A
    IN A,(HFSCMD)
    OR A
    LD HL,0
    JR NZ,_SELECTED
    LD HL,HOSTDPH
_SELECTED:
    POP DE
    POP BC
    RET
_SETDMA:
    LD (HOSTDMA),BC
    PUSH BC
    PUSH DE
    LD D,B
    LD E,C
    LD C,26
    CALL HOSTBDOS
    POP DE
    POP BC
    RET
_HOME:
    LD BC,0
_SETTRK:
    LD (HOSTTRACK),BC
    RET
_SETSEC:
    LD A,C
    LD (HOSTSECTOR),A
    RET
_READ:
    XOR A
    JR _FDCIO
_WRITE:
    LD A,1
_FDCIO:
    LD (HOSTWRITE),A
    PUSH BC
    PUSH DE
    PUSH HL
    LD A,(HOSTDRIVE)
    OUT (FDCDRV),A
    LD A,(HOSTTRACK+1)
    OUT (FDCTHI),A
    LD A,(HOSTTRACK)
    OUT (FDCDATA),A
    LD A,010h
    OUT (FDCSTAT),A
    IN A,(FDCSTAT)
    AND 098h
    JR NZ,_FDCERROR
    LD A,(HOSTSECTOR)
    OUT (FDCSEC),A
    LD A,(HOSTWRITE)
    OR A
    LD A,080h
    JR Z,_FDCCOMMAND
    LD A,0A0h
_FDCCOMMAND:
    OUT (FDCSTAT),A
    LD HL,(HOSTDMA)
    LD B,128
_FDCBYTE:
    IN A,(FDCREQ)
    AND 1
    JR Z,_FDCERROR
    LD A,(HOSTWRITE)
    OR A
    JR NZ,_FDCWRBYTE
    IN A,(FDCDATA)
    LD (HL),A
    JR _FDCNEXT
_FDCWRBYTE:
    LD A,(HL)
    OUT (FDCDATA),A
_FDCNEXT:
    INC HL
    DJNZ _FDCBYTE
    IN A,(FDCSTAT)
    AND 0D8h
    JR NZ,_FDCERROR
    XOR A
    JR _FDCRETURN
_FDCERROR:
    LD A,1
_FDCRETURN:
    POP HL
    POP DE
    POP BC
    RET

; C=function, DE=FCB or argument. Return HL and A=L, B=H as CP/M BDOS does.
; The port packet also supplies MBASE and BIOS-owned scratch space.
HOSTDISPATCH:
    LD A,C
    CP 13
    JP C,FBASE1
    CP 41
    JP NC,FBASE1
HOSTBDOS:
    XOR A
    OUT (HFSCMD),A
    LD A,C
    OUT (HFSDAT),A
    LD A,E
    OUT (HFSDAT),A
    LD A,D
    OUT (HFSDAT),A
    LD A,MB
    OUT (HFSDAT),A
    LD HL,HOSTSCRATCH
    LD A,L
    OUT (HFSDAT),A
    LD A,H
    OUT (HFSDAT),A
    LD A,1
    OUT (HFSCMD),A
    IN A,(HFSDAT)
    LD L,A
    IN A,(HFSDAT)
    LD H,A
    LD A,L
    LD B,H
    RET

HOSTDPH:
    DW 0,0,0,0,HOSTDIR,HOSTDPB,0,HOSTSCRATCH
HOSTDPB:
    DW 32
    DB 4,15,0
    DW 2047,255
    DB 240,0
    DW 0,0
HOSTDIR: DS 128
HOSTSCRATCH: DS 272
HOSTDRIVE: DB 0
HOSTTRACK: DW 0
HOSTSECTOR: DB 1
HOSTDMA: DW 080h
HOSTWRITE: DB 0
endif
    END
