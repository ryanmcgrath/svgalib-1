/* mach32info.c prints out some info about your mach32card              */

/* Please report the info it produces if the mach32driver of svgalib    */
/* works not like expected.                                             */

/* This tool is part of svgalib. Although it's output maybe useful to   */
/* debug Xfree86 Mach32 Servers, I am NOT related to Xfree86!!          */
/* PLEASE DO NOT SEND ME (MICHAEL WELLER) ANY XFREE86 BUG REPORTS!!!    */
/* Thanx in advance.                                                    */

/* This tool is free software; you can redistribute it and/or           */
/* modify it without any restrictions. This tool is distributed */
/* in the hope that it will be useful, but without any warranty.        */

/* Copyright 1994 by Michael Weller                                     */
/* eowmob@exp-math.uni-essen.de mat42b@aixrs1.hrz.uni-essen.de          */
/* eowmob@pollux.exp-math.uni-essen.de                                  */

/*

 * MICHAEL WELLER DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL MICHAEL WELLER BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* This tool contains one routine out of Xfree86, therefore I repeat    */
/* its copyright here: (Actually it is longer than the copied code)     */

/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 * Copyright 1993 by Kevin E. Martin, Chapel Hill, North Carolina.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Thomas Roell not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Thomas Roell makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THOMAS ROELL, KEVIN E. MARTIN, AND RICKARD E. FAITH DISCLAIM ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Thomas Roell, roell@informatik.tu-muenchen.de
 *
 * Rewritten for the 8514/A by Kevin E. Martin (martin@cs.unc.edu)
 * Modified for the Mach-8 by Rickard E. Faith (faith@cs.unc.edu)
 * Rewritten for the Mach32 by Kevin E. Martin (martin@cs.unc.edu)
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* Some stuff for the ATI VGA */
#define ATIPORT         0x1ce
#define ATIOFF          0x80
#define ATISEL(reg)     (ATIOFF+reg)
/* Ports we use: */
#define SUBSYS_CNTL     0x42E8
#define GE_STAT         0x9AE8
#define CONF_STAT1      0x12EE
#define CONF_STAT2      0x16EE
#define MISC_OPTIONS    0x36EE
#define MEM_CFG         0x5EEE
#define MEM_BNDRY       0x42EE
#define SCRATCH_PAD_0   0x52EE
#define DESTX_DIASTP    0x8EE8
#define R_SRC_X         0xDAEE
#define R_EXT_GE_CONF   0x8EEE
#define CHIP_ID		0xFAEE
#define MAX_WAITSTATES	0x6AEE
#define LOCAL_CNTL	0x32EE
#define R_MISC_CNTL	0x92EE
#define PCI_CNTL	0x22EE
#define DISP_STATUS	0x2E8
#define DISP_CNTL	0x22E8
#define CLOCK_SEL	0x4AEE
#define H_DISP		0x06E8
#define H_TOTAL		0x02E8
#define H_SYNC_WID	0x0EE8
#define H_SYNC_STRT	0x0AE8
#define V_DISP		0x16E8
#define V_SYNC_STRT	0x1AE8
#define V_SYNC_WID	0x1EE8
#define V_TOTAL		0x12E8
#define	R_H_TOTAL	0xB2EE
#define	R_H_SYNC_STRT	0xB6EE
#define	R_H_SYNC_WID	0xBAEE
#define	R_V_TOTAL	0xC2EE
#define	R_V_DISP	0xC6EE
#define	R_V_SYNC_STRT	0xCAEE
#define	R_V_SYNC_WID	0xD2EE



/* Bit masks: */
#define GE_BUSY         0x0200

/* Chip_id's */
#define ATI68800_3	('A'*256+'A')
#define ATI68800_6	('X'*256+'X')
#define ATI68800_6HX	('H'*256+'X')
#define ATI68800LX	('L'*256+'X')
#define ATI68800AX	('A'*256+'X')

static inline void port_out(int value, int port)
{
    __asm__ volatile ("outb %0,%1"
	      ::"a" ((unsigned char) value), "d"((unsigned short) port));
}

static inline void port_outw(int value, int port)
{
    __asm__ volatile ("outw %0,%1"
	     ::"a" ((unsigned short) value), "d"((unsigned short) port));
}

static inline int port_in(int port)
{
    unsigned char value;
    __asm__ volatile ("inb %1,%0"
		      :"=a" (value)
		      :"d"((unsigned short) port));
    return value;
}

static inline int port_inw(int port)
{
    unsigned short value;
    __asm__ volatile ("inw %1,%0"
		      :"=a" (value)
		      :"d"((unsigned short) port));
    return value;
}

#define inb port_in
#define inw port_inw
#define outb(port, value) port_out(value, port)
#define outw(port, value) port_outw(value, port)

int force = 0, chip_id, bus;
unsigned short eeprom[128];
char *pel_width[] =
{"  4bpp", "  8bpp", "  16bpp", "  24bpp"};
char *bpp16mode[] =
{"  5-5-5", "  5-6-5", "  6-5-5", "  6-6-4"};
char *bpp24mode[] =
{"  RGB", "  RGBa", "  BGR", "  aBGR"};
char *bustype[] =
{"  16-bit ISA", "  EISA", "  16-bit MicroChannel",
 "  32-bit MicroChannel", "  LocalBus SX, 386SX",
 "  LocalBus 1/2, 386DX",
 "  LocalBus 1/2, 486DX", "  PCI"};
char *memtype3[] =
{"  256Kx4 DRAM", "  256Kx4 VRAM, 512 bit serial transfer",
 "  256Kx4 VRAM, 256 bit serial transfer", "  256Kx16 DRAM",
 "  invalid", "  invalid", "  invalid", "  invalid"};
