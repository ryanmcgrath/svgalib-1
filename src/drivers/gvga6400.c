/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* GVGA 6400 driver Copyright 1994 Arno Schaefer */
/* only Hires 256 color modes (640x480, 800x600) supported */


#include <stdlib.h>		/* for NULL */
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include "vga.h"
#include "libvga.h"
#include "driver.h"

#include "regs/gvga6400.regs"

/* Mode table */
static ModeTable gvga6400_modes[] =
{
/* *INDENT-OFF* */
    OneModeEntry(640x480x256),
    OneModeEntry(800x600x256),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

static unsigned char last_page = 0;

static void gvga6400_setregs(const unsigned char regs[], int mode);
static int gvga6400_init(int force, int par1, int par2);

/* Fill in chipset-specific modeinfo */

static void gvga6400_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
    if (modeinfo->bytesperpixel == 1) {		/* linear mode */
	modeinfo->maxpixels = 512 * 1024;
	modeinfo->startaddressrange = 0xffff;
    } else
	switch (modeinfo->colors) {
	case 16:		/* 4-plane 16 color mode */
	    modeinfo->maxpixels = 65536 * 8;
	    modeinfo->startaddressrange = 0x7ffff;
	    break;
	case 256:		/* 4-plane 256 color mode */
	    modeinfo->maxpixels = 65536 * 4;
	    modeinfo->startaddressrange = 0x3ffff;
	    break;
	}

    modeinfo->maxlogicalwidth = 2040;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~IS_INTERLACED;
    modeinfo->flags |= HAVE_RWPAGE;
}

static void nothing(void)
{
}

/* Return nonzero if mode available */

static int gvga6400_modeavailable(int mode)
{
    const unsigned char *regs;

    regs = LOOKUPMODE(gvga6400_modes, mode);

    if (regs == NULL || mode == GPLANE16)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    if (regs == DISABLE_MODE || mode <= TEXT || mode > GLASTMODE)
	return 0;

    return SVGADRV;
}


/* Set a mode */

static int lastmode;

static int gvga6400_setmode(int mode, int prv_mode)
{
    const unsigned char *regs;

    switch (gvga6400_modeavailable(mode)) {
    case STDVGADRV:
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    case SVGADRV:
	regs = LOOKUPMODE(gvga6400_modes, mode);
	if (regs != NULL)
	    break;
    default:
	return (1);		/* not available */
    }

    __svgalib_setregs(regs);
    gvga6400_setregs(regs, mode);

    return 0;
}

static void gvga6400_setpage(int page)
{
    page &= 7;
    last_page = 0x40 | page | (page << 3);

    port_out_r(SEQ_I, 6);
    port_out_r(SEQ_D, last_page);
}

static void gvga6400_setrdpage(int page)
{
    last_page &= 0xf8;
    last_page |= page & 0x07;

    port_out_r(SEQ_I, 6);
    port_out_r(SEQ_D, last_page);
}

static void gvga6400_setwrpage(int page)
{
    last_page &= 0xc7;
    last_page |= ((page & 7) << 3);

    port_out_r(SEQ_I, 6);
    port_out_r(SEQ_D, last_page);
}


/* Set display start */

static void gvga6400_setdisplaystart(int address)
{
    vga_modeinfo modeinfo;
    gvga6400_getmodeinfo(lastmode, &modeinfo);

    if (modeinfo.bytesperpixel == 0)
	switch (modeinfo.colors) {
	case 16:		/* planar 16-color mode */
	    port_in(0x3da);
	    port_out_r(ATT_IW, 0x13 + 0x20);
	    port_out_r(ATT_IW, (port_in(ATT_R) & 0xf0) | (address & 7));
	    /* write sa0-2 to bits 0-2 */
	    address >>= 3;
	    break;
	case 256:		/* planar 256-color mode */
	    port_in(0x3da);
	    port_out_r(ATT_IW, 0x13 + 0x20);
	    port_out_r(ATT_IW, (port_in(ATT_R) & 0xf0) | ((address & 3) << 1));
	    /* write sa0-1 to bits 1-2 */
	    address >>= 2;
	    break;
	}
    port_outw_r(CRT_IC, 0x0d + (address & 0x00ff) * 256);	/* sa0-sa7 */
    port_outw_r(CRT_IC, 0x0c + (address & 0xff00));	/* sa8-sa15 */
}

static void gvga6400_setlogicalwidth(int width)
{
    port_outw_r(CRT_IC, 0x13 + (width >> 3) * 256);	/* lw3-lw11 */
}


