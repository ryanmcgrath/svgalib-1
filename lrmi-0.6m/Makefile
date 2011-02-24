CFLAGS ?= -g -Wall

sources = lrmi.c
objects = lrmi.o
all = vbetest mode3 vga_reset vbemodeinfo dosint

%.o: %.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

all: $(all)

vbetest: vbetest.c lrmi.o
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $^

mode3: mode3.c lrmi.o
	$(CC) $(CFLAGS) -o $@ $^

dosint: dosint.c lrmi.o
	$(CC) $(CFLAGS) -o $@ $^

vbemodeinfo: vbemodeinfo.c lrmi.o
	$(CC) $(CFLAGS) -o $@ $^

vga_reset: vga_reset.c lrmi.o
	$(CC) $(CFLAGS) -o $@ $^

install: mode3 vga_reset
	install mode3 /sbin
	install vga_reset /sbin

.PHONY: clean
clean:
	rm -f $(objects) $(all) core regs-out *.bak

.PHONY: distclean
distclean: clean
	rm -f .depend

.PHONY: depend
depend: $(sources)
	-$(CC) -M $(CPPFLAGS) $^ >.depend
