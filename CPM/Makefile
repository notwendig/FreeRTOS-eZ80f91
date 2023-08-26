
SHELL=/bin/bash

CPMFILES:=$(wildcard ./RAMDSK/*)
AFLAGS=-define:_EZ80ACCLAIM!=1 -define:CPM22 \
		-list -NOlistmac -name -pagelen:0 -pagewidth:132 -quiet -sdiopt \
		-warn -debug -NOigcase -cpu:eZ80F91

ZRUN=WINEDEBUG=-all wine ~/ZiLOG/ZDSII_eZ80Acclaim!_5.3.5/bin
AS=$(ZRUN)/ez80asm
LN=$(ZRUN)/ez80link
CAT=cat

ASDIR=./CPM22
ASRCS=BOOT.ASM CCP.ASM BDOS.ASM BIOS.ASM
AOBJS=$(addprefix $(ASDIR)/,$(ASRCS:%.ASM=%.obj))

current_dir := $(shell pwd)

vpath %.ASM $(ASDIR)
vpath %.obj $(ASDIR)
vpath %.INC $(ASDIR)

all: CPM22_A.c relist test

# Compile Z80-Assembler to Z80-Binary
%.obj:%.ASM CPM.INC Makefile CPM22.linkcmd
	@$(AS) $(AFLAGS) $<

# Concatinate all CP/M Binarys (BOOT,CCP,BDOS,BIOS)
# to system-track (52 sectors) imgage
CPM.SYS:$(AOBJS)
	@$(LN) @CPM22.linkcmd
	srec_cat CPM22.hex -Intel -Output $@ -Binary

A.DSK:CPM.SYS #$(CPMFILES)
	@dd if=/dev/zero bs=128 count=2002 of=$@
	@mkfs.cpm -f ibm-3740 -b $< $@
	@cpmcp -f ibm-3740 $@ $(CPMFILES) 0:
	@md5sum $@ > $@.md5

A.UZ:A.DSK
	@../uzlib/examples/tgzip/tgzip $< $@
	#@cp -f $< $@
	@md5sum $@ > $@.md5

CPM22_A.c:A.UZ
	@srec_cat $< -binary -output $@ -C-Array cpm22img


relist:$(ASRCS)
	@for f in $^; do $(AS) -relist:CPM22.map $(AFLAGS) $$f;done

# Very basic test for (de)-compressor
test:CPM22_A.c
	@md5sum -c A.DSK.md5
	@../uzlib/examples/tgunzip/tgunzip A.UZ A.DSK
	@md5sum -c A.DSK.md5

clean:
	@-rm -f $(ASDIR)/*.lst $(AOBJS) CPM.SYS A.* *.md5 1>/dev/null 2>&1
