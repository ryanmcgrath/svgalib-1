#---------------------------------------------------------------------OU
# Makefile for SVGAlib.
#
# It's pretty unreadable, but just doing make install should be
# enough. This will install the header files and shared library first
# (which is enough to compile things), after which the static version is
# optionally compiled and installed (if it fails, the shared libraries
# should still work fine).
#
#----------------------------------------------------------------------

# *** NO SERVICIBLE PARTS HERE!
#     All options are in Makefile.cfg.

#include Makefile.ppc
include Makefile.cfg

#----------------------------------------------------------------------
# Rules Section
#----------------------------------------------------------------------

# In case someone goes for the demos w/o installing svgalib make
# the static libs first.
ifdef INSTALLSHAREDLIB
  PREDEMO =
else
  PREDEMO = static
endif

# A flag if this is a distribution:
DISTRIBUTION = $(shell sh -c "echo sharedlib/DIST*")

UTILS	= restorefont runx restorepalette restoretextmode textmode \
	  savetextmode dumpreg fix132x43

OBSOLETEHDIRS = /usr/include/ /usr/include/vga/ /usr/local/include/ /usr/local/include/vga/
OBSOLETEBDIRS = /usr/bin/ /usr/local/bin/

OBSOLETEHEADERS = /inc/vga.h /inc/vgakeyboard.h /inc/vgamouse.h /inc/vgagl.h /inc/vgajoystick.h

OBSOLETELIBLINKS = /lib/libvga.so /lib/libvga.so.1 /lib/libvgagl.so /lib/libvgagl.so.1

OBSOLETESHAREDIMAGES = /lib/libvga.so.1.* /lib/libvga.so

# for ELF
OBSOLETESHAREDIMAGES +=	/lib/libvgagl.so.1.* /lib/libvgagl.so

OBSOLETELDIRS = /lib/ /usr/lib/ /usr/local/lib/ /usr/share/lib/ \
		$(shell sed 's?\#.*$$??' /etc/ld.so.conf 2>/dev/null | \
		  sed 's?\([^/]\)[ 	]*$$?\1/ ?' | grep -v aout/ )

SHAREDLIBS = sharedlib/libvga.so.$(VERSION) sharedlib/libvgagl.so.$(VERSION)
SVGALIBSHAREDSTUBS =
JUMP =

BACKUP   = ../svgalib-$(VERSION).tar.gz

default:
	@echo "To install SVGAlib, do one of the following:"
	@echo ""
	@echo "	make clean      - clean every thing. Do this after every change"
	@echo "	                  of Makefile.cfg!"
	@echo "	make install	- compile & install components specified in Makefile.cfg"
	@echo "	make demoprogs	- make demo programs in demo/ and threeDKit/"
	@echo ""
	@echo "	make uninstall	- remove an existing installation from various"
	@echo "	                  common places. (old traces often confuse the"
	@echo "	                  compiler even when svgalib is not installed anew)"
	@echo "	                  (make install includes an uninstall first)"
	@echo ""
	@echo "	Be sure to read the file 0-INSTALL!"
	@echo ""

.PHONY: default all install installheaders installconfig
.PHONY: clean distclean indent uninstall
.PHONY: force_remake remake_shared shared static
.PHONY: indent-gnu 

installheaders:
	@echo Installing header files in $(includedir).
	@mkdir -p $(includedir)
	@cp $(SRCDIR)/src/vga.h $(includedir)/vga.h
	@chmod a+r $(includedir)/vga.h
	@cp $(SRCDIR)/gl/vgagl.h $(includedir)/vgagl.h
	@chmod a+r $(includedir)/vgagl.h
	@cp $(SRCDIR)/src/mouse/vgamouse.h $(includedir)/vgamouse.h
	@chmod a+r $(includedir)/vgamouse.h
	@cp $(SRCDIR)/src/joystick/vgajoystick.h $(includedir)/vgajoystick.h
	@chmod a+r $(includedir)/vgajoystick.h
	@cp $(SRCDIR)/src/keyboard/vgakeyboard.h $(includedir)/vgakeyboard.h
	@chmod a+r $(includedir)/vgakeyboard.h