char *memtype6[] =
{"  256Kx4 DRAM", "  256Kx4 VRAM, 512 bit serial transfer",
 "  256Kx16 VRAM, 256 bit serial transfer", "  256Kx16 DRAM",
 "  256Kx4 Graphics DRAM",
 "  256Kx4 VRAM, 512 bit split transfer",
 "  256Kx16 VRAM, 256 bit split transfer",
 "  invalid"};
char *dactype[] =
{"  ATI-68830 (Type 0)", "  SC-11483 (Type 1)",
 "  ATI-68875 (Type 2)", "  Bt-476 (Type 3)",
 "  Bt-481 (Type 4)", "  ATI-68860 (Type 5)",
 "  Unknown type 6", "  Unknown type 7"};
char *localbus[] =
{"  reserved", "  LOCAL#2", "  LOCAL#3", "  LOCAL#1"};
char *aperture[] =
{"  memory aperture disabled", "  1 MB memory aperture",
 "  4 MB memory aperture", "  reserved"};
char *mono_color[] =
{"  white", "  green", "  amber", " reserved"};
char *videomonames[] =
{"lores color - secondary",
 "(hires) color - secondary",
 "monochrome - secondary",
 "lores color - primary",
 "hires color - primary",
 "monochrome - primary"};
char *clockdiv[] =
{"  1", "  2", "  reserved", "  reserved"};
char *transwid[] =
{"  auto select", "  16 bit", "  8 bit", "  8 bit hostdata/16 bit other"};
char *vgabound[] =
{"  shared", "  256 KB", "  512 KB", "  1 MB"};
char *maxpix[] =
{"  8 bpp", "  16 bpp", "  24 bpp", "  reserved"};
static int mach32_clocks[16];

void puttable(int table);

void usage(void)
{
    fputs("Usage: mach32info {info|force}\n"
	  "    prints out almost all the info about your mach32 card from configuration\n"
	  "    registers and Mach32 EEPROM.  It also measures the Mach32 clocks.  A\n"
	  "    completely idle system is required when these measurements are being\n"
	  "    performed.  During these measurements, the video signals will be screwed up\n"
	  "    for about 3-4 seconds.\n"
	  "*   If your monitor does not switch off when getting a video signal it can't\n"
	  "    stand (fixed freq. monitors) better switch it off before starting\n"
	  "    mach32info.  Your computer will beep when it is finished probing.\n"
	  "      You can redirect the 'stdout' of 'mach32info' to some file for viewing\n"
	  "    the results easier.  Do not redirect 'stderr' as you won't hear the beep.\n"
	  "*   The 'force' option disables the sanity check that tries to detect the\n"
	  "    presence of the mach32.  Do not use this option unless you are really,\n"
	  "    really sure that you have a Mach32 compatible vga card installed.\n"
	  "*   This tool is part of svgalib. Although it's output maybe useful to debug\n"
	  "    Xfree86 Mach32 Servers, I am NOT related to Xfree86!  PLEASE DO NOT SEND\n"
	  "    ME (MICHAEL WELLER) ANY XFREE86 BUG REPORTS!  Thanx in advance.\n"
	  "*   Note that this tool comes WITHOUT ANY WARRANTY! Use it at your OWN risk!\n"
	  "*   Warning, this tool does not check for VC changes etc.. Just let it run in\n"
	  "    its own virtual console and don't try to fool it.\n"
	  "Please report any problems with running 'mach32info' or with config-\n"
	  "uring the 'svgalib' mach32 driver to 'eowmob@exp-math.uni-essen.de'.\n"
	"Include the results from running this test with your report.\n",
	  stderr);
    exit(2);
}

static void mach32_i_bltwait()
{
    int i;

    for (i = 0; i < 100000; i++)
	if (!(inw(GE_STAT) & (GE_BUSY | 1)))
	    break;
    if (i >= 100000)
	puts("GE idled out");
}

static int mach32_test()
{
    int result = 0;
    short tmp;

    tmp = inw(SCRATCH_PAD_0);
    outw(SCRATCH_PAD_0, 0x5555);
    mach32_i_bltwait();
    if (inw(SCRATCH_PAD_0) == 0x5555) {
	outw(SCRATCH_PAD_0, 0x2a2a);
	mach32_i_bltwait();
	if (inw(SCRATCH_PAD_0) == 0x2a2a) {
	    /* Aha.. 8514/a detected.. */
	    result = 1;
	}
    }
    outw(SCRATCH_PAD_0, tmp);
    if (!result)
	goto quit;
/* Now ensure it is not a plain 8514/a: */
    result = 0;
    outw(DESTX_DIASTP, 0xaaaa);
    mach32_i_bltwait();
    if (inw(R_SRC_X) == 0x02aa) {
	outw(DESTX_DIASTP, 0x5555);
	mach32_i_bltwait();
	if (inw(R_SRC_X) == 0x0555)
	    result = 1;
    }
  quit:
    return result;
}

static void mach32_wait()
{
/* Wait for at least 22 us.. (got that out of a BIOS disassemble on my 486/50 ;-) ) ... */
    register int i;
    volatile dummy;

    for (i = 0; i < 16; i++)
	dummy++;		/*Dummy is volatile.. */
}

static int mach32_eeclock(register int ati33)
{
    outw(ATIPORT, ati33 |= 0x200);	/* clock on */
    mach32_wait();
    outw(ATIPORT, ati33 &= ~0x200);	/* clock off */
    mach32_wait();
    return ati33;
}

static void mach32_eekeyout(register int ati33, register int offset, register int mask)
{
    do {
	if (mask & offset)
	    ati33 |= 0x100;
	else
	    ati33 &= ~0x100;
	outw(ATIPORT, ati33);
	mach32_eeclock(ati33);
    }
    while (mask >>= 1);
}