static int gvga6400_test(void)
{
    int mem_fd;
    unsigned char *vga_bios;
    int result = 0;
    int address;

    /*  Ensure that this open gets a file descriptor greater
     *  than 2, else problems can occur with stdio functions
     *  under certain strange conditions:  */
    if ((fcntl(0,F_GETFD) == -1) && (open("/dev/null", O_RDONLY) == -1)) {
        perror("/dev/null");
        exit(1);
    }
    if ((fcntl(1,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)) {
        perror("/dev/null");
        exit(1);
    }
    if ((fcntl(2,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)) {
        perror("/dev/null");
        exit(1);
    }
    mem_fd = open("/dev/mem", O_RDONLY);
    if (mem_fd == -1) {
        perror("/dev/mem");
        exit(-1);
    }
    /*  Ensure this file is closed if we were ever to exec something else...  */
    if (fcntl(mem_fd, F_SETFD, 1) == -1) {
        perror("fcntl");
        exit(-1);
    }

    /* Changed to use valloc(). */
    if ((vga_bios = valloc(4096)) == NULL) {
	fprintf(stderr, "svgalib: malloc error\n");
	exit(-1);
    }
    vga_bios = (unsigned char *) mmap
	(
	    (caddr_t) vga_bios,
	    4096,
	    PROT_READ,
	    MAP_SHARED | MAP_FIXED,
	    mem_fd,
	    0xc0000
	);
    if (vga_bios == MAP_FAILED) {
	fprintf(stderr, "svgalib: mmap error\n");
	exit(-1);
    }
    address = vga_bios[0x37];

    if (vga_bios[address] == 0x77 &&
	vga_bios[address + 1] == 0x11 &&
	vga_bios[address + 2] == 0x99 &&
	vga_bios[address + 3] == 0x66) {
	result = 1;
	gvga6400_init(0, 0, 0);
    }
#if 0
    munmap((caddr_t) vga_bios, 4096);
    free(vga_bios);
#endif
    close(mem_fd);

    return (result);
}

static int gvga6400_saveregs(unsigned char regs[])
{
    port_out_r(SEQ_I, 6);
    regs[EXT + 2] = port_in(SEQ_D);
    port_out_r(SEQ_I, 7);
    regs[EXT + 3] = port_in(SEQ_D);
    port_out_r(SEQ_I, 8);
    regs[EXT + 4] = port_in(SEQ_D);
    port_out_r(SEQ_I, 0x10);
    regs[EXT + 5] = port_in(SEQ_D);
    port_out_r(GRA_I, 9);
    regs[EXT + 6] = port_in(GRA_D);
    port_out_r(CRT_IC, 0x2f);
    regs[EXT + 10] = port_in(CRT_DC);

    return (11);
}

static void gvga6400_setregs(const unsigned char regs[], int mode)
{
    port_out_r(SEQ_I, 6);
    port_out_r(SEQ_D, regs[EXT + 2]);
    port_out_r(SEQ_I, 7);
    port_out_r(SEQ_D, regs[EXT + 3]);
    port_out_r(SEQ_I, 8);
    port_out_r(SEQ_D, regs[EXT + 4]);
    port_out_r(SEQ_I, 0x10);
    port_out_r(SEQ_D, regs[EXT + 5]);
    port_out_r(GRA_I, 9);
    port_out_r(GRA_D, regs[EXT + 6]);
    port_out_r(CRT_IC, 0x2f);
    port_out_r(CRT_DC, regs[EXT + 10]);
}

DriverSpecs __svgalib_gvga6400_driverspecs =
{
    gvga6400_saveregs,
    gvga6400_setregs,
    nothing,
    nothing,
    gvga6400_test,
    gvga6400_init,
    gvga6400_setpage,
    gvga6400_setrdpage,
    gvga6400_setwrpage,
    gvga6400_setmode,
    gvga6400_modeavailable,
    gvga6400_setdisplaystart,
    gvga6400_setlogicalwidth,
    gvga6400_getmodeinfo,
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    0,				/* extset */
    0,
    0,				/* linear */
    NULL,                       /* Accelspecs */
    NULL,                       /* Emulation */
};

/* Initialize chipset (called after detection) */

static int gvga6400_init(int force, int par1, int par2)
{
    if (__svgalib_driver_report)
	fprintf(stderr,"Using Genoa GVGA 6400 driver.\n");
    __svgalib_driverspecs = &__svgalib_gvga6400_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;

    return 0;
}
