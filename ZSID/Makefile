SHELL=/bin/bash

CPMFILES=./RAMDSK

AS=zmac
CAT=cat

ASDIR=./ZSID
ASRCS= SIDRELO.ASM
ACIMS=$(ASRCS:.ASM=.CIM)

AFLAGS=-L -m -z --oo CIM,LST -I $(ASDIR) -I ../CPM/CPM22

vpath %.INC $(ASDIR) 
vpath %.ASM	$(ASDIR)

all: minimon.c

# Compile Z80-Assembler to Z80-Binary
%.CIM:%.ASM
	$(AS) $(AFLAGS) -o $@ -o $(@:.CIM=.LST) $<

minimon.c:$(ACIMS)
	../uzlib/examples/tgzip/tgzip $< $(basename $@).z
	srec_cat $(basename $@).z -binary -output $@ -C-Array minimonz -INClude		

clean:
	-rm *.CIM *.LST minimon.[chz]