static int mach32_eeget(int offset)
{
    register int ati33;
    register int result, i;

/* get current ATI33 */
    outb(ATIPORT, ATISEL(0x33));
    ati33 = ((int) inw(ATIPORT + 1)) << 8;
    ati33 |= ATISEL(0x33);
/* prepare offset.. cut and add header and trailer */
    offset = (0x600 | (offset & 0x7f)) << 1;

/* enable eeprom sequence */
    ati33 = mach32_eeclock(ati33);
/*input to zero.. */
    outw(ATIPORT, ati33 &= ~0x100);
/*enable to one */
    outw(ATIPORT, ati33 |= 0x400);
    mach32_eeclock(ati33);
/*select to one */
    outw(ATIPORT, ati33 |= 0x800);
    mach32_eeclock(ati33);
    mach32_eekeyout(ati33, offset, 0x800);
    for (i = 0, result = 0; i < 16; i++) {
	result <<= 1;
	outb(ATIPORT, ATISEL(0x37));
	if (inb(ATIPORT + 1) & 0x8)
	    result |= 1;
	mach32_eeclock(ati33);
    }
/*deselect... */
    outw(ATIPORT, ati33 &= ~0x800);
    mach32_eeclock(ati33);
/*disable... */
    outw(ATIPORT, ati33 &= ~0x400);
    mach32_eeclock(ati33);
    return result;
}

void putflag(char *str, int flag)
{
    int i;

    i = 72 - strlen(str) - 10;
    printf("   %s  ", str);
    while (i-- > 0)
	putchar('.');
    puts(flag ? ".  enabled" : "  disabled");
}

void putint(char *str, char *format, int value)
{
    char buffer[128];
    int i;

    sprintf(buffer, format, value);
    i = 72 - strlen(str) - strlen(buffer);
    printf("   %s  ", str);
    while (i-- > 0)
	putchar('.');
    puts(buffer);
}

void putstr(char *str, char *strval)
{
    putint(str, strval, 0);
}

unsigned short putword(int word)
{
    printf("\n EEPROM Word %02xh:\t%04x\n", word, eeprom[word]);
    return eeprom[word];
}

char *
 offset(char *buffer, int word)
{
    int tab;

    word >>= 8;
    if ((word < 0x0d) || (word > 0x67)) {
      illegal:
	sprintf(buffer, "  %02xh words (no table there)", word);
    } else {
	tab = word - 0x0d;
	if (tab % (0x1c - 0x0d))
	    goto illegal;
	sprintf(buffer, "  %02xh words (table %d)", word, tab / (0x1c - 0x0d) + 1);
    }
    return buffer;
}

char *
 hsyncstr(int pixels, int clock, double fclock)
{
    static char buffer[50];

    if (!clock)
	sprintf(buffer, "  %d pixels", pixels);
    else
	sprintf(buffer, "  %d pixels, %.3f us",
		pixels, pixels / fclock);
    return buffer;
}

char *
 vsyncstr(int lines, int clock, double lilen)
{
    static char buffer[50];

    if (!clock)
	sprintf(buffer, "  %d lines", lines);
    else
	sprintf(buffer, "  %d lines, %.3f ms",
		lines, lines / lilen);
    return buffer;
}

/* Shamelessly ripped out of Xfree2.1 (with slight changes) : */

static void mach32_scan_clocks(void)
{
    const int knownind = 7;
    const double knownfreq = 44.9;

    char hstrt, hsync;
    int htotndisp, vdisp, vtotal, vstrt, vsync, clck, i;

    int count, saved_nice, loop;
    double scale;

    saved_nice = nice(0);
    nice(-20 - saved_nice);

    puts(
	    "Warning, about to measure clocks. Wait until system is completely idle!\n"
	    "Any activity will disturb measuring, and therefor hinder correct driver\n"
	    "function. Test will need about 3-4 seconds.");
#if 0
    puts("\n(Enter Y<Return> to continue, any other text to bail out)");

    if (getchar() != 'Y')
	exit(0);
    if (getchar() != '\n')
	exit(0);
#endif

    htotndisp = inw(R_H_TOTAL);
    hstrt = inb(R_H_SYNC_STRT);
    hsync = inb(R_H_SYNC_WID);
    vdisp = inw(R_V_DISP);
    vtotal = inw(R_V_TOTAL);
    vstrt = inw(R_V_SYNC_STRT);
    vsync = inw(R_V_SYNC_WID);
    clck = inw(CLOCK_SEL);

    outb(DISP_CNTL, 0x63);

    outb(H_TOTAL, 0x63);
    outb(H_DISP, 0x4f);
    outb(H_SYNC_STRT, 0x52);
    outb(H_SYNC_WID, 0x2c);
    outw(V_TOTAL, 0x418);
    outw(V_DISP, 0x3bf);
    outw(V_SYNC_STRT, 0x3d6);
    outw(V_SYNC_WID, 0x22);

    for (i = 0; i < 16; i++) {
	outw(CLOCK_SEL, (i << 2) | 0xac1);
	outb(DISP_CNTL, 0x23);

	usleep(50000);

	count = 0;
	loop = 200000;

	while (!(inb(DISP_STATUS) & 2))
	    if (loop-- == 0)
		goto done;
	while (inb(DISP_STATUS) & 2)
	    if (loop-- == 0)
		goto done;
	while (!(inb(DISP_STATUS) & 2))
	    if (loop-- == 0)
		goto done;

	for (loop = 0; loop < 5; loop++) {
	    while (!(inb(DISP_STATUS) & 2))
		count++;
	    while ((inb(DISP_STATUS) & 2))
		count++;
	}
      done:
	mach32_clocks[i] = count;

	outb(DISP_CNTL, 0x63);
    }

    outw(CLOCK_SEL, clck);

    outw(H_DISP, htotndisp);
    outb(H_SYNC_STRT, hstrt);
    outb(H_SYNC_WID, hsync);
    outw(V_DISP, vdisp);
    outw(V_TOTAL, vtotal);
    outw(V_SYNC_STRT, vstrt);
    outw(V_SYNC_WID, vsync);
    nice(20 + saved_nice);

/*Recalculation: */
    scale = ((double) mach32_clocks[knownind]) * knownfreq;
    for (i = 0; i < 16; i++) {
	if (i == knownind)
	    continue;
	if (mach32_clocks[i])
	    mach32_clocks[i] = 0.5 + scale / ((double) mach32_clocks[i]);
    }
    mach32_clocks[knownind] = knownfreq + 0.5;
}

