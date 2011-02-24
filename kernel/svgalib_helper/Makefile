
SVGALIB_HELPER_MAJOR=209

svgalib_helper-objs := main.o i810.o interrupt.o virtual.o displaystart.o
obj-m		:= svgalib_helper.o

ifneq ($(KERNELRELEASE),)

   VER    ?= $(KERNELRELEASE)
   KDIR	:= /lib/modules/$(VER)/build
   PWD	:= $(shell pwd)
   TARGET  := svgalib_helper

      ifeq ($(PATCHLEVEL),4)
         include $(TOPDIR)/Rules.make
      endif

else

   VER    ?= $(shell uname -r)
   KDIR	:= /lib/modules/$(VER)/build
   PWD	:= $(shell pwd)
   TARGET  := svgalib_helper

endif


CLASS_SIMPLE := $(shell grep -s class_simple_create $(KDIR)/include/linux/device.h)

ifneq ($(CLASS_SIMPLE),)
  CLASS_CFLAGS = -DCLASS_SIMPLE=1
endif

EXTRA_CFLAGS := -DSVGALIB_HELPER_MAJOR=$(SVGALIB_HELPER_MAJOR) $(CLASS_CFLAGS)


default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) CLASS_CFLAGS=$(CLASS_CFLAGS) modules

depend:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) depend


clean:
	rm -f svgalib_helper.{o,ko,mod.{o,c}} .*.{cmd,flags}
	rm -f *.o  *~ core .depend *.bak *.orig
	rm -fr .tmp_versions

svgalib_helper.o: $(svgalib_helper-objs)
	$(LD) -r -o $@ $(svgalib_helper-objs)

install: modules_install

modules_install: $(MODNAME)
	mkdir -p $(TOPDIR)/lib/modules/$(VER)/kernel/misc
	install -m 0644 -c $(TARGET).ko $(TOPDIR)/lib/modules/$(VER)/kernel/misc ||	install -m 0644 -c $(TARGET).o $(TOPDIR)/lib/modules/$(VER)/kernel/misc

