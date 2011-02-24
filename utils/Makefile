#----------------------------------------------------------------------
# Makefile for SVGAlib utilities.
#
# This file is a part of SVGAlib.
#----------------------------------------------------------------------

include ../Makefile.cfg

srcdir    = ..
VPATH     = $(srcdir)/utils

#----------------------------------------------------------------------
# Compiler Section (overrides Makefile.cfg)
#----------------------------------------------------------------------

CFLAGS = $(WARN) $(OPTIMIZE) -I../include -L../sharedlib
#Use the next one for the Alpha/AXP if you need it
#LDFLAGS = -L../staticlib 
LIBS = -lvga -lm

#----------------------------------------------------------------------
# Rules Section
#----------------------------------------------------------------------

UTILPROGS = restorefont convfont restoretextmode restorepalette dumpreg gtfcalc 
OBJECTS = restorefont.o convfont.o restoretextmode.o restorepalette.o dumpreg.o

ifeq ($(ARCH),i386)
UTILPROGS += fix132x43 setmclk
OBJECTS += fix132x43.o setmclk.o
endif

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

.o:
	$(CC) $(CFLAGS) $(LDFLAGS) -o $* $*.o $(LIBS)
	chmod 4755 $*

all: $(UTILPROGS)
.PHONY: all clean cleanbin dep

$(OBJECTS): .depend

convfont: convfont.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o convfont convfont.o $(LIBS)

gtfcalc: gtf/gtfcalc.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o gtfcalc -DTESTING_GTF gtf/gtfcalc.c $(LIBS)

clean:
	rm -f $(UTILPROGS) *.o *~ UVCONFIG.CRT

dep:
	rm -f .depend
	make .depend

install:
	$(INSTALLPROG) $(UTILPROGS) $(UTILINSTALLDIR)

.depend:
	gcc $(INCLUDES) -MM $(patsubst %.o,$(srcdir)/utils/%.c,$(OBJECTS)) >.depend

#
# include a dependency file if one exists
#
ifeq (.depend.src,$(wildcard .depend.src))
include .depend.src
endif