int main(int argc, char *argv[])
{
    char *ptr, buffer[40];
    int i, j, lastfound, mask, index, flag;

    memset(eeprom, 0, sizeof(unsigned short) * (size_t) 256);

    if (argc != 2)
	usage();
    if (strcmp(argv[1], "info")) {
	if (strcmp(argv[1], "force"))
	    usage();
	force = 1;
    }
    if (iopl(3) < 0) {
	fputs("mach32info needs to be run as root!\n", stderr);
	exit(1);
    }
    if (!force) {
	if (mach32_test())
	    puts("Mach32 succesful detected.");
	else {
	    fputs("Sorry, no Mach32 detected.\n", stderr);
	    exit(1);
	}
    } else
	puts("Mach32 autodetection skipped.");

    puts("\nThis tool is part of svgalib. Although this output maybe useful\n"
       "to debug Xfree86 Mach32 Servers, I am NOT related to Xfree86!!\n"
    "PLEASE DO NOT SEND ME (MICHAEL WELLER) ANY XFREE86 BUG REPORTS!!!\n"
	 "Thanx in advance.\n");

    mach32_scan_clocks();

    puts("\nResulting clocks command for your libvga.config should be:\n");
    fputs("clocks", stdout);
    for (i = 0; i < 16; i++)
	printf(" %3d", mach32_clocks[i]);

    fputs("\a", stderr);
    fflush(stderr);
    puts("\n\nParsing for chip id...");
    lastfound = inw(CHIP_ID) & 0x3ff;
    flag = 0;
    for (i = 0; i < 10240; i++) {
	j = inw(CHIP_ID) & 0x3ff;
	index = (j >> 4);
	mask = 1 << (j & 15);
	if (!(eeprom[index] & mask))
	    printf("\tfound id: %c%c\n",
		   0x41 + ((j >> 5) & 0x1f), 0x41 + (j & 0x1f));
	eeprom[index] |= mask;
	if (lastfound != j)
	    flag = 1;
    }
/* Build chip_id from last found id: */
    chip_id = (j & 0x1f) + ((j << 3) & 0x1f00);
    chip_id += ATI68800_3;

    switch (chip_id) {
    case ATI68800_3:
	ptr = "ATI68800-3 (guessed)";
	break;
    case ATI68800_6:
	ptr = "ATI68800-6";
	break;
    case ATI68800_6HX:
	ptr = "ATI68800-6 (HX-id)";
	break;
    case ATI68800LX:
	ptr = "ATI68800LX";
	break;
    case ATI68800AX:
	ptr = "ATI68800AX";
	break;
    default:
	ptr = "Unknown (assuming ATI68800-3)";
	chip_id = ATI68800_3;
	flag = 1;
	break;
    }
    printf("Chipset: %s, Class: %d, Revision: %d\n",
	   ptr, (j >> 10) & 3, (j >> 12) & 15);
    if (flag) {
	puts(
		"WARNING! Strange chipset id! Please report all output of this utility\n"
		"together with exact type of your card / type printed on your videochips\n"
		"to me, Michael Weller, eowmob@exp-math.uni-essen.de.  Alternate\n"
		"email-addresses are in the source of this utility and in 'README.mach32'.\n"
	    );
    }
    j = inw(MAX_WAITSTATES);
    if (chip_id == ATI68800AX) {
	printf("\nAPERTURE_CNTL:\t\t%04x\n", j);
	putflag("Zero waitstates for PCI aperture", j & 0x400);
	putflag("Fifo read ahead for PCI aperture", j & 0x800);
	putflag("Pixel stream 1 SCLK delay", j & 0x1000);
	putflag("Decrement burst", j & 0x2000);
	putstr("Direction of burst", (j & 0x4000) ?
	       "Increments burst" : "Decrements burst");
	putflag("Bus timeout on burst read/writes", !(j & 0x8000));
    } else {
	printf("\nMAX_WAITSTATES:\t\t%04x\n", j);
	putint("Max. I/O waitstates", "  %d", 4 * (j & 15));
	putint("BIOS-ROM waitstates", "  %d", (j >> 4) & 15);
	putflag("Linedraw optimizations", j & 0x100);
    }
    j = inw(MISC_OPTIONS);
    printf("\nMISC_OPTIONS:\t\t%04x\n", j);
    putflag("Waitstates if FIFO is half full", j & 0x0001);
    putstr("Host data I/O size", (j & 0x0002) ? "8-bit" : "16-bit");
    putint("Memory size", "  %d KB", (1 << ((j >> 2) & 3)) * 512);
    putflag("VGA-controller", !(j & 0x0010));
    putflag("16-bit 8514 I/O cycles", j & 0x0020);
    putflag("Local RAMDAC", !(j & 0x0040));
    putflag("VRAM-serial/DRAM-memory(bits 63:0) data delay latch", j & 0x0080);
    putflag("Test-mode", j & 0x0100);
    putflag("Non ATI68800-3: Block-write", j & 0x0400);
    putflag("Non ATI68800-3: 64-bit Draw", j & 0x0800);
    putflag("Latch video memory read data", j & 0x1000);
    putflag("Memory data delay latch(bits 63:0)", j & 0x2000);
    putflag("Memory data latch full clock pulse", j & 0x4000);


    j = inw(R_EXT_GE_CONF);
    printf("\nR_EXT_GE_CONF:\t\t%04x\n", j);
    putint("Monitor alias id", "  %d", j & 7);
    putflag("Monitor alias", j & 0x0008);
    putstr("Pixel width", pel_width[(j >> 4) & 3]);
    putstr("16 bit per plane organization", bpp16mode[(j >> 6) & 3]);
    putflag("Multiplex pixels", j & 0x0100);
    putstr("24 bit per plane organization", bpp24mode[(j >> 9) & 3]);
    putstr("Reserved (11)", (j & 0x0800) ? "  1" : "  0");
    putint("Extended RAMDAC address", "  %d", (j >> 12) & 3);
    putflag("8 bit RAMDAC operation", j & 0x4000);
    putstr("Reserved (15)", (j & 0x8000) ? "  1" : "  0");

    j = inw(CONF_STAT1);
    printf("\nCONF_STAT1:\t\t%04x\n", j);
    putflag("VGA circuitry", !(j & 0x0001));
    putstr("Bus Type", bustype[bus = ((j >> 1) & 7)]);
    putstr("Memory Type", (chip_id == ATI68800_3) ? memtype3[(j >> 4) & 7] :
	   memtype6[(j >> 4) & 7]);
    putflag("Chip", !(j & 0x0080));
    putflag("Delay memory write for tests", (j & 0x0100));
    putstr("RAMDAC Type", dactype[(j >> 9) & 7]);
    putflag("Internal MicroChannel address decode", !(j & 0x1000));
    putint("Controller id (0 if unsupported)", "  %d", (j >> 13) & 7);

    j = inw(CONF_STAT2);
    printf("\nCONF_STAT2:\t\t%04x\n", j);
    if (chip_id == ATI68800_3)
	putflag("ATI68800-3: 2 clock sequencer timing", j & 0x0001);
    else
	putstr("Reserved (0)", (j & 0x0001) ? "  1" : "  0");
    putflag("Memory address range FE0000-FFFFFF", !(j & 0x0002));
    if (!bus)
	putflag("16-bit ISA Bus (ISA cards only)", (j & 0x0004));
    else
	putstr("Reserved (2)", (j & 0x0004) ? "  1" : "  0");
    putflag("Korean character font support", (j & 0x0008));
    putstr("Local Bus signal (Local Bus only)", localbus[(j >> 4) & 3]);
    putflag("Local Bus 2 (non multiplexed) configuration", (j & 0x0040));
    putflag("Read data 1 clk after RDY (Local Bus only)", (j & 0x0080));
    putflag("Local decode of RAMDAC write (Local Bus only)", !(j & 0x0100));
    putflag("1 clk RDY delay for write (Local Bus only)", !(j & 0x0200));
    putstr("BIOS EPROM at", (j & 0x0400) ? "  C000:0-C7FF:F" : "  E000:0-E7FF:F");
    switch (bus) {
    case 1:
	putflag("Enable POS register function (EISA)", (j & 0x0800));
	break;
    case 4:
    case 5:
    case 6:
	putflag("Local decode of 102h register (Local Bus only)",
		!(j & 0x0800));
	break;
    default:
	putstr("Reserved (11)", (j & 0x0800) ? "  1" : "  0");
	break;
    }
    putflag("VESA compliant RDY format (Local Bus only)", !(j & 0x1000));
    putflag("Non ATI68800-3: 4 GB aperture address", (j & 0x2000));
    putstr("Non ATI68800-3: Memory support in LBus 2 config",
	   (j & 0x4000) ? "  2MB DRAM" : "  1MB DRAM");
    putstr("Reserved (15)", (j & 0x8000) ? "  1" : "  0");

    j = inw(MEM_BNDRY);
    printf("\nMEM_BNDRY:\t\t%04x\n", j);
    putint("Video memory partition (VGA <, Mach32 >=)", "  %d KB", (j & 15) * 256);
    putflag("Video memory partition write protection", j & 0x0010);
    putint("Reserved (15:5)", "  %03xh", (j >> 5));


    j = inw(MEM_CFG);
    printf("\nMEM_CFG:\t\t%04x\n", j);
    putstr("Memory aperture", aperture[j & 3]);
    putint("Memory aperture page (for 1MB aperture)", "  %d", (j >> 2) & 3);
    if ((bus == 7) || (((bus == 5) || (bus == 6)) && (inw(CONF_STAT2) & 0x2000)))
	putint("Memory aperture location (0-4 GB)", "  %d MB", j >> 4);
    else {
	putint("Reserved (7:4)", "  %x", (j >> 4) & 0xf);
	putint("Memory aperture location (0-128 MB)", "  %d MB", j >> 8);
    }

    j = inw(LOCAL_CNTL);
    printf("\nLOCAL_CNTL:\t\t%04x\n", j);
    putflag("6 clock non page cycle", j & 0x0001);
    putflag("7 clock non page cycle", j & 0x0002);
    putflag("1/2 memory clock CAS precharge time", j & 0x0004);
    putflag("RAMDAC clocked on positive clock edge", j & 0x0008);
    putflag("FIFO testing", j & 0x0010);
    if (chip_id == ATI68800_3)
	putint("Filtering of 1 clock IOW low or high pulse", "  %d", (j >> 5) & 3);
    else {
	putflag("Memory mapped registers", j & 0x0020);
	putflag("Local Bus BIOS ROM decode", j & 0x0040);
    }
    putint("ROM wait states", "  %d", (j >> 7) & 7);
    putint("Memory read wait states", "  %d", (j >> 10) & 3);
    if (chip_id == ATI68800AX)
	putint("Additional I/O waitstates", "  %d", (j >> 12) & 15);
    else
	putint("Minimum Local Bus waistates", "  %d", (j >> 12) & 15);

    j = inw(R_MISC_CNTL);
    printf("\nR_MISC_CNTL:\t\t%04x\n", j);
    putint("Reserved (3:0)", "  %x", j & 15);
    putint("ROM page select", "  %d KB", (j >> 3) & 0x1e);
    putint("Blank adjust (delays BLANK_1_PCLK for RAMDAC type 2)", "  %d", (j >> 8) & 3);
    putint("Pixel data skew from PCLK (pixel delay)", "  %d", (j >> 10) & 3);
    putint("Reserved (15:12)", "  %x", (j >> 12) & 15);

    j = inw(PCI_CNTL);
    printf("\nPCI_CNTL:\t\t%04x\n", j);
    putint("RAMDAC read/write waitstates", "  %d", j & 7);
    putflag("Target abort cycle", j & 0x0004);
    putflag("PCI RAMDAC delay", j & 0x0010);
    putflag("Snooping on DAC read", j & 0x0020);
    putflag("0 waitstates on aperture burst write", j & 0x0040);
    putflag("Fast memory mapped I/O read/write", j & 0x0080);
    putint("Reserved (15:8)", "  %02x", (j >> 8) & 0xff);

    fputs("\nReading in EEPROM... (some screen flicker will occur)", stdout);
    fflush(stdout);
    for (i = 0; i < 128; i++)
	eeprom[i] = mach32_eeget(i);
    puts(" ...done.\n");
    fputs("EEPROM contents:", stdout);
    for (i = 0; i < 128; i++) {
	if (i & 7)
	    putchar(' ');
	else
	    fputs("\n   ", stdout);
	printf(" %02x-%04x", i, eeprom[i]);
    }
    puts("\n\nDecoded info out of EEPROM:");
    putword(0);
    putint("EEPROM write counter", "  %d", eeprom[0]);
    putword(1);
    switch (eeprom[1] & 0xff) {
    case 0x00:
	ptr = "  disabled";
	break;
    case 0x08:
	ptr = "  secondary address";
	break;
    case 0x18:
	ptr = "  primary address";
	break;
    default:
	ptr = "  reserved";
    }
    putstr("Mouse address select", ptr);
    switch ((eeprom[1] >> 8) & 0xff) {
    case 0x20:
	ptr = "  IRQ 5";
	break;
    case 0x28:
	ptr = "  IRQ 4";
	break;
    case 0x30:
	ptr = "  IRQ 3";
	break;
    case 0x38:
	ptr = "  IRQ 2";
	break;
    default:
	ptr = "  reserved";
    }
    putstr("Mouse interrupt handler select", ptr);
    j = putword(2);
    switch ((j >> 8) & 0xff) {
    case 0x03:
    case 0x05:
    case 0x07:
    case 0x09:
    case 0x0b:
    case 0x12:
    case 0x13:
    case 0x15:
    case 0x17:
    case 0x19:
    case 0x1b:
	sprintf(ptr = buffer, "  %cGA %s", (j & 0x1000) ? 'E' : 'V',
		videomonames[(((j >> 8) & 0xf) - 1) >> 1]);
	break;
    case 0x20:
	ptr = "  CGA";
	break;
    case 0x30:
	ptr = "  Hercules 720x348";
	break;
    case 0x40:
	ptr = "  Hercules 640x400";
	break;
    default:
	ptr = "  reserved";
    }
    putstr("Power up video mode", ptr);
    putstr("Monochrome color", mono_color[(j >> 6) & 3]);
    putflag("Dual monitor", j & 0x0020);
    putstr("Power up font", (j & 0x0010) ? "  8x16 or 9x16" : "  8x14 or 9x14");
    putint("VGA Bus I/O", "  %d bits", (j & 0x0008) + 8);
    putflag("0 waitstates RAM read/write", j & 0x0004);
    putflag("0 waitstates ROM read", j & 0x0002);
    putflag("ROM 16 bit", j & 0x0001);
    j = putword(3);
    putflag("Scrolling fix", j & 0x8000);
    putflag("Korean BIOS support", j & 0x4000);
    putint("Reserved (13:4)", "  %03xh", (j >> 4) & 0x3ff);
    putint("EEPROM table revision", "  %d", j & 15);
    j = putword(4);
    putint("Custom monitor indices", "  %04x", j);
    j = putword(5);
    putstr("Host data transfer width", transwid[(j >> 14) & 3]);
    putint("Monitor code", "  %02xh", (j >> 8) & 0x3f);
    putint("Reserved (7)", "  %d", (j >> 7) & 1);
    putstr("VGA boundary", vgabound[(j >> 4) & 3]);
    putflag("Monitor alias", j & 0x0008);
    putint("Monitor alias setting", "  %d", j & 0x0007);
    j = putword(6);
    putint("Memory aperture location", "  %d MB", (j >> 4));
    j &= 15;
    putstr("Memory aperture size", aperture[(j > 3) ? 3 : j]);
    j = putword(7);
    putstr("Offset to 640x480 mode table", offset(buffer, j));
    putint("Reserved (7:2)", "  %02xh", (j >> 2) & 0x3f);
    putflag("Use stored params for 640x480", j & 2);
    putflag("640x480 72Hz", j & 1);
    j = putword(8);
    putstr("Offset to 800x600 mode table", offset(buffer, j));
    putflag("Use stored params for 800x600", j & 0x80);
    putint("Reserved (6)", "  %d", (j >> 6) & 1);
    putflag("800x600 72Hz", j & 0x20);
    putflag("800x600 70Hz", j & 0x10);
    putflag("800x600 60Hz", j & 8);
    putflag("800x600 56Hz", j & 4);
    putflag("800x600 89Hz Interlaced", j & 2);
    putflag("800x600 95Hz Interlaced", j & 1);
    j = putword(9);
    putstr("Offset to 1024x768 mode table", offset(buffer, j));
    putflag("Use stored params for 1024x768", j & 0x80);
    putint("Reserved (6:5)", "  %d", (j >> 5) & 3);
    putflag("1024x768 66Hz", j & 0x10);
    putflag("1024x768 72Hz", j & 8);
    putflag("1024x768 70Hz", j & 4);
    putflag("1024x768 60Hz", j & 2);
    putflag("1024x768 87Hz Interlaced", j & 1);
    j = putword(10);
    putstr("Offset to 1280x1024 mode table", offset(buffer, j));
    putflag("Use stored params for 1280x1024", j & 0x80);
    putint("Reserved (6:2)", "  %02xh", (j >> 2) & 0x1f);
    putflag("1280x1024 95Hz Interlaced", j & 2);
    putflag("1280x1024 87Hz Interlaced", j & 1);
    j = putword(11);
    putstr("Offset to alternate mode table", offset(buffer, j));
    putflag("Use stored params for alternate", j & 0x80);
    putint("Reserved (6:2)", "  %02xh", (j >> 2) & 0x1f);
    putflag("1152x900", j & 2);
    putflag("1120x760", j & 1);
    for (j = 0; j < 7; j++)
	puttable(j);
    puts("\n EEPROM Words 76h-7dh:  reserved.");
    j = putword(0x7e);
    putint("Reserved (15)", "  %d", j >> 15);
    putflag("VGA circuitry", j & 0x4000);
    putint("Memory size", "  %d KB", 1 << (((j >> 11) & 7) + 8));
    putstr("DAC type", dactype[(j >> 8) & 7]);
    putint("Reserved (7:0)", "  %02xh", j & 0xff);
    j = putword(0x7f);
    putint("EEPROM Checksum", "  %04x", j);
    j = 0;
    for (i = 0; i <= 0x7f;)
	j += eeprom[i++];
    printf("\nEEPROM contents sum up to %04x:%04x.\n", j >> 16, j & 0xffff);
    if (!(j & 0xffff)) {
	puts("ATI style checksum.");
    } else {
	j -= (eeprom[0x7f] << 1) - 1;
	if (!(j & 0xffff))
	    puts("AST style checksum.");
	else
	    puts(
		    "WARNING! Strange EEPROM checksum!\n"
		    "Be sure that:\n"
		    "1. You installed the Mach32 correctly with the ATI install tool from\n"
		    "   DOS (yuck!).\n"
		    "2. Wrote the proper config to the EEPROM with it.\n"
		    "3. DOS bios reads out the Mach32 EEPROM with out problems and obeys\n"
		  "   all settings (for example, power up video mode).\n"
		    "If you can't get a correct checksum, read the section \"EEPROM woes\"\n"
		    "in \"README.mach32\" of your svgalib distribution.\n"
		);
    }
    return 0;
}

