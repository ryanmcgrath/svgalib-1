#----------------------------------------------------------------------
# Makefile for threeDKit
#----------------------------------------------------------------------

# *** NO SERVICIBLE PARTS HERE!
#     All options are in Makefile.cfg.

include ../Makefile.cfg

srcdir    = ..
VPATH     = $(srcdir)/src

#----------------------------------------------------------------------
# Compiler Section (overrides Makefile.cfg)
#----------------------------------------------------------------------

INCLUDES += -I$(srcdir)/src

#----------------------------------------------------------------------
# Rules Section
#----------------------------------------------------------------------

CFILES = 3dinit.c 3dkit.c quickmath.c swtriangle.c triangl.c triangle.c wrapsurf.c wtriangle.c
OBJECTS = $(CFILES:.c=.o)
	#
# Determine what library (static or shared) we will be linking programs with
ifdef INSTALLSHAREDLIB
  LIBS = -lm -lvgagl -lvga
endif
ifndef LIBS
  LIBS    = -lm $(srcdir)/staticlib/libvgagl.a $(srcdir)/staticlib/libvga.a
  LVGADEP = $(srcdir)/staticlib/libvgagl.a $(srcdir)/staticlib/libvga.a
endif

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<

.c.s:
	$(CC) $(CFLAGS) -S -o $*.s $<

.PHONY:	all clean install installheaders



ifeq (a.out, $(TARGET_FORMAT))

all:	lib3dkit.a plane wrapdemo

else                                                               

all:	lib3dkit.so.$(VERSION) plane wrapdemo

# These rules are for ELF only.
lib3dkit.so.$(VERSION): $(OBJECTS)
	$(CC) -shared -Wl,-soname,lib3dkit.so.$(MAJOR_VER) \
	  -o lib3dkit.so.$(VERSION) $(OBJECTS) -lm

$(sharedlibdir)/lib3dkit.so.$(VERSION): lib3dkit.so.$(VERSION)
	$(INSTALL_SHLIB) $< $(sharedlibdir)/$<
	(cd $(sharedlibdir); ln -sf lib3dkit.so.$(VERSION) `echo lib3dkit.so.$(VERSION) | sed 's/\.so\..*/.so/'` )
	-ldconfig

install: $(sharedlibdir)/lib3dkit.so.$(VERSION) installheaders

installheaders:
	@cp 3dkit.h $(includedir)/3dkit.h
	@chmod a+r $(includedir)/3dkit.h
	@cp triangle.h $(includedir)/triangle.h
	@chmod a+r $(includedir)/triangle.h

endif	  


lib3dkit.a: $(ALLOBJS)
	rm -f lib3dkit.a
	$(AR) rcs lib3dkit.a $(ALLOBJS)

plane: planukit.o planinit.o $(OBJECTS) $(LVGADEP)
	$(CC) $(LDFLAGS) -o plane planukit.o planinit.o $(OBJECTS) $(LIBS)
	# chown root plane
	# chmod u+s plane

wrapdemo: wrapdemo.o $(OBJECTS) $(LVGADEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o wrapdemo wrapdemo.c $(OBJECTS) $(LIBS)
	# chown root wrapdemo
	# chmod u+s wrapdemo

clean:
	rm -f *.o core lib3dkit.a lib3dkit.so.* plane wrapdemo *.bak


