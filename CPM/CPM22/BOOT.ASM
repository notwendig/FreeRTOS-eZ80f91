ifdef CPM22
    include CPM.INC
    define _BOOT,space=ram,org=0
    segment _BOOT
    .assume adl=0
    DI
    JP BOOT
endif
    END
