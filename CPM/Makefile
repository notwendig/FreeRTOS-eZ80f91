
ZDEII=~/ZiLOG/ZDSII_eZ80Acclaim!_5.3.4/bin/

LN=$(ZDEII)ez80link.exe

LFLAGS=-FORMAT=INTEL32 -map -maxhexlen=32 -warnoverlap -xref -unresolved=fatal -sort ADDRESS=ascending -warn -NOigcase

OBJS=BOOT.obj CCP.obj BDOS.obj BIOS.obj

vpath %.obj ../Debug

COMMA := ,
EMPTY :=
SPACE := $(EMPTY) $(EMPTY)

NUMS = $(subst $(SPACE),$(COMMA),$(OBJS))

all: cpm.sys

cpm.sys:cpm.hex
	srec_cat $< -Intel -o $@ -Binary
	ln -fs $(CURDIR)/cpm.sys ./disks/A/.cpm.sys
	
cpm.hex:$(OBJS)
	$(LN) @cpm.linkcmd
	
#.PONY check

check:cpm.map
#sed -ne 's/\s*Segment:\s*\(BOOT\|CPM22\|CBIOS\)\s*D:\([[:xdigit:]]\+\)\s*D:\([[:xdigit:]]\+\)\s*\([[:xdigit:]]\+\)h/\1 \2 \3 \4 /p' cpm.map

clean:
	-rm cpm.map cpm.hex cpm.sys
