; CAMEL24 direct-port console; wait for input without starving other tasks.
    INCLUDE HWC.INC
    XDEF camel_putc, camel_getc, camel_kbhit
    XREF _vTaskDelay
    SEGMENT CODE
    .ASSUME ADL=1
camel_putc:
    OUT (HCONDAT),A
    LD A,1
    OR A
    RET
camel_kbhit:
    IN A,(HCONSTA)
    AND 1
    RET
camel_getc:
    CALL camel_kbhit
    JR NZ,camel_read
    CALL camel_wait
    JR camel_getc
camel_read:
    IN A,(HCONDAT)
    RET
camel_wait:
    ; C may clobber registers that the Forth interpreter keeps live.
    PUSH AF
    PUSH BC
    PUSH DE
    PUSH HL
    PUSH IX
    PUSH IY
    EXX
    PUSH BC
    PUSH DE
    PUSH HL
    EXX
    EX AF,AF'
    PUSH AF
    EX AF,AF'
    LD HL,1
    PUSH HL
    CALL _vTaskDelay
    POP HL
    EX AF,AF'
    POP AF
    EX AF,AF'
    EXX
    POP HL
    POP DE
    POP BC
    EXX
    POP IY
    POP IX
    POP HL
    POP DE
    POP BC
    POP AF
    RET
    END