installsharedlib: $(SHAREDLIBS) $(SVGALIBSHAREDSTUBS)
	@echo Installing shared library image as \
		$(addprefix $(sharedlibdir)/,$(notdir $(SHAREDLIBS))).
	@mkdir -p ${sharedlibdir};
	@for foo in $(notdir $(SHAREDLIBS)); do \
		$(INSTALL_SHLIB) sharedlib/$$foo $(sharedlibdir)/$$foo; \
		(cd $(sharedlibdir); \
		 ln -sf $$foo `echo $$foo | sed 's/\.so\..*/.so/'` ); \
	done
	@./fixldsoconf
	-ldconfig

installstaticlib: static
	@echo Installing static libraries in $(libdir).
	@mkdir -p $(libdir)
	@$(INSTALL_DATA) staticlib/libvga.a $(libdir)/libvga.a
	@chmod a+r $(libdir)/libvga.a
	@$(INSTALL_DATA) staticlib/libvgagl.a $(libdir)/libvgagl.a
	@chmod a+r $(libdir)/libvgagl.a

installutils: textutils $(LRMI)
	@if [ ! -d $(bindir) ]; then \
		echo No $(bindir) directory, creating it.; \
		mkdir -p $(bindir); \
	fi
	@echo Installing textmode utilities in $(bindir):
	@echo "restorefont:      Save/restore textmode font."
	@$(INSTALL_PROGRAM) utils/restorefont $(bindir)
	@echo "restorepalette:   Set standard VGA palette."
	@$(INSTALL_PROGRAM) utils/restorepalette $(bindir)
	@echo "dumpreg:          Write ASCII dump of SVGA registers."
	@$(INSTALL_PROGRAM) utils/dumpreg $(bindir)
	@echo "restoretextmode:  Save/restore textmode registers."
	@$(INSTALL_PROGRAM) utils/restoretextmode $(bindir)
	@echo "textmode:         Script that tries to restore textmode."
	@$(INSTALL_SCRIPT) utils/textmode $(bindir)
	@echo "savetextmode:     Script that saves textmode information used by 'textmode'."
	@$(INSTALL_SCRIPT) utils/savetextmode $(bindir)
ifeq ($(LRMI),lrmi)
	@echo "mode3:       Restore textmode by setting VESA mode 3."
	@$(INSTALL_PROGRAM) lrmi-0.6m/mode3 $(bindir)
endif
	@echo "Installing keymap utilities in $(bindir):"
	@echo "svgakeymap:       Perl script that generates scancode conversion maps."
	@$(INSTALL_SCRIPT) utils/svgakeymap $(bindir)

installconfig:
	mkdir -p ${datadir};
	@if [ \( -f /usr/local/lib/libvga.config -a ! -f $(datadir)/libvga.config \) ]; then \
		echo "Moving old config file /usr/local/lib/libvga.config to $(datadir)." ; \
		mv -f /usr/local/lib/libvga.config $(datadir)/libvga.config; \
	fi
	@if [ \( -f /usr/local/lib/libvga.et4000 -a ! -f $(datadir)/libvga.et4000 \) ]; then \
		echo "Moving old config file /usr/local/lib/libvga.et4000 to $(datadir)." ; \
		mv -f /usr/local/lib/libvga.et4000 $(datadir)/libvga.et4000; \
	fi
	@if [ \( -f /usr/local/lib/libvga.ega -a ! -f $(datadir)/libvga.ega \) ]; then \
		echo "Moving old config file /usr/local/lib/libvga.ega to $(datadir)." ; \
		mv -f /usr/local/lib/libvga.ega $(datadir)/libvga.ega; \
	fi
	@if [ \( -f /etc/mach32.eeprom -a ! -f $(datadir)/mach32.eeprom \) ]; then \
		echo Consider moving your /etc/mach32.eeprom file to $(datadir) ; \
		echo and changing $(datadir)/libvga.config appropriately. ; \
	fi
	@if [ ! -f $(datadir)/libvga.config ]; then \
		echo Installing default configuration file in $(datadir).; \
		cp $(CONFDIR)/libvga.config $(datadir)/libvga.config; \
	fi
	@if [ ! -f $(datadir)/libvga.et4000 ]; then \
		echo Installing dynamically loaded ET4000 registers in $(datadir).; \
		cp $(CONFDIR)/et4000.regs $(datadir)/libvga.et4000; \
	fi
	@if [ ! -f $(datadir)/default.keymap ]; then \
		echo Installing default keymap file in $(datadir).; \
		cp $(CONFDIR)/default.keymap $(datadir)/null.keymap; \
	fi
	@if [ ! -f $(datadir)/dvorak-us.keymap ]; then \
		echo Installing Dvorak keymap file in $(datadir).; \
		cp $(CONFDIR)/dvorak-us.keymap $(datadir)/dvorak-us.keymap; \
	fi