void puttable(int table)
{
    int i;
    int clock;
    char buffer[80];

    unsigned short *tab;

    tab = eeprom + (table * 15 + 0xd);
    printf("\n EEPROM Words %02xh-%02xh:\tCRT Parameter table %d", table * 15 + 0xd,
	   (table + 1) * 15 + 0xc, table + 1);
    if (tab[10] & 0x3f00)
	puts(":");
    else {
	puts("  .....................  invalid");
	return;
    }
    table = tab[0];
    putstr("Vertical sync polarity", (table & 0x8000) ? "  -" : "  +");
    putstr("Horizontal sync polarity", (table & 0x4000) ? "  -" : "  +");
    putflag("Interlace", table & 0x2000);
    putflag("Multiplex pixels", table & 0x1000);
    i = (table >> 9) & 7;
    putstr("Maximum pixel depth", maxpix[(i > 3) ? 3 : i]);
    putstr("Parameter type", (table & 0x0100) ? "  8514/Mach32" : "  VGA");
    putstr("Dotclock select", (table & 0x0080) ? "  user supplied" : "  default");
    putstr("Usage of CRTC parameters", (table & 0x0040) ? "  all"
	   : "  sync polarities only");
    putint("Dotclock chip select", "  #%d", table & 15);
    clock = mach32_clocks[table & 15];
    putstr("Dotclock divide by", clockdiv[(table >> 4) & 3]);
    if (!(table & 0x20))
	if (table & 0x10)
	    clock /= 2;
    if (clock)
	putint("Pixel clock (approximate value)", "  %d MHz", (int) (clock + 0.5));
    else
	putstr("Pixel clock", "  (sorry, don't know the frequency)");
    if (table & 0x0100) {	/*8514/Mach32 */
	double fclock, lilen;
	int xpels = ((tab[3] & 0xff) + 1) << 3, ypels = tab[6], xtotal = ((tab[3] >> 8) + 1) << 3,
	 ytotal = tab[5], xstart = ((tab[4] >> 8) + 1) << 3, ystart = tab[7],
	 xsync = (tab[4] & 0x1f) * 8, ysync = (tab[8] >> 8) & 0x1f;
	puts("  Mach32 / 8514/A CRT parameters:");
	putint("Video fifo 16bpp", "  %d", tab[2] & 0xff);
	putint("Video fifo 24bpp", "  %d", tab[2] >> 8);
	putint("H_TOTAL", "  %d", tab[3] >> 8);
	putint("H_DISP", "  %d", tab[3] & 0xff);
	putint("H_SYNC_STRT", "  %d", tab[4] >> 8);
	putint("H_SYNC_WID", "  %02xh", tab[4] & 0xff);
	putint("V_TOTAL", "  %xh", tab[5]);
	putint("V_DISP", "  %xh", tab[6]);
	putint("V_SYNC_STRT", "  %xh", tab[7]);
	putint("V_SYNC_WID", "  %02xh", tab[8] >> 8);
	putint("DISP_CNTL", "  %02xh", tab[8] & 0xff);
	putint("CLOCK_SEL", "  %xh", tab[9]);
	clock = mach32_clocks[(tab[9] >> 2) & 15];
	if (!(tab[9] & 0x40))
	    clock *= 2;
	puts("  Resulting video timings:");
	if (clock) {
	    sprintf(buffer, "  %.1f MHz", fclock = ((double) clock) / 2);
	} else {
	    sprintf(buffer, "  #%d, don't know clock frequency, so no timings",
		    (tab[9] >> 2) & 15);
	    fclock = 0;
	}
	putstr("Pixel clock", buffer);
	switch (tab[8] & 0x6) {
	case 0:
	    ypels = ((ypels >> 2) & ~1) | (ypels & 1);
	    ytotal = ((ytotal >> 2) & ~1) | (ytotal & 1);
	    ystart = ((ystart >> 2) & ~1) | (ystart & 1);
	    break;
	case 2:
	    ypels = ((ypels >> 1) & 0xFFFC) | (ypels & 3);
	    ytotal = ((ytotal >> 1) & 0xFFFC) | (ytotal & 3);
	    ystart = ((ystart >> 1) & 0xFFFC) | (ystart & 3);
	    break;
	default:
	    puts("  Unknown DISP_CNTL, vertical values are probably wrong.");
	}
	ypels++;
	ytotal++;
	ystart++;
	sprintf(buffer, "  %d x %d%s", xpels, ypels, (tab[8] & 0x10) ? ", Interlaced" :
		"");
	putstr("Resolution", buffer);
	if (clock) {
	    sprintf(buffer, "  %.3f KHz", lilen = (fclock * 1e3) / xtotal);
	    putstr("Horizontal frequency", buffer);
	    sprintf(buffer, "  %.2f Hz", (lilen * 1000) / ytotal);
	    putstr("Vertical frequency", buffer);
	} else
	    lilen = 0;
	putstr("Horizontal sync polarity", (tab[4] & 0x20) ? "  -" : "  +");
	putstr("Horizontal sync width", hsyncstr(xsync, clock, fclock));
	putstr("Horizontal front porch", hsyncstr(xstart - xpels, clock, fclock));
	putstr("Horizontal back porch", hsyncstr(xtotal - xsync - xstart,
						 clock, fclock));
	putstr("Horizontal active time", hsyncstr(xpels, clock, fclock));
	putstr("Horizontal blank time", hsyncstr(xtotal - xpels, clock, fclock));
	putstr("Vertical sync polarity", (tab[8] & 0x2000) ? "  -" : "  +");
	putstr("Vertical sync width", vsyncstr(ysync, clock, lilen));
	putstr("Vertical front porch", vsyncstr(ystart - ypels, clock, lilen));
	putstr("Vertical back porch", vsyncstr(ytotal - ysync - ystart,
					       clock, lilen));
	putstr("Vertical active time", vsyncstr(ypels, clock, lilen));
	putstr("Vertical blank time", vsyncstr(ytotal - ypels, clock, lilen));
    } else {			/*VGA mode */
	puts("  VGA CRT parameters:");
	putint("VIDEO_MODE_SEL_1", "  %02xh", tab[1] >> 8);
	putint("VIDEO_MODE_SEL_2", "  %02xh", tab[1] & 0xff);
	putint("VIDEO_MODE_SEL_3", "  %02xh", tab[2] >> 8);
	putint("VIDEO_MODE_SEL_4", "  %02xh", tab[2] & 0xff);
	putint("H_TOTAL (CRT00)", "  %02xh", tab[3] >> 8);
	putint("V_TOTAL (CRT06)", "  %02xh", tab[3] & 0xff);
	putint("H_RETRACE_START (CRT04)", "  %02xh", tab[4] >> 8);
	putint("H_RETRACE_END (CRT05)", "  %02xh", tab[4] & 0xff);
	putint("V_RETRACE_START (CRT10)", "  %02xh", tab[5] >> 8);
	putint("V_RETRACE_END (CRT11)", "  %02xh", tab[5] & 0xff);
	putint("H_BLANK_START (CRT02)", "  %02xh", tab[6] >> 8);
	putint("H_BLANK_END (CRT03)", "  %02xh", tab[6] & 0xff);
	putint("V_BLANK_START (CRT15)", "  %02xh", tab[7] >> 8);
	putint("V_BLANK_END (CRT16)", "  %02xh", tab[7] & 0xff);
	putint("CRT_OVERFLOW (CRT07)", "  %02xh", tab[8] >> 8);
	putint("MAX_SCANLINE (CRT09)", "  %02xh", tab[8] & 0xff);
	putint("V_DISPLAYED (CRT12)", "  %02xh", tab[9] >> 8);
	putint("CRT_MODE (CRT17)", "  %02xh", tab[9] & 0xff);
	puts(
		"  Resulting video timings  .........................  not implemented for VGA"
	    );
    }
    table = tab[10];
    puts("  Additional mode flags:");
    putflag("Pixel clock divide by 2", table & 0x8000);
    putflag("Multiplex (MUX flag)", table & 0x4000);
    putint("Size of mode table", "  %d words", (table >> 8) & 0x3f);
    putstr("Offset to alternate table", offset(buffer, (table << 8) & 0xff00));
    putint("Horizontal overscan", "  %d", tab[11]);
    putint("Vertival overscan", "  %d", tab[12]);
    putint("Overscan color blue", "  %d", tab[13] >> 8);
    putint("Overscan color index 8bpp", "  %d", tab[13] & 0xff);
    putint("Overscan color red", "  %d", tab[14] >> 8);
    putint("Overscan color green", "  %d", tab[14] & 0xff);
}
