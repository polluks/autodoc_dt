ROOTPATH=../../../../
include $(ROOTPATH)Makefile.global

CFLAGS+= -DUSE_INLINE_STDARG -DCOMPILE_DATATYPE -I./
LDLIBS = -labox -lmemblock -lmath -lc -lm -lsyscall

VERSION = 1

OBJS = autodoc_init.o libfunc.o functable.o obtainengine.o autodoc_class.o

# The descriptor datatypes.library reads to map *.autodoc, *.doc files
# onto this class (FORM DTYP in DEVS:DataTypes, written by tools/mkdesc.py
# of creep-ltx's AmigaDatatypes). DTDIR is where your OS keeps descriptors;
# DEVS:DataTypes on AmigaOS, MorphOS and AROS alike.
PATTERN = \#?.(autodoc|doc)
VERSTR  = $(shell sed -n 's/^#define VERSION_STRING.*"\$$VER: //p' autodoc.datatype_VERSION.h | sed 's/\\r\\n".*//')
DTDIR   = DEVS:DataTypes

autodoc.datatype: $(OBJS)
	$(LINKECHO)
	$(LINKPREFIX)$(CC) -noixemul -nostdlib -o $@.db $(OBJS) $(LDLIBS)
	$(LINKPREFIX)$(STRIP) --strip-unneeded --remove-section .comment $@.db -o $@

DUMP:	autodoc.datatype
	ppc-morphos-objdump --section-headers --all-headers --reloc --disassemble-all autodoc.datatype.db >autodoc.datatype.dump

Autodoc: tools/mkdesc.py autodoc.datatype_VERSION.h
	python3 tools/mkdesc.py $@ Autodoc autodoc '$(PATTERN)' text adoc 0x0001 0 '$(VERSTR)'

all: autodoc.datatype Autodoc

autodoc_init.o: autodoc_init.c autodoc_intern.h libdefs.h autodoc.datatype_VERSION.h
libfunc.o: libfunc.c autodoc_intern.h libdefs.h
functable.o: functable.c autodoc_intern.h libdefs.h
obtainengine.o: obtainengine.c autodoc_intern.h libdefs.h
autodoc_class.o: autodoc_class.c autodoc_intern.h

install: all
	mkdir -p SYS:MorphOS/classes/datatypes
	cp autodoc.datatype /sys/morphos/classes/datatypes/autodoc.datatype
	mkdir -p $(DTDIR)
	cp Autodoc $(DTDIR)/Autodoc
	-flushlib autodoc.datatype

install-iso: all
	mkdir -p $(ISOPATH)MorphOS/Classes/Datatypes
	cp autodoc.datatype $(ISOPATH)MorphOS/Classes/Datatypes/autodoc.datatype
	mkdir -p $(ISOPATH)Devs/DataTypes
	cp Autodoc $(ISOPATH)Devs/DataTypes/Autodoc

check: all
	multiview sample.autodoc

source:
	(cd .. && tar --transform "s,^autodoc,&.datatype," -cf $(SOURCEPATH)autodoc.datatype.tar autodoc)

bump:
	bumprev2 VERSION $(VERSION) FILE autodoc.datatype_VERSION TAG autodoc.datatype ADD " Credits"

clean:
	-rm -rf *.bak *.o autodoc.datatype autodoc.datatype.db Autodoc