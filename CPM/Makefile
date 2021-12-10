SHELL=/bin/bash

CPMFILES=./RAMDSK
AFLAGS=-L -m -z --oo CIM,LST --dri

AS=zmac
CAT=cat

ASDIR=./CPM22
ASRCS=BOOT.ASM CCP.ASM BDOS.ASM BIOS.ASM
ACIMS=$(ASRCS:.ASM=.CIM)

current_dir := $(shell pwd)

vpath %.ASM $(ASDIR) 
vpath %.INC $(ASDIR)

all: CPM22_A.c

# Compile Z80-Assembler to Z80-Binary
%.CIM:%.ASM CPM.INC
	$(AS) $(AFLAGS) -o $@ -o $(@:.CIM=.LST) $<

# Concatinate all CP/M Binarys (BOOT,CCP,BDOS,BIOS)
# to system-track (52 sectors) imgage
CPM.SYS:$(ACIMS)
	$(CAT) $^ | dd of=$@ bs=128 count=52

A.DSK:CPM.SYS $(CPMFILES)/* $(CPMFILES)/RESET.COM
	cp $< $@
	dd if=/dev/zero bs=3328 count=75 | tr '\0' '\345' >> $@
	cd $(CPMFILES); cpmcp -f ibm-3740 $(current_dir)/$@ * 0:
	cpmcp -f ibm-3740 $@ $(addprefix $(ASDIR)/,$(ASRCS)) 0:
	md5sum $@ > $@.md5

A.UZ:A.DSK
	../uzlib/examples/tgzip/tgzip $< $@
	md5sum $@ > $@.md5

CPM22_A.c:A.UZ
	srec_cat $< -binary -output $@ -C-Array cpm22img -INClude

$(CPMFILES)/RESET.COM:$(CPMFILES)/RESET.ASM
	$(eval TMP := $(shell mktemp --suffix=.CIM))
	$(AS) $(AFLAGS) -o $(TMP) $<
	cp $(TMP) $@
	rm $(TMP)

# Very basic smoke test for decompressor
test:CPM22_A.c
	md5sum -c A.DSK.md5
	../uzlib/examples/tgunzip/tgunzip A.UZ A.DSK
	md5sum -c A.DSK.md5

clean:
	-rm *.CIM *.LST CPM.SYS A.* CPM22_A.[ch] *.md5 