installman:
	(cd doc; $(MAKE) -f $(SRCDIR)/doc/Makefile SRCDIR="$(SRCDIR)" install )

installmodule:
	(cd kernel/svgalib_helper ; $(MAKE) default modules_install )

installmodule.alt:
	(cd kernel/svgalib_helper ; $(MAKE) -f Makefile.alt modules_install )

installdev:
	(cd kernel/svgalib_helper ; $(MAKE) device )

lib3dkit-install:
	(cd threeDKit/; $(MAKE) install)
	 
install: installheaders $(INSTALLSHAREDLIB) installconfig \
	$(INSTALLSTATICLIB) $(INSTALLUTILS) $(INSTALLMAN) $(INSTALLMODULE) $(INSTALLDEV) \
	lib3dkit-install
	@echo
	@echo
	@echo Now run "'make demoprogs'" to make the test and demo programs in
	@echo demos/ and threedkit/.

uninstall:
	@echo "Removing textmode utilities..."
	@for i in $(OBSOLETEBDIRS); do \
          for prog in $(UTILS); do \
            rm -f $$i$$prog ; \
          done ; \
         done
	@echo "Removing shared library stubs (old & current)..."
	@for i in $(OBSOLETELDIRS); do \
	    rm -f `echo /lib/libvga.so.$(VERSION) /lib/libvgagl.so.$(VERSION) \
			$(OBSOLETELIBLINKS) /lib/libvga.sa /lib/libvgagl.sa \
		     | sed s?/lib/?$$i?g`; \
         done
ifndef KEEPSHAREDLIBS
	@echo "Removing shared library images (old & current)..."
	@for i in $(OBSOLETELDIRS); do \
	    rm -f `echo $(OBSOLETESHAREDIMAGES) | sed s?/lib/?$$i?g`; \
         done
endif
	@echo "Removing static libraries..."
	@for i in $(OBSOLETELDIRS); do \
	    rm -f `echo /lib/libvga.a /lib/libvgagl.a | sed s?/lib/?$$i?g`; \
         done
	@echo "Removing header files..."
	@for i in $(OBSOLETEHDIRS); do \
	    rm -f `echo $(OBSOLETEHEADERS) | sed s?/inc/?$$i?g`; \
         done
	(cd doc; $(MAKE) -f $(SRCDIR)/doc/Makefile SRCDIR="$(SRCDIR)" uninstall)
	 

SHAREDDIRS0 = sharedlib/mouse sharedlib/keyboard sharedlib/ramdac \
		sharedlib/clockchip sharedlib/joystick \
		sharedlib/drivers
SHAREDDIRS = $(SHAREDDIRS0) $(JUMP)
STATICDIRS = staticlib/mouse staticlib/keyboard staticlib/ramdac \
		staticlib/clockchip staticlib/joystick \
		staticlib/drivers
UTILDIRS = utils
DEMODIRS = demos threeDKit

$(SHAREDDIRS0) $(STATICDIRS) $(DEMODIRS):
	mkdir -p $@

utils:
	mkdir -p utils
	if [ ! -f utils/runx ]; then \
		cp $(SRCDIR)/utils/runx $(SRCDIR)/utils/savetextmode \
		   $(SRCDIR)/utils/textmode utils; \
	fi

static: staticlib/libvga.a staticlib/libvgagl.a

.PHONY: staticlib/libvgagl.a staticlib/libvga.a

staticlib/libvgagl.a staticlib/libvga.a: $(STATICDIRS)
	(cd $(dir $@); \
	 $(MAKE) -f $(SRCDIR)/src/Makefile $(notdir $@) \
	 	SRCDIR="$(SRCDIR)" DLLFLAGS="" \
	)

# ELF

.PHONY: sharedlib/libvga.so.$(VERSION) sharedlib/libvgagl.so.$(VERSION)

shared: $(SHAREDLIBS) $(SVGALIBSHAREDSTUBS)

sharedlib/libvga.so.$(VERSION): $(SHAREDDIRS)
	@rm -f sharedlib/DISTRIBUTION
	(cd $(dir $@); \
	 $(MAKE) -f $(SRCDIR)/src/Makefile $(notdir $@) \
	 	SRCDIR="$(SRCDIR)" DLLFLAGS="$(DLLFLAGS)"; \
		ln -fs libvga.so.$(VERSION) libvga.so; \
	)

sharedlib/libvgagl.a: $(SHAREDDIRS)
	(cd $(dir $@); \
	 $(MAKE) -f $(SRCDIR)/gl/Makefile $(notdir $@) \
	 	SRCDIR="$(SRCDIR)" DLLFLAGS="$(DLLFLAGS)" \
	)

sharedlib/libvgagl.so.$(VERSION): $(SHAREDDIRS)
	(cd $(dir $@); \
	$(MAKE) -f $(SRCDIR)/gl/Makefile $(notdir $@) \
		SRCDIR="$(SRCDIR)" DLLFLAGS="$(DLLFLAGS)"; \
		ln -fs libvgagl.so.$(VERSION) libvgagl.so; \
	)

demoprogs: $(PREDEMO) $(DEMODIRS)
	@for dir in $(DEMODIRS); do \
		if [ -d $(SRCDIR)/$$dir ]; then \
			(cd $$dir; \
			$(MAKE) -f $(SRCDIR)/$$dir/Makefile SRCDIR="$(SRCDIR)"); \
		fi; \
	done

textutils: $(UTILDIRS)
	(cd utils; \
	$(MAKE) -f $(SRCDIR)/utils/Makefile SRCDIR="$(SRCDIR)")

lrmi:
	(cd lrmi-0.6m;\
	$(MAKE))

backup: $(BACKUP)

$(BACKUP):
# I tried using a dependency, but make reordered them
# thus I have to do it this way:
	$(MAKE) shared
	$(MAKE) distclean

	sed 's/^TARGET_FORMAT = a.out$$/# TARGET_FORMAT = a.out/' \
		$(SRCDIR)/Makefile.cfg | \
	sed 's/^#[ 	]*TARGET_FORMAT = elf$$/TARGET_FORMAT = elf/' \
		> mkcfg ; \
	mv mkcfg $(SRCDIR)/Makefile.cfg; \
	(cd ..; \
	find svgalib-$(VERSION) ! -type d -print | sort | tar -cvf- -T- ) \
		| gzip -9 >$(BACKUP)

distclean:
	(cd $(SRCDIR)/doc; $(MAKE) clean)
	(cd $(SRCDIR)/doc; $(MAKE) ../0-README)
	(cd $(SRCDIR)/src; $(MAKE) clean)
	(cd $(SRCDIR)/gl; $(MAKE) clean)
	(cd $(SRCDIR)/utils; $(MAKE) clean)
	(cd $(SRCDIR)/demos; $(MAKE) clean)
	(cd $(SRCDIR)/threeDKit; $(MAKE) clean)
	(cd $(SRCDIR)/lrmi-0.6m; $(MAKE) clean)
	(cd $(SRCDIR)/kernel/svgalib_helper; $(MAKE) clean)
	rm -f *.orig
	find . \( -name '.depend*' -o -name '*~*' \) -exec rm {} \;
	rm -rf sharedlib/[!l]* sharedlib/l[!i]* sharedlib/li[!b]* staticlib
	rm -rf sharedlib/*.a core
	mkdir -p sharedlib
	touch sharedlib/DISTRIBUTION

clean: distclean
	rm -rf sharedlib

indent:
	find demos gl mach src support -name '*.[ch]' -exec indent -kr {} \;
	indent -kr src/*.regs

indent-gnu:
	find demos gl mach src support -name '*.[ch]' -exec indent -gnu {} \;
	indent -gnu src/*.regs

dkms:
	rm -rf /usr/src/svgalib_helper-$(VERSION)
	mkdir -p /usr/src/svgalib_helper-$(VERSION)
	cp -a kernel/svgalib_helper/* /usr/src/svgalib_helper-$(VERSION)
	dkms add -m svgalib_helper -v $(VERSION)
	dkms build -m svgalib_helper -v $(VERSION)
	dkms install -m svgalib_helper -v $(VERSION)

FORCE:
