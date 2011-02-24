/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* Modified by Hartmut Schirmer */

/*----------------------------------------------------------------------*/
/* most of this code is the result of hacking round with the		*/
/* XFree86 chips & technologies driver, the tvga8900 (svgalib		*/
/* driver), vgadoc4b, the output from dumpreg (run from text mode	*/
/* and also under XFree86)						*/
/* all copyrights acknowledged						*/
/* 									*/
/* HACKED 1996 by Sergio and Angelo Masci				*/
/*		  titan.demon.co.uk					*/
/* 									*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* MODIFIED 1996 by David Bateman <dbateman@ee.uts.edu.au>		*/
/* 		Added Linear Addressing					*/
/* 		Added BitBLT support					*/
/* 		Added support for the HiQV chips			*/
/* 		Programmable Clocks and XFree like modelines		*/
/* 		15/16 and 24bpp support					*/
/* MODIFIED 1998:                                                       */
/*              Added support for 65555, 68554, 69000 and 64300 chips   */
/*              Some smaller fixes (65554 memory probing, max. pixel    */
/*                                 clock)                               */
/* 									*/
/* Note that although this is a fully featured driver, it is still	*/
/* considered to be experimental. It hasn't been exhaustively tested at	*/
/* all with much code written from the specification sheets for chipset	*/
/* and architectures that the author did have available. So sucess or	*/
/* failure reports are most welcome. Please e-mail to "David Bateman	*/
/* <dbateman@ee.uts.edu.au>"						*/
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* Fixes 1998 for ct65550 by Christian Groessler <cpg@aladdin.de>	*/
/*              - modelines work now                                    */
/*              - some smaller bugs fixed                               */
/*              - set XRE2 to BIOS video mode                           */
/*              - in vga.c: no usleep(MODESWITCHDELAY) when restoring   */
/*                textmode -> textmode streching is restored correctly. */
/*----------------------------------------------------------------------*/

/*----------------------------------------------------------------------*/
/* By default the text clock frequency is assumed to be 25.175MHz.	*/
/* at the LCD and 28.322MHz at a CRT. This can be forced to be probed	*/
/* by defining CHIPS_PROBE_TEXT_CLOCK below. Note that the probing	*/
/* process can be unreliable. Alternatively the text clock can be	*/
/* overridden from the libvga.config file with the TextClockFreq	*/
/* option.								*/
/*----------------------------------------------------------------------*/
/*
#define CHIPS_PROBE_TEXT_CLOCK
*/

/*----------------------------------------------------------------------*/
/* Normaly there is a 64K page at 0xA0000 that acts as a window		*/
/* into video RAM located on the video adaptor. This window can		*/
/* be moved around to allow all the video RAM to be accessed by		*/
/* the CPU. Some chip sets allow two different windows to share		*/
/* the same 64K page. The video adaptor differentiates between		*/
/* the two windows by using the read/write signal provided by the	*/
/* CPU, so that when it is reading from any location within the		*/
/* 64K page at 0xA0000 it is reading through the read window		*/
/* (often refered to as the read bank) and when it is writing to	*/
/* any location within the 64K page at 0xA0000 it is writing		*/
/* through the write window (often refered to as the write bank).	*/
/* This allows the CPU to move data around the video RAM without	*/
/* the overhead of continually changing the address of the window	*/
/* between reads and writes. The Chips & Technologies chip set		*/
/* does thing slightly differently. It allows the 64K page at		*/
/* 0xA0000 to be split into two 32K pages, one at 0xA0000 and the	*/
/* other at 0xA8000. These two pages provide two seperate windows	*/
/* each of which can be accessed for both reading and writing data.	*/
/*----------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>		/* sigprocmask */
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

/* New style driver interface */
#include "timing.h"
#include "vgaregs.h"
#include "interface.h"
#include "accel.h"
#include "vgapci.h"

#define FALSE 0
#define TRUE (!FALSE)

#define ENTER TRUE
#define LEAVE FALSE
typedef int Bool;

static int CHIPSchipset;
static int video_memory;	/* amount of video memory in K */
static unsigned char ctVgaIOBaseFlag;
struct {
    int HDisplay;
    int HRetraceStart;
    int HRetraceEnd;
    int HTotal;
    int VDisplay;
    int VRetraceStart;
    int VTotal;
} __svgalib_ctSize;
static Bool ctLCD=FALSE, ctCRT=TRUE;
/* Panel Types */
unsigned char __svgalib_ctPanelType = 0;
#define TFT 1
#define SS 2			       /* STN Types */
#define DS 4
#define DD 6
#define IS_STN(X) X&6

/* Masks for which option flags have been set */
static int ctFlagsSet = 0;
#define ctFlags_LCDPanelSize	0x1
#define ctFlags_UseModeline	0x2
#define ctFlags_NoBitBlt	0x4
#define ctFlags_Use18BitBus	0x8
#define ctFlags_SetLinear	0x10
#define ctFlags_StretchEnable	0x20
#define ctFlags_StretchDisable	0x40
#define ctFlags_CenterEnable	0x80
#define ctFlags_CenterDisable	0x100

/* Global variables for acceleration support */
static unsigned int ctROP = 0; /* Default to GXcopy */
static unsigned int ctFGCOLOR;
static unsigned int ctBGCOLOR;
static unsigned int ctTRANSMODE = 0; /* Default to non transparency */
static Bool ctMMIO=FALSE;	/* Is the chip using MMIO */
unsigned char *__svgalib_ctMMIOBase = NULL;	/* MMIO base address */
unsigned int __svgalib_ctMMIOPage = -1;		/* MMIO paged base address */
unsigned int __svgalib_CHIPS_LinearBase = -1; /* Linear FrameBuffer address */
unsigned char *__svgalib_ctBltDataWindow = NULL; /* Window for Monochrome src data */

static void * MMIO_mem1, * MMIO_mem2;

static int ct_video_mode(int bpp, int weight_green, int display_size);

/* Forward definitions for accelerated support */
static void CHIPS_ScreenCopy(int x1, int y1, int x2, int y2, int w, int h);
static void CHIPS_mmio_ScreenCopy(int x1, int y1, int x2, int y2, int w, 
				  int h);
static void CHIPS_hiqv_ScreenCopy(int x1, int y1, int x2, int y2, int w, 
				  int h);
void __svgalib_CHIPS_FillBox(int x, int y, int width, int height);
void __svgalib_CHIPS_mmio_FillBox(int x, int y, int width, int height);
void __svgalib_CHIPS_hiqv_FillBox(int x, int y, int width, int height);
void __svgalib_CHIPS_FillBox24(int x, int y, int width, int height);
void __svgalib_CHIPS_mmio_FillBox24(int x, int y, int width, int height);
void __svgalib_CHIPS_PutBitmap(int x, int y, int w, int h, void *bitmap);
void __svgalib_CHIPS_mmio_PutBitmap(int x, int y, int w, int h, void *bitmap);
void __svgalib_CHIPS_hiqv_PutBitmap(int x, int y, int w, int h, void *bitmap);
void __svgalib_CHIPS_SetRasterOp(int rop);
void __svgalib_CHIPS_SetBGColor(int fg);
void __svgalib_CHIPS_SetFGColor(int fg);
static void CHIPS_Sync(void);
static void CHIPS_mmio_Sync(void);
static void CHIPS_hiqv_Sync(void);
void __svgalib_CHIPS_SetTransparency(int mode, int color);

/* alu to C&T conversion for use with source data */
static unsigned int ctAluConv[] =
{
    0xCC,			/* ROP_COPY   : dest = src; GXcopy */
    0xEE,			/* ROP_OR     : dest |= src; GXor */
    0x88,			/* ROP_AND    : dest &= src; GXand */
    0x66,			/* ROP_XOR    : dest = ^src; GXxor */
    0x55,			/* ROP_INVERT : dest = ~dest; GXInvert */
};
/* alu to C&T conversion for use with pattern data */
static unsigned int ctAluConv2[] =
{
    0xF0,			/* ROP_COPY   : dest = src; GXcopy */
    0xFC,			/* ROP_OR     : dest |= src; GXor */
    0xA0,			/* ROP_AND    : dest &= src; GXand */
    0x5A,			/* ROP_XOR    : dest = ^src; GXxor */
    0x55,			/* ROP_INVERT : dest = ~dest; GXInvert */
};

/* Bitwise reversal of bytes, required for monochrome source expansion */
unsigned char __svgalib_byte_reversed[256] =
{
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
};

/* Registers to save */
unsigned char __svgalib_XRregs[] =
{
0x02, 0x03, 0x04, 0x06, 0x07, 0x0B, 0x0D, 0x0F, 0x17, 0x19,
0x1A, 0x1B, 0x1C, 0x1E, 0x21, 0x22, 0x23, 0x28, 0x2C, 0x2D,
0x2F, 0x33, 0x40, 0x50, 0x51, 0x52, 0x54, 0x55, 0x56, 0x57, 
0x58, 0x5A, 0x64, 0x65, 0x66, 0x67, 0x68, 0x6F,
};
unsigned char __svgalib_HiQVXRregs[] =
{
0x09, 0x0A, 0x0E, 0x20, 0x40, 0x80, 0x81, 0xE2,
};
unsigned char __svgalib_HiQVCRregs[] =
{
0x30, 0x31, 0x32, 0x33, 0x38, 0x3C, 0x41,
};
unsigned char __svgalib_HiQVFRregs[] =
{
0x03, 0x08, 0x10, 0x11, 0x12, 0x20, 0x21, 0x22, 0x23, 0x24,
0x25, 0x26, 0x27, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
0x37, 0x40, 0x48, 0x73,
};

static int NUM_XRregs = (sizeof(__svgalib_XRregs) / sizeof(__svgalib_XRregs[0]));
static int NUM_HiQVXRregs = (sizeof(__svgalib_HiQVXRregs) / sizeof(__svgalib_HiQVXRregs[0]));
static int NUM_HiQVFRregs = (sizeof(__svgalib_HiQVFRregs) / sizeof(__svgalib_HiQVFRregs[0]));
static int NUM_HiQVCRregs = (sizeof(__svgalib_HiQVCRregs) / sizeof(__svgalib_HiQVCRregs[0]));
static int NUM_Clock = 5;
#define CHIPSREG_XR(i) (VGA_TOTAL_REGS + i)
#define CHIPSREG_HiQVXR(i) (VGA_TOTAL_REGS + NUM_XRregs + i)
#define CHIPSREG_HiQVFR(i) (VGA_TOTAL_REGS + NUM_XRregs + NUM_HiQVXRregs + i)
#define CHIPSREG_HiQVCR(i) (VGA_TOTAL_REGS + NUM_XRregs + NUM_HiQVXRregs + \
			    NUM_HiQVFRregs + i)
#define CHIPS_CLOCK(i) (VGA_TOTAL_REGS + NUM_XRregs + NUM_HiQVXRregs + \
			    NUM_HiQVFRregs + NUM_HiQVCRregs + i)
#define CHIPS_TOTAL_REGS (VGA_TOTAL_REGS + NUM_XRregs + NUM_HiQVXRregs + \
			  NUM_HiQVFRregs + NUM_HiQVCRregs + NUM_Clock)
#define MSR CHIPS_CLOCK(0)
#define VCLK(i) CHIPS_CLOCK(1 + i)
#define XR02 CHIPSREG_XR(0)
#define XR03 CHIPSREG_XR(1)
#define XR04 CHIPSREG_XR(2)
#define XR06 CHIPSREG_XR(3)
#define XR07 CHIPSREG_XR(4)
#define XR0B CHIPSREG_XR(5)
#define XR0D CHIPSREG_XR(6)
#define XR0F CHIPSREG_XR(7)
#define XR17 CHIPSREG_XR(8)
#define XR19 CHIPSREG_XR(9)
#define XR1A CHIPSREG_XR(10)
#define XR1B CHIPSREG_XR(11)
#define XR1C CHIPSREG_XR(12)
#define XR1E CHIPSREG_XR(13)
#define XR21 CHIPSREG_XR(14)
#define XR22 CHIPSREG_XR(15)
#define XR23 CHIPSREG_XR(16)
#define XR28 CHIPSREG_XR(17)
#define XR2C CHIPSREG_XR(18)
#define XR2D CHIPSREG_XR(19)
#define XR2F CHIPSREG_XR(20)
#define XR33 CHIPSREG_XR(21)
#define XR40 CHIPSREG_XR(22)
#define XR50 CHIPSREG_XR(23)
#define XR51 CHIPSREG_XR(24)
#define XR52 CHIPSREG_XR(25)
#define XR54 CHIPSREG_XR(26)
#define XR55 CHIPSREG_XR(27)
#define XR56 CHIPSREG_XR(28)
#define XR57 CHIPSREG_XR(29)
#define XR58 CHIPSREG_XR(30)
#define XR5A CHIPSREG_XR(31)
#define XR64 CHIPSREG_XR(32)
#define XR65 CHIPSREG_XR(33)
#define XR66 CHIPSREG_XR(34)
#define XR67 CHIPSREG_XR(35)
#define XR68 CHIPSREG_XR(36)
#define XR6F CHIPSREG_XR(37)
#define HiQVXR09 CHIPSREG_HiQVXR(0)
#define HiQVXR0A CHIPSREG_HiQVXR(1)
#define HiQVXR0E CHIPSREG_HiQVXR(2)
#define HiQVXR20 CHIPSREG_HiQVXR(3)
#define HiQVXR40 CHIPSREG_HiQVXR(4)
#define HiQVXR80 CHIPSREG_HiQVXR(5)
#define HiQVXR81 CHIPSREG_HiQVXR(6)
#define HiQVXRE2 CHIPSREG_HiQVXR(7)

#define HiQVFR03 CHIPSREG_HiQVFR(0)
#define HiQVFR08 CHIPSREG_HiQVFR(1)
#define HiQVFR10 CHIPSREG_HiQVFR(2)
#define HiQVFR11 CHIPSREG_HiQVFR(3)
#define HiQVFR12 CHIPSREG_HiQVFR(4)
#define HiQVFR20 CHIPSREG_HiQVFR(5)
#define HiQVFR21 CHIPSREG_HiQVFR(6)
#define HiQVFR22 CHIPSREG_HiQVFR(7)
#define HiQVFR23 CHIPSREG_HiQVFR(8)
#define HiQVFR24 CHIPSREG_HiQVFR(9)
#define HiQVFR25 CHIPSREG_HiQVFR(10)
#define HiQVFR26 CHIPSREG_HiQVFR(11)
#define HiQVFR27 CHIPSREG_HiQVFR(12)
#define HiQVFR30 CHIPSREG_HiQVFR(13)
#define HiQVFR31 CHIPSREG_HiQVFR(14)
#define HiQVFR32 CHIPSREG_HiQVFR(15)
#define HiQVFR33 CHIPSREG_HiQVFR(16)
#define HiQVFR34 CHIPSREG_HiQVFR(17)
#define HiQVFR35 CHIPSREG_HiQVFR(18)
#define HiQVFR36 CHIPSREG_HiQVFR(19)
#define HiQVFR37 CHIPSREG_HiQVFR(20)
#define HiQVFR40 CHIPSREG_HiQVFR(21)
#define HiQVFR48 CHIPSREG_HiQVFR(22)
#define HiQVFR73 CHIPSREG_HiQVFR(23)

#define HiQVCR30 CHIPSREG_HiQVCR(0)
#define HiQVCR31 CHIPSREG_HiQVCR(1)
#define HiQVCR32 CHIPSREG_HiQVCR(2)
#define HiQVCR33 CHIPSREG_HiQVCR(3)
#define HiQVCR38 CHIPSREG_HiQVCR(4)
#define HiQVCR3C CHIPSREG_HiQVCR(5)
#define HiQVCR41 CHIPSREG_HiQVCR(6)

/* Define the set of fixed H/W clocks used by those chips that don't
 * support programmable clocks */
#define CHIPS_NUM_CLOCKS 4
static int chips_fixed_clocks[CHIPS_NUM_CLOCKS] =
{
    25175, 28322, 31500, 35500
};
#ifndef CHIPS_PROBE_TEXT_CLOCK
#define LCD_TXT_CLOCK_FREQ 25175
#define CRT_TXT_CLOCK_FREQ 28322
#endif
static int ctTextClock = 0;
static int ctDacSpeed = 0;

static int CHIPS_init(int, int, int);
static int CHIPS_interlaced(int mode);
static void CHIPS_EnterLeave(Bool enter);
static void CHIPS_setlinear(int addr);
static int vgaIOBase = 0;
static Bool ctisHiQV = FALSE;
static Bool PCIcard = FALSE;
static unsigned int chips_pcilinearbase = -1;

#define CT_520	 0
#define CT_525	 1
#define CT_530	 2
#define CT_535	 3
#define CT_540	 4
#define CT_545	 5
#define CT_546	 6
#define CT_548	 7
#define CT_550	 8
#define CT_554	 9
#define CT_555	10
#define CT_8554	11
#define CT_9000	12
#define CT_4300	13

static CardSpecs *cardspecs;

/*----------------------------------------------------------------------*/
/*----------------------------------------------------------------------*/
static void nothing(void)
{
}


/*----------------------------------------------------------------------*/
/* Fill in chipset specific mode information				*/
/*----------------------------------------------------------------------*/
static void CHIPS_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_getmodeinfo(%d, *)\n", mode);
#endif
	if (modeinfo->bytesperpixel > 0)
	{	modeinfo->maxpixels = video_memory * 1024 /
			modeinfo->bytesperpixel;
	}
	else
	{	modeinfo->maxpixels = video_memory * 1024;
	}

	modeinfo->maxlogicalwidth = 2040;
	modeinfo->linewidth_unit = 8;
	modeinfo->startaddressrange = 0xfffff;
	modeinfo->memory = video_memory;

	if (mode == G320x200x256)
	{
		/* Special case: bank boundary may not fall within display. */
		modeinfo->startaddressrange = 0xf0000;

		/* Hack: disable page flipping capability for the moment. */
		modeinfo->startaddressrange = 0xffff;
		modeinfo->maxpixels = 65536;
	}

	modeinfo->haveblit = 0;
	if (CHIPS_interlaced(mode))
	{	modeinfo->flags |= IS_INTERLACED;
	}
	if (CHIPSchipset != CT_520)
	{
	    modeinfo->flags |= EXT_INFO_AVAILABLE | CAPABLE_LINEAR;	
	}

#if defined(seperated_read_write_bank)
	if (ctisHiQV) {
	    /* HiQV doesn't have seperate Read/Write pages */
	    modeinfo->flags &= ~HAVE_RWPAGE;
	} else {
	    modeinfo->flags |= HAVE_RWPAGE;
	}
#else
	modeinfo->flags &= ~HAVE_RWPAGE;
#endif
}

static void ctHWCalcClock(unsigned char *vclk, unsigned int Clock)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: ctHWCalcClock(*, %d)\n", Clock);
#endif
    vclk[MSR] =  (Clock << 2) & 0xC;
    vclk[XR54] = vclk[MSR];
    vclk[XR33] = 0;
}

#define Fref 14318180

/* 
 * This is Ken Raeburn's <raeburn@raeburn.org> clock
 * calculation code just modified a little bit to fit in here.
 */

static void ctCalcClock(unsigned char *vclk, unsigned int Clock)
{
    int M, N, P, PSN, PSNx;

    int bestM=0, bestN=0, bestP=0, bestPSN=0;
    double bestError, abest = 42, bestFout=0;
    double target;

    double Fvco, Fout;
    double error, aerror;

    int M_min = 3;

    /* Hack to deal with problem of Toshiba 720CDT clock */
    int M_max = ctisHiQV ? 63 : 127;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: ctCalcClock(*, %d)\n", Clock);
#endif

    /* Other parameters available on the 65548 but not the 65545, and
     * not documented in the Clock Synthesizer doc in rev 1.0 of the
     * 65548 datasheet:
     * 
     * + XR30[4] = 0, VCO divider loop uses divide by 4 (same as 65545)
     * 1, VCO divider loop uses divide by 16
     * 
     * + XR30[5] = 1, reference clock is divided by 5
     * 
     * Other parameters available on the 65550 and not on the 65545
     * 
     * + XRCB[2] = 0, VCO divider loop uses divide by 4 (same as 65545)
     * 1, VCO divider loop uses divide by 16
     * 
     * + XRCB[1] = 1, reference clock is divided by 5
     * 
     * + XRCB[7] = Vclk = Mclk
     * 
     * + XRCA[0:1] = 2 MSB of a 10 bit M-Divisor
     * 
     * + XRCA[4:5] = 2 MSB of a 10 bit N-Divisor
     * 
     * I haven't put in any support for those here.  For simplicity,
     * they should be set to 0 on the 65548, and left untouched on
     * earlier chips.  */

    target = Clock * 1000;

    for (PSNx = 0; PSNx <= 1; PSNx++) {
	int low_N, high_N;
	double Fref4PSN;

	PSN = PSNx ? 1 : 4;

	low_N = 3;
	high_N = 127;

	while (Fref / (PSN * low_N) > 2.0e6)
	    low_N++;
	while (Fref / (PSN * high_N) < 150.0e3)
	    high_N--;

	Fref4PSN = Fref * 4 / PSN;
	for (N = low_N; N <= high_N; N++) {
	    double tmp = Fref4PSN / N;

	    for (P = ctisHiQV ? 1 : 0; P <= 5; P++) {	
	      /* to force post divisor on Toshiba 720CDT */
		double Fvco_desired = target * (1 << P);
		double M_desired = Fvco_desired / tmp;

		/* Which way will M_desired be rounded?  Do all three just to
		 * be safe.  */
		int M_low = M_desired - 1;
		int M_hi = M_desired + 1;

		if (M_hi < M_min || M_low > M_max)
		    continue;

		if (M_low < M_min)
		    M_low = M_min;
		if (M_hi > M_max)
		    M_hi = M_max;

		for (M = M_low; M <= M_hi; M++) {
		    Fvco = tmp * M;
		    if (Fvco <= 48.0e6)
			continue;
		    if (Fvco > 220.0e6)
			break;

		    Fout = Fvco / (1 << P);

		    error = (target - Fout) / target;

		    aerror = (error < 0) ? -error : error;
		    if (aerror < abest) {
			abest = aerror;
			bestError = error;
			bestM = M;
			bestN = N;
			bestP = P;
			bestPSN = PSN;
			bestFout = Fout;
		    }
		}
	    }
	}
    }

    if (ctisHiQV) {
        vclk[MSR] = 3 << 2;
        /* Leave non-clock bits of FR03 alone */
        vclk[HiQVFR03] = (vclk[HiQVFR03] & 0xF3) | vclk[MSR];
	vclk[VCLK(0)] = bestM - 2;
	vclk[VCLK(1)] = bestN - 2;
	vclk[VCLK(2)] = 0;
	vclk[VCLK(3)] = (bestP << 4) + (bestPSN == 1);
#ifdef DEBUG
	if (__svgalib_driver_report) {
	    fprintf(stderr,"Probed Freq: %.2f MHz", (float)(Clock / 1000.));
            fprintf(stderr,"VCLK(0) = %x, ",VCLK(0));
            fprintf(stderr,"VCLK(1) = %x, ",VCLK(1));
            fprintf(stderr,"VCLK(2) = %x, ",VCLK(2));
            fprintf(stderr,"VCLK(3) = %x\n",VCLK(3));
	    fprintf(stderr,", vclk[0]=%X, vclk[1]=%X, vclk[2]=%X, vlck[3]=%X\n",
		   vclk[VCLK(0)], vclk[VCLK(1)], vclk[VCLK(2)], vclk[VCLK(3)]);
	    fprintf(stderr,"Freq used: %.2f MHz\n", bestFout / 1.0e6);
	}
#endif
	return;
    } else {	
        vclk[MSR] = 3 << 2;
        /* Leave non-clock bits of XR54 alone */
        vclk[XR54] = (vclk[XR54] & 0xF3) | vclk[MSR];
	vclk[VCLK(0)] = (bestP << 1) + (bestPSN == 1);
	vclk[VCLK(1)] = bestM - 2;
	vclk[VCLK(2)] = bestN - 2;
#ifdef DEBUG
	if (__svgalib_driver_report) {
	    fprintf(stderr,"Probed Freq: %.2f MHz", (float)(Clock / 1000.));
	    fprintf(stderr,", vclk[0]=%X, vclk[1]=%X, vclk[2]=%X\n",
		   vclk[VCLK(0)], vclk[VCLK(1)], vclk[VCLK(2)]);
	    fprintf(stderr,"Freq used: %.2f MHz\n", bestFout / 1.0e6);
	}
#endif
	return;
    }
    
}

static void ctClockSave(unsigned char regs[])
{
    unsigned char xr54, msr, temp;
    int i;
    unsigned int Clk;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: ctClockSave\n");
#endif

    msr = port_in(0x3CC);  	/* save the standard VGA clock registers */
    regs[MSR] = msr;
    
    if (ctisHiQV) {
	port_out_r(0x3D0,0x03);
	regs[HiQVFR03] = port_in(0x3D1);/* save alternate clock select reg.  */
	temp = (regs[HiQVFR03] & 0xC) >> 2;
	if (temp == 3)
	    temp = 2;
	temp = temp << 2;
	for (i = 0; i < 4; i++) {
	    port_out_r(0x3D6,0xC0 + i + temp);
	    regs[VCLK(i)] = port_in(0x3D7);
	}
    } else {
	port_out_r(0x3D6,0x33);    /* get status of MCLK/VCLK select reg.*/
	regs[XR33] = port_in(0x3D7);
	port_out_r(0x3D6,0x54);    /* save alternate clock select reg.   */
	xr54 = port_in(0x3D7);
	regs[XR54] = xr54;
	if ((CHIPSchipset == CT_535 || CHIPSchipset == CT_540 ||
	     CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
	     CHIPSchipset == CT_548 || CHIPSchipset == CT_4300) && 
	     (((xr54 & 0xC) == 0xC) || ((xr54 & 0xC) == 0x8))) {
	    if (ctTextClock) {
		Clk = ctTextClock;
	    } else {
#ifdef CHIPS_PROBE_TEXT_CLOCK
		register int status = vgaIOBase + 0xA;
		int maskval = 0x08;
		int cnt, rcnt, sync;
		int Clk1, Clk2;
		sigset_t sig2block;

		/* If we are using a programmable clock, then there
		 * is no way to be sure that the register values
		 * correspond to those currently uploaded to the VCO.
		 * Hence we use the same ugly hack XFree uses to probe
		 * the value of the clock */

		/* Disable the interrupts */
		sigfillset(&sig2block);
		sigprocmask(SIG_BLOCK, &sig2block, (sigset_t *)NULL);

		/* Select Clk1 */
		port_out_r(0x3C2, (msr & 0xF2) | 0x4 | ctVgaIOBaseFlag);
		port_out_r(0x3D6,0x54);
		port_out_r(0x3D7,((xr54 & 0xF3) | 0x4));
	    
		usleep(50000);     /* let VCO stabilise */
	    
		cnt  = 0;
		sync = 200000;

		while ((port_in(status) & maskval) == 0x00) 
	            if (sync-- == 0) goto finishClk1;
		/* Something appears to be happening, so reset sync count */
		sync = 200000;
		while ((port_in(status) & maskval) == maskval) 
	            if (sync-- == 0) goto finishClk1;
		/* Something appears to be happening, so reset sync count */
		sync = 200000;
		while ((port_in(status) & maskval) == 0x00) 
	            if (sync-- == 0) goto finishClk1;
    
		for (rcnt = 0; rcnt < 5; rcnt++) 
		{
		    while (!(port_in(status) & maskval)) 
		        cnt++;
		    while ((port_in(status) & maskval)) 
		        cnt++;
		}
		
	      finishClk1:
		Clk1 = cnt;

		/* Select Clk2 or Clk3 */
		port_out_r(0x3C2, (msr & 0xFE) | ctVgaIOBaseFlag);
		port_out_r(0x3D6,0x54);
		port_out_r(0x3D7,xr54);
	    
		usleep(50000);     /* let VCO stabilise */

		cnt  = 0;
		sync = 200000;

		while ((port_in(status) & maskval) == 0x00) 
	            if (sync-- == 0) goto finishClk2;
		/* Something appears to be happening, so reset sync count */
		sync = 200000;
		while ((port_in(status) & maskval) == maskval) 
	            if (sync-- == 0) goto finishClk2;
		/* Something appears to be happening, so reset sync count */
		sync = 200000;
		while ((port_in(status) & maskval) == 0x00) 
	            if (sync-- == 0) goto finishClk2;
    
		for (rcnt = 0; rcnt < 5; rcnt++) 
		{
		    while (!(port_in(status) & maskval)) 
		        cnt++;
		    while ((port_in(status) & maskval)) 
		        cnt++;
		}
		
	      finishClk2:
		Clk2 = cnt;

		/* Re-enable the interrupts */
		sigprocmask(SIG_UNBLOCK, &sig2block, (sigset_t *)NULL);

		Clk = (int)(0.5 + (((float)28322) * Clk1) / Clk2);
#else	/* CHIPS_PROBE_TEXT_CLOCK */
		if (ctLCD) {
		    Clk = LCD_TXT_CLOCK_FREQ;
		} else {
		    Clk = CRT_TXT_CLOCK_FREQ;
		}
#endif
	    }
	    /* Find a set of register settings that will give this
	     * clock */
	    ctCalcClock(regs, Clk);
	}
    }
    return;
}

static void ctClockRestore(const unsigned char regs[])
{
    unsigned char temp, msr;
    int i;
    
#ifdef DEBUG
    fprintf(stderr,"CHIPS: ctClockRestore\n");
#endif

    msr = port_in(0x3CC); 			/* Select fixed clock */
    port_out_r(0x3C2, (msr & 0xFE) | ctVgaIOBaseFlag);

    if (ctisHiQV) {
	port_out_r(0x3D0,0x03);
	temp = port_in(0x3D1);
	port_out_r(0x3D1, ((temp & ~0xC) | 0x04)); /* Select alt fixed clk */
	temp = (regs[HiQVFR03] & 0xC) >> 2;
	if (temp == 3)
	    temp = 2;
	temp = temp << 2;
	for (i = 0; i < 4; i++) {
	    port_out_r(0x3D6,0xC0 + i + temp);
	    port_out_r(0x3D7,regs[VCLK(i)]);
	}
	port_out_r(0x3D0,0x03);
	temp = port_in(0x3D1);
	/* restore alternate clock select reg.  */
	port_out_r(0x3D1,((regs[HiQVFR03]&0xC) | (temp&~0xC)));  
    } else {
	port_out_r(0x3D6,0x54);			 /* Select alt fixed clk */
	temp = port_in(0x3D7);
	port_out_r(0x3D7,((temp & 0xF3) | 0x4));

	if ((CHIPSchipset == CT_535 || CHIPSchipset == CT_540 || 
	     CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
	     CHIPSchipset == CT_548 || CHIPSchipset == CT_4300) &&
	     (((regs[XR54] & 0xC) == 0xC) || ((regs[XR54] & 0xC) == 0x8))) {
	    port_out_r(0x3D6,0x33);
	    temp = port_in(0x3D7);
	    port_out_r(0x3D7,(temp & ~0x20));	/* Select Vclk */
	    port_out_r(0x3D6,0x30);
	    port_out_r(0x3D7,regs[VCLK(0)]);
	    port_out_r(0x3D6,0x31);
	    port_out_r(0x3D7,regs[VCLK(1)]);
	    port_out_r(0x3D6,0x32);
	    port_out_r(0x3D7,regs[VCLK(2)]);
	}
	port_out_r(0x3D6,0x33);    /* restore status of MCLK/VCLK select reg.*/
	temp = port_in(0x3D7);
	port_out_r(0x3D7,((regs[XR33] & 0x20) | (temp & ~0x20)));
	port_out_r(0x3D6,0x54);    /* restore alternate clock select reg.   */
	temp = port_in(0x3D7);
	port_out_r(0x3D7,((regs[XR54] & 0xC) | (temp & ~0xC)));
    }
    usleep(10000);
    port_out_r(0x3C2, (regs[MSR] & 0xC) | (msr & 0xFE) | ctVgaIOBaseFlag);
    usleep(10000);
}

static int CHIPS_matchProgrammableClock(int clock)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_matchProgrammableClock(%d)\n",clock);
#endif
    /* Basically we can program any valid clock */
    return clock;
}

static int CHIPS_mapClock(int bpp, int pixelclock)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_mapClock(%d, %d)\n",bpp, pixelclock);
#endif
    if (ctisHiQV) {
	return pixelclock;
    } else {
	switch (bpp) {
	  case 24:
	    return pixelclock*3;
	    break;
	  case 16:
	    return pixelclock*2;
	    break;
	  case 15:
	    return pixelclock*2;
	    break;
	  default:
	    return pixelclock;
	    break;
	}
    }
}

static int CHIPS_mapHorizontalCrtc(int bpp, int pixelclock, int htiming)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_mapHorizontalCrtc(%d, %d, %d)\n",bpp, pixelclock,
	   htiming);
#endif
    if (ctisHiQV) {
	return htiming;
    } else {
	switch (bpp) {
	  case 24:
	    return htiming*3;
	    break;
	  case 16:
	    return htiming*2;
	    break;
	  case 15:
	    return htiming*2;
	    break;
	  default:
	    return htiming;
	    break;
	}
    }
}

static void CHIPS_unlock(void)
{
    unsigned char tmp;
    
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_unlock\n");
#endif

   /* set registers so that we can program the controller */
    if (ctisHiQV) {
	port_outw_r(0x3D6, 0x0E);
    } else {
	port_outw_r(0x3D6, 0x10);
#if defined(seperated_read_write_bank)
	port_outw_r(0x3D6, (1 << 11) | 0x11);
#endif
	port_outw_r(0x3D6, 0x15);	       /* unprotect all registers */
	port_out_r(0x3D6, 0x14);
	tmp = port_in(0x3D7);
	port_out_r(0x3D7, (tmp & ~0x20)); /* enable vsync on ST01 */
    }
    port_out_r(vgaIOBase + 4, 0x11);
    tmp = port_in(vgaIOBase + 5);
    port_out_r(vgaIOBase + 5, (tmp & 0x7F)); /*group 0 protection off */

}

/*----------------------------------------------------------------------*/
/* Read and store chipset-specific registers				*/
/*----------------------------------------------------------------------*/
static int CHIPS_saveregs(unsigned char regs[])
{
    int i;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_saveregs\n");
#endif

    CHIPS_unlock();

    if (ctisHiQV) {
	for (i = 0; i < NUM_HiQVXRregs; i++) {
	    port_out_r(0x3D6,__svgalib_HiQVXRregs[i]);
	    regs[CHIPSREG_HiQVXR(i)] = port_in(0x3D7);
	}
	
	for (i = 0; i < NUM_HiQVFRregs; i++) {
	    port_out_r(0x3D0,__svgalib_HiQVFRregs[i]);
	    regs[CHIPSREG_HiQVFR(i)] = port_in(0x3D1);
	}
	
	for (i = 0; i < NUM_HiQVCRregs; i++) {
	    port_out_r(vgaIOBase + 4,__svgalib_HiQVCRregs[i]);
	    regs[CHIPSREG_HiQVCR(i)] = port_in(vgaIOBase + 5);
	}
    } else {
	for (i = 0; i < NUM_XRregs; i++) {
	    port_out_r(0x3D6,__svgalib_XRregs[i]);
	    regs[CHIPSREG_XR(i)] = port_in(0x3D7);
	}
    }
    ctClockSave(regs);
    return CHIPS_TOTAL_REGS - VGA_TOTAL_REGS;
}

/*----------------------------------------------------------------------*/
/* Set chipset-specific registers					*/
/*----------------------------------------------------------------------*/
static void CHIPS_setregs(const unsigned char regs[], int mode)
{
    int i,tmp;
    
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setregs(*, %d)\n", mode);
#endif

    CHIPS_unlock();

    while (((port_in(0x3DA)) & 0x08) == 0x08 );/* wait VSync off */
    while (((port_in(0x3DA)) & 0x08) == 0 );   /* wait VSync on  */
    port_outw_r(SEQ_I,0x07);              /* reset hsync - just in case...  */

    if (ctisHiQV) {
	for (i = 0; i < NUM_HiQVXRregs; i++) {
	    port_out_r(0x3D6,__svgalib_HiQVXRregs[i]);
            if (port_in(0x3D7) != regs[CHIPSREG_HiQVXR(i)])
              port_out_r(0x3D7,regs[CHIPSREG_HiQVXR(i)]);
	}
	    
	for (i = 0; i < NUM_HiQVFRregs; i++) {
	    if ((__svgalib_HiQVFRregs[i] == 0x40) || (__svgalib_HiQVFRregs[i] == 0x48)) {
		/* Be careful to leave stretching off */
		port_out_r(0x3D0,__svgalib_HiQVFRregs[i]);
		port_out_r(0x3D1,(regs[CHIPSREG_HiQVFR(i)]&0xFE));
	    } else if (__svgalib_HiQVFRregs[i] == 0x3) {
                /* Leave clock bits alone */
                port_out_r(0x3D0,3);
                tmp = port_in(0x3D1);
                port_out_r(0x3D0,3);
                port_out_r(0x3D1,(regs[CHIPSREG_HiQVFR(i)] & 0xC3) | (tmp & ~0xC3));
            } else {
		port_out_r(0x3D0,__svgalib_HiQVFRregs[i]);
                if (port_in(0x3D1) != regs[CHIPSREG_HiQVFR(i)])
                  port_out_r(0x3D1,regs[CHIPSREG_HiQVFR(i)]);
	    }
	}

	for (i = 0; i < NUM_HiQVCRregs; i++) {
	    port_out_r(vgaIOBase + 4,__svgalib_HiQVCRregs[i]);
	    port_out_r(vgaIOBase + 5,regs[CHIPSREG_HiQVCR(i)]);
	}

    } else {
	for (i = 0; i < NUM_XRregs; i++) {
	    if ((__svgalib_XRregs[i] == 0x55) || (__svgalib_XRregs[i] == 0x57)) {
		/* Be careful to leave stretching off */
		port_out_r(0x3D6,__svgalib_XRregs[i]);
		port_out_r(0x3D7,(regs[CHIPSREG_XR(i)]&0xFE));
            } else if (__svgalib_XRregs[i] == 0x54) {
                port_out_r(0x3D6, 0x54);
                tmp = port_in(0x3D7);   /* restore the non clock bits  */
                port_out_r(0x3D6, 0x54);  /* Don't touch alternate clock sel. reg. */
                port_out_r(0x3D7, ((regs[CHIPSREG_XR(i)] & 0xF3) | (tmp & ~0xF3)));
	    } else {
		port_out_r(0x3D6,__svgalib_XRregs[i]);
		port_out_r(0x3D7,regs[CHIPSREG_XR(i)]);
	    }
	}
    }    
    ctClockRestore(regs);

    /* Turn the stretching back on if needed.  */
    if (ctisHiQV) {
	/* Why Twice? It seems that strecthing on the text console is not
	 * always restored well even though the register contain the right
	 * values. Restoring them twice is a work around. */
	port_out_r(0x3D0, 0x40);
	port_out_r(0x3D1, regs[HiQVFR40]);
	port_out_r(0x3D0, 0x48);
	port_out_r(0x3D1, regs[HiQVFR48]);
	usleep(10000);
	port_out_r(0x3D0, 0x40);
	port_out_r(0x3D1, regs[HiQVFR40]);
	port_out_r(0x3D0, 0x48);
	port_out_r(0x3D1, regs[HiQVFR48]);
    } else {
	port_out_r(0x3D6, 0x55);
	port_out_r(0x3D7, regs[XR55]);
	port_out_r(0x3D6, 0x57);
	port_out_r(0x3D7, regs[XR57]);
    }
    usleep(10000);
}



/*----------------------------------------------------------------------*/
/* Return nonzero if mode is available					*/
/*----------------------------------------------------------------------*/
static int CHIPS_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_modeavailable(%d)\n", mode);
#endif

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (video_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 0;
    }
    free(modetiming);
    free(modeinfo);

    return SVGADRV;
}

/* Set a mode */

/* Local, called by CHIPS_setmode(). */

static void CHIPS_initializemode(unsigned char * moderegs,
			    ModeTiming * mode, ModeInfo * modeinfo)
{
    unsigned char tmp;
    static int HSyncStart, HDisplay;
    int lcdHTotal, lcdHDisplay;
    int lcdVTotal, lcdVDisplay;
    int lcdHRetraceStart, lcdHRetraceEnd;
    int lcdVRetraceStart, lcdVRetraceEnd;
    int CrtcHDisplay;
    int temp;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_initializemode\n");
#endif

    /* Get current values. Must be called before CalcClock */
    CHIPS_saveregs(moderegs);

    /* Set up the standard VGA registers for a generic SVGA. */
    __svgalib_setup_VGA_registers(moderegs, mode, modeinfo);

    /* store orig. HSyncStart needed for flat panel mode */
    HSyncStart = mode->CrtcHSyncStart / modeinfo->bytesPerPixel - 16;
    HDisplay = (mode->CrtcHDisplay + 1) / modeinfo->bytesPerPixel;

    /* init clock */
    if ((CHIPSchipset == CT_520) || (CHIPSchipset == CT_525) || 
	(CHIPSchipset == CT_530)) {
	ctHWCalcClock(moderegs,mode->selectedClockNo);
    } else {
	ctCalcClock(moderegs,mode->programmedClock);
    }

    moderegs[VGA_AR10] = 0x01;   /* mode */
    moderegs[VGA_AR11] = 0x00;   /* overscan (border) color */
    moderegs[VGA_AR12] = 0x0F;   /* enable all color planes */
    moderegs[VGA_AR13] = 0x00;   /* horiz pixel panning 0 */
    moderegs[VGA_GR5] = 0x00;   /* normal read/write mode */
    moderegs[VGA_CR13] = ((mode->CrtcHDisplay) >> 3) & 0xFF ;
    moderegs[XR1E] = ((mode->CrtcHDisplay) >> 3) & 0xFF ;
    moderegs[XR0D] = (((mode->CrtcHDisplay) >> 11) & 0x1)  |
        (((mode->CrtcHDisplay) >> 10) & 0x2);

    moderegs[XR04] |= 4;	/* enable addr counter bits 16-17 */
#if defined(seperated_read_write_bank)
    moderegs[XR0B] = moderegs[XR0B] | 0x7;	/* extended mode, dual maps */
#else
    moderegs[XR0B] = (moderegs[XR0B] & 0xF8) | 0x5; /* single maps */
#endif
    moderegs[XR28] |= 0x10;	/* 256-colour video */

    /* Setup extended display timings */
    if (ctCRT) {
	/* in CRTonly mode this is simple: only set overflow for CR00-CR06 */
	moderegs[XR17] = ((((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8)
	    | ((((mode->CrtcHDisplay >> 3) - 1) & 0x100) >> 7)
	    | ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 6)
	    | ((((mode->CrtcHSyncEnd >> 3)) & 0x20) >> 2)
	    | ((((mode->CrtcHSyncStart >> 3) - 1) & 0x100) >> 4)
	    | (((mode->CrtcHSyncEnd >> 3) & 0x40) >> 1);
    } else {
	/* horizontal timing registers */
	/* in LCD/dual mode use saved bios values to derive timing values if
	 * not told otherwise*/
	if (!(ctFlagsSet & ctFlags_UseModeline)) {
	    lcdHTotal = __svgalib_ctSize.HTotal;
	    lcdHRetraceStart = __svgalib_ctSize.HRetraceStart;
	    lcdHRetraceEnd = __svgalib_ctSize.HRetraceEnd;
	    if (modeinfo->bitsPerPixel == 16) {
		lcdHRetraceStart <<= 1;
		lcdHRetraceEnd <<= 1;
		lcdHTotal <<= 1;
	    } else if (modeinfo->bitsPerPixel == 24) {
		lcdHRetraceStart += (lcdHRetraceStart << 1);
		lcdHRetraceEnd += (lcdHRetraceEnd << 1);
		lcdHTotal += (lcdHTotal << 1);
	    }
 	    lcdHRetraceStart -=8;       /* HBlank =  HRetrace - 1: for */
 	    lcdHRetraceEnd   -=8;       /* compatibility with vgaHW.c  */
	} else {
	    /* use modeline values if bios values don't work */
	    lcdHTotal = mode->CrtcHTotal;
	    lcdHRetraceStart = mode->CrtcHSyncStart;
	    lcdHRetraceEnd = mode->CrtcHSyncEnd;
	}
	/* The chip takes the size of the visible display area from the
	 * CRTC values. We use bios screensize for LCD in LCD/dual mode
	 * wether or not we use modeline for LCD. This way we can specify
	 * always specify a smaller than default display size on LCD
	 * by writing it to the CRTC registers. */
	lcdHDisplay = __svgalib_ctSize.HDisplay;
	if (modeinfo->bitsPerPixel == 16) {
	    lcdHDisplay++;
	    lcdHDisplay <<= 1;
	    lcdHDisplay--;
	} else if (modeinfo->bitsPerPixel == 24) {
	    lcdHDisplay++;
	    lcdHDisplay += (lcdHDisplay << 1);
	    lcdHDisplay--;
	}
	lcdHTotal = (lcdHTotal >> 3) - 5;
	lcdHDisplay = (lcdHDisplay >> 3) - 1;
	lcdHRetraceStart = (lcdHRetraceStart >> 3);
	lcdHRetraceEnd = (lcdHRetraceEnd >> 3);
	/* This ugly hack is needed because CR01 and XR1C share the 8th bit!*/
	CrtcHDisplay = ((mode->CrtcHDisplay >> 3) - 1);
	if((lcdHDisplay & 0x100) != ( CrtcHDisplay & 0x100)){
	  fprintf(stderr,"This display configuration might cause problems !\n");
	  lcdHDisplay = 255;}

	/* now init register values */
	moderegs[XR17] = (((lcdHTotal) & 0x100) >> 8)
	    | ((lcdHDisplay & 0x100) >> 7)
	    | ((lcdHRetraceStart & 0x100) >> 6)
	    | (((lcdHRetraceEnd) & 0x20) >> 2);
	moderegs[XR19] = lcdHRetraceStart & 0xFF;
	moderegs[XR1A] = lcdHRetraceEnd & 0x1F;
	moderegs[XR1B] = lcdHTotal & 0xFF;
	moderegs[XR1C] = lcdHDisplay & 0xFF;

	if (ctFlagsSet & ctFlags_UseModeline) {
	    /* for ext. packed pixel mode on 64520/64530 */
	    /* no need to rescale: used only in 65530    */
	    moderegs[XR21] = lcdHRetraceStart & 0xFF;
	    moderegs[XR22] = lcdHRetraceEnd & 0x1F;
	    moderegs[XR23] = lcdHTotal & 0xFF;

	    /* vertical timing registers */
	    lcdVTotal = mode->CrtcVTotal - 2;
	    lcdVDisplay = __svgalib_ctSize.VDisplay - 1;
	    lcdVRetraceStart = mode->CrtcVSyncStart;
	    lcdVRetraceEnd = mode->CrtcVSyncEnd;

	    moderegs[XR64] = lcdVTotal & 0xFF;
	    moderegs[XR66] = lcdVRetraceStart & 0xFF;
	    moderegs[XR67] = lcdVRetraceEnd & 0x0F;
	    moderegs[XR68] = lcdVDisplay & 0xFF;
	    moderegs[XR65] = ((lcdVTotal & 0x100) >> 8)
		| ((lcdVDisplay & 0x100) >> 7)
		| ((lcdVRetraceStart & 0x100) >> 6)
		| ((lcdVRetraceStart & 0x400) >> 7)
		| ((lcdVTotal & 0x400) >> 6)
		| ((lcdVTotal & 0x200) >> 4)
		| ((lcdVDisplay & 0x200) >> 3)
		| ((lcdVRetraceStart & 0x200) >> 2);

	    /* 
	     * These are important: 0x2C specifies the numbers of lines 
	     * (hsync pulses) between vertical blank start and vertical 
	     * line total, 0x2D specifies the number of clock ticks? to
	     * horiz. blank start ( caution ! 16bpp/24bpp modes: that's
	     * why we need HSyncStart - can't use mode->CrtcHSyncStart) 
	     */
	    tmp = ((__svgalib_ctPanelType == DD) && !(moderegs[XR6F] & 0x02))
	      ? 1 : 0; /* double LP delay, FLM: 2 lines iff DD+no acc*/
	    /* Currently we support 2 FLM scemes: #1: FLM coincides with
	     * VTotal ie. the delay is programmed to the difference bet-
	     * ween lctVTotal and lcdVRetraceStart.    #2: FLM coincides
	     * lcdVRetraceStart - in this case FLM delay will be turned
	     * off. To decide which sceme to use we compare the value of
	     * XR2C set by the bios to the two scemes. The one that fits
	     * better will be used.
	     */
	    if (moderegs[XR2C] 
		< abs((__svgalib_ctSize.VTotal - __svgalib_ctSize.VRetraceStart 
		       - tmp - 1) - moderegs[XR2C]))
	      	    moderegs[XR2F] |= 0x80;   /* turn FLM delay off */
	    moderegs[XR2C] = lcdVTotal - lcdVRetraceStart - tmp;
	    /*moderegs[XR2D] = (HSyncStart >> (3 - tmp)) & 0xFF;*/
	    moderegs[XR2D] = (HDisplay >> (3 - tmp)) & 0xFF;
	    moderegs[XR2F] = (moderegs[XR2F] & 0xDF)
		| (((HSyncStart >> (3 - tmp)) & 0x100) >> 3);
	}

	if ((ctFlagsSet & ctFlags_StretchEnable) ||
	    (ctFlagsSet & ctFlags_StretchDisable) ||
	    (ctFlagsSet & ctFlags_CenterEnable) ||
	    (ctFlagsSet & ctFlags_CenterDisable)) {
	    moderegs[XR51] |= 0x40;		/* enable FP compensation */
	    moderegs[XR55] |= 0x01;		/* enable horiz compensation */
	    moderegs[XR57] |= 0x01;		/* enable vert compensation */
	    moderegs[XR57] &= 0x7F;		/* disable fast-centre */
	    moderegs[XR58] = 0;
	}

	/* Screen Centering */
        moderegs[XR57]|=0x01;
	if (ctFlagsSet & ctFlags_CenterEnable) { 
	    moderegs[XR57] |= 0x02;		/* enable v-centring */
	    if (mode->CrtcHDisplay < 1489)	/* HWBug */
	        moderegs[XR55] |= 0x02;		/* enable h-centring */
	    else if (modeinfo->bitsPerPixel == 24) {
		moderegs[XR55] &= ~0x02;
	        moderegs[XR56] = (lcdHDisplay - CrtcHDisplay) >> 1;
	    }
	} else if (ctFlagsSet & ctFlags_CenterDisable) { 
	    moderegs[XR55] &= 0xFD;		/* disable h-centering    */
	    moderegs[XR56] = 0;
	    moderegs[XR57] &= 0xFD;		/* disable v-centering    */
	}

	/* Screen Stretching */
	if (ctFlagsSet & ctFlags_StretchEnable) {
	    moderegs[XR55] |= 0x20;		/* h-comp on, h-double off */
	    moderegs[XR57] |= 0x60;		/* vertical stretching on */
	    fprintf(stderr,"0x%X, 0x%X\n", mode->flags, DOUBLESCAN);
	    fprintf(stderr,"%d, %d\n", mode->CrtcVDisplay, __svgalib_ctSize.VDisplay);
	    if (2*mode->CrtcVDisplay <= __svgalib_ctSize.VDisplay) 
	    {
	        /* We assume that automatic double scanning occurs */
		if (2 * mode->CrtcVDisplay == __svgalib_ctSize.VDisplay)
		    temp = 0;
		else
	            temp = (( 2 * mode->CrtcVDisplay) / (__svgalib_ctSize.VDisplay -
					      (2 * mode->CrtcVDisplay)));
	    } else {
		if (mode->CrtcVDisplay == __svgalib_ctSize.VDisplay)
		    temp = 0;
		else
	            temp = (mode->CrtcVDisplay / (__svgalib_ctSize.VDisplay -
					      mode->CrtcVDisplay));
	    }
	    moderegs[XR5A] = temp > 0x0F ? 0 : (unsigned char)temp;
	} else if (ctFlagsSet & ctFlags_StretchDisable) {
	    moderegs[XR55] &= 0xDF;
	    moderegs[XR57] &= 0x9F;
	}
    }

    moderegs[XR03] |= 0x02;   /* 32 bit I/O enable etc.          */
    moderegs[XR07] = 0xF4;    /* 32 bit I/O port selection       */
    moderegs[XR03] |= 0x08;   /* High bandwidth on 65548         */
    moderegs[XR40] = 0x01;    /*BitBLT Draw Mode for 8 and 24 bpp*/
  
    moderegs[XR52] |= 0x01;   /* Refresh count                   */
    moderegs[XR0F] &= 0xEF;   /* not Hi-/True-Colour             */
    moderegs[XR02] |= 0x01;   /* 16bit CPU Memory Access         */
    moderegs[XR02] &= 0xE3;   /* Attr. Cont. default access      */
                              /* use ext. regs. for hor. in dual */
    moderegs[XR06] &= 0xF3;   /* bpp clear                       */

    if (PCIcard)
        moderegs[XR03] |= 0x40;	/* PCI Burst for 65548 */

    /* sync. polarities */
    if ((mode->flags & (PHSYNC | NHSYNC))
	&& (mode->flags & (PVSYNC | NVSYNC))) {
	if (mode->flags & (PHSYNC | NHSYNC)) {
	    if (mode->flags & PHSYNC) {
		moderegs[XR55] &= 0xBF;	/* CRT Hsync positive */
	    } else {
		moderegs[XR55] |= 0x40;	/* CRT Hsync negative */
	    }
	}
	if (mode->flags & (PVSYNC | NVSYNC)) {
	    if (mode->flags & PVSYNC) {
		moderegs[XR55] &= 0x7F;	/* CRT Vsync positive */
	    } else {
		moderegs[XR55] |= 0x80;	/* CRT Vsync negative */
	    }
	}
    }

    if (modeinfo->bitsPerPixel == 16) {
	moderegs[XR06] |= 0xC4;   /*15 or 16 bpp colour         */
	moderegs[XR0F] |= 0x10;   /*Hi-/True-Colour             */
	moderegs[XR40] = 0x02;    /*BitBLT Draw Mode for 16 bpp */
	if (modeinfo->greenWeight != 5)
	    moderegs[XR06] |= 0x08;	/*16bpp              */
    } else if (modeinfo->bitsPerPixel == 24) {
	moderegs[XR06] |= 0xC8;   /*24 bpp colour               */
        moderegs[XR0F] |= 0x10;   /*Hi-/True-Colour             */
	if (ctFlagsSet & ctFlags_Use18BitBus) { 
	    moderegs[XR50] &= 0x7F;   /*18 bit TFT data width   */
	} else {
	    moderegs[XR50] |= 0x80;   /*24 bit TFT data width   */
	}
    }

    /* STN specific */
    if (IS_STN(__svgalib_ctPanelType)) {
	moderegs[XR50] &= ~0x03;  /* FRC clear                  */
	moderegs[XR50] |= 0x01;   /* 16 frame FRC               */
	moderegs[XR50] &= ~0x0C;  /* Dither clear               */
	moderegs[XR50] |= 0x08;   /* Dither all modes           */
 	if (CHIPSchipset == CT_548) {
	    moderegs[XR03] |= 0x20; /* CRT I/F priority           */
	    moderegs[XR04] |= 0x10; /* RAS precharge 65548        */
	}
    }
}

static void CHIPS_HiQV_initializemode(unsigned char *moderegs,
			    ModeTiming * mode, ModeInfo * modeinfo)
{
    int lcdHTotal, lcdHDisplay;
    int lcdVTotal, lcdVDisplay;
    int lcdHRetraceStart, lcdHRetraceEnd;
    int lcdVRetraceStart, lcdVRetraceEnd;
    int lcdHSyncStart;
    unsigned int temp;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_HiQV_initializemode\n");
#endif

    /* Get current values. Must be called before CalcClock */
    CHIPS_saveregs(moderegs);

    /* Set up the standard VGA registers for a generic SVGA. */
    __svgalib_setup_VGA_registers(moderegs, mode, modeinfo);

    /* init clock */
#ifdef DEBUG
    fprintf(stderr,"CHIPS: pixelclock used: %d\n",mode->pixelClock);
#endif
    ctCalcClock(moderegs,mode->pixelClock);

    moderegs[VGA_AR10] = 0x01;   /* mode */
    moderegs[VGA_AR11] = 0x00;   /* overscan (border) color */
    moderegs[VGA_AR12] = 0x0F;   /* enable all color planes */
    moderegs[VGA_AR13] = 0x00;   /* horiz pixel panning 0 */
    moderegs[VGA_GR5] = 0x00;    /* normal read/write mode */

    temp = mode->CrtcHDisplay >> 3;     /* bytes per scan line, 256 color mode */
    if (modeinfo->bitsPerPixel == 24) {
      temp += temp << 1;                /* -> x3 in 16M color mode */
    } else if (modeinfo->bitsPerPixel == 16) {
      temp <<= 1;                       /* or x2 in 64K color mode */
    }
    moderegs[VGA_CR13] = (temp) & 0xFF;     /* bytes of fb memory per scan line */
    moderegs[HiQVCR41] = (temp >> 8) & 0xF; /* upper 4 bits of it, as HiQVXR09[0] is set to 1 */

    moderegs[HiQVXR0A] |= 0x1;	  /* Paging mode enabled, nolinear */
    moderegs[HiQVXR09] |= 0x1;	  /* Enable extended CRT registers */
    moderegs[HiQVXR0E] = 0;	  /* Single map */
    moderegs[HiQVXR40] |= 0x3;	  /* High Resolution. XR40[1] reserved? */
    moderegs[HiQVXR81] &= 0xF8;   /* 256 Color Video */
    moderegs[HiQVXR81] |= 0x2;
    moderegs[HiQVXR80] |= 0x10;   /* Enable cursor output on P0 and P1 */
    moderegs[HiQVXR20] = 0x0;     /* BitBLT Draw mode for 8bpp */

    /* panel timing */
    /* By default don't set panel timings, but allow it as an option */
    if (ctFlagsSet & ctFlags_UseModeline) {
	lcdHTotal = (mode->CrtcHTotal >> 3) - 5;
	lcdHDisplay = (__svgalib_ctSize.HDisplay >> 3) - 1;
	lcdHRetraceStart = (mode->CrtcHSyncStart >> 3);
	lcdHRetraceEnd = (mode->CrtcHSyncEnd >> 3);
	lcdHSyncStart = lcdHRetraceStart - 2;

	lcdVTotal = mode->CrtcVTotal - 2;
	lcdVDisplay = __svgalib_ctSize.VDisplay - 1;
	lcdVRetraceStart = mode->CrtcVSyncStart;
	lcdVRetraceEnd = mode->CrtcVSyncEnd;

#ifdef MODELINE_DEBUG
        fprintf(stderr,"lcdHTotal = %d, lcdHDisplay = %d, lcdHRetraceStart = %d\n",
               lcdHTotal,lcdHDisplay,lcdHRetraceStart);
        fprintf(stderr,"lcdHRetraceEnd = %d, lcdHSyncStart = %d\n",lcdHRetraceEnd,lcdHSyncStart);
        fprintf(stderr,"lcdVTotal = %d, lcdVDisplay = %d, lcdVretraceStart = %d\n",
               lcdVTotal,lcdVDisplay,lcdVRetraceStart);

        fprintf(stderr,"before:\n");
        fprintf(stderr,"#20 = %02X, #21 = %02X, #22 = %02X, #23 = %02X, #24 = %02X, #25 = %02X\n",
               moderegs[HiQVFR20],moderegs[HiQVFR21],moderegs[HiQVFR22],moderegs[HiQVFR23],
               moderegs[HiQVFR24],moderegs[HiQVFR25]);
        fprintf(stderr,"#26 = %02X, #27 = %02X\n",
               moderegs[HiQVFR26],moderegs[HiQVFR27]);
        fprintf(stderr,"#30 = %02X, #31 = %02X, #32 = %02X, #33 = %02X, #34 = %02X, #35 = %02X\n",
               moderegs[HiQVFR30],moderegs[HiQVFR31],moderegs[HiQVFR32],moderegs[HiQVFR33],
               moderegs[HiQVFR34],moderegs[HiQVFR35]);
        fprintf(stderr,"#36 = %02X, #37 = %02X\n",
               moderegs[HiQVFR36],moderegs[HiQVFR37]);
#endif

	moderegs[HiQVFR20] = lcdHDisplay & 0xFF;
	moderegs[HiQVFR21] = lcdHRetraceStart & 0xFF;
	moderegs[HiQVFR25] = ((lcdHRetraceStart & 0xF00) >> 4) |
	    ((lcdHDisplay & 0xF00) >> 8);
	moderegs[HiQVFR22] = lcdHRetraceEnd & 0x1F;
	moderegs[HiQVFR23] = lcdHTotal & 0xFF;
	moderegs[HiQVFR24] = (lcdHSyncStart >> 3) & 0xFF;
	moderegs[HiQVFR26] = (moderegs[HiQVFR26] & ~0x1F)
	    | ((lcdHTotal & 0xF00) >> 8)
	    | (((lcdHSyncStart >> 3) & 0x100) >> 4);
	moderegs[HiQVFR27] &= 0x7F;

	moderegs[HiQVFR30] = lcdVDisplay & 0xFF;
	moderegs[HiQVFR31] = lcdVRetraceStart & 0xFF;
	moderegs[HiQVFR35] = ((lcdVRetraceStart & 0xF00) >> 4)
	    | ((lcdVDisplay & 0xF00) >> 8);
	moderegs[HiQVFR32] = lcdVRetraceEnd & 0x0F;
	moderegs[HiQVFR33] = lcdVTotal & 0xFF;
	moderegs[HiQVFR34] = (lcdVTotal - lcdVRetraceStart) & 0xFF;
	moderegs[HiQVFR36] = ((lcdVTotal & 0xF00) >> 8) |
	    (((lcdVTotal - lcdVRetraceStart) & 0x700) >> 4);
	moderegs[HiQVFR37] |= 0x80;

#ifdef MODELINE_DEBUG
        fprintf(stderr,"after:\n");
        fprintf(stderr,"#20 = %02X, #21 = %02X, #22 = %02X, #23 = %02X, #24 = %02X, #25 = %02X\n",
               moderegs[HiQVFR20],moderegs[HiQVFR21],moderegs[HiQVFR22],moderegs[HiQVFR23],
               moderegs[HiQVFR24],moderegs[HiQVFR25]);
        fprintf(stderr,"#26 = %02X, #27 = %02X\n",
               moderegs[HiQVFR26],moderegs[HiQVFR27]);
        fprintf(stderr,"#30 = %02X, #31 = %02X, #32 = %02X, #33 = %02X, #34 = %02X, #35 = %02X\n",
               moderegs[HiQVFR30],moderegs[HiQVFR31],moderegs[HiQVFR32],moderegs[HiQVFR33],
               moderegs[HiQVFR34],moderegs[HiQVFR35]);
        fprintf(stderr,"#36 = %02X, #37 = %02X\n",
               moderegs[HiQVFR36],moderegs[HiQVFR37]);
#endif
    }

    /* Set up the extended CRT registers of the HiQV32 chips */
    moderegs[HiQVCR30] = ((mode->CrtcVTotal - 2) & 0xF00) >> 8;
    moderegs[HiQVCR31] = ((mode->CrtcVDisplay - 1) & 0xF00) >> 8;
    moderegs[HiQVCR32] = (mode->CrtcVSyncStart & 0xF00) >> 8;
    moderegs[HiQVCR33] = (mode->CrtcVSyncStart & 0xF00) >> 8;
    if (CHIPSchipset == CT_9000) {
	/* The 69000 has overflow bits for the horizontal values as well */
	moderegs[HiQVCR38] = (((mode->CrtcHTotal >> 3) - 5) & 0x100) >> 8;
	moderegs[HiQVCR3C] = ((mode->CrtcHSyncEnd >> 3) & 0xC0);
    }

    /* Screen Centring */
    if (ctFlagsSet & ctFlags_CenterEnable) {
	moderegs[HiQVFR40] |= 0x3;     /* Enable Horizontal centering */
	moderegs[HiQVFR48] |= 0x3;     /* Enable Vertical centering */
    } else if (ctFlagsSet & ctFlags_CenterDisable) {
	moderegs[HiQVFR40] &= 0xFD;    /* Disable Horizontal centering */
	moderegs[HiQVFR48] &= 0xFD;    /* Disable Vertical centering */
    }

    /* Screen Stretching */
    if (ctFlagsSet & ctFlags_StretchEnable) {
	moderegs[HiQVFR40] |= 0x21;    /* Enable Horizontal stretching */
	moderegs[HiQVFR48] |= 0x05;    /* Enable Vertical stretching */
    } else if (ctFlagsSet & ctFlags_StretchDisable) {
	moderegs[HiQVFR40] &= 0xDF;    /* Disable Horizontal stretching */
	moderegs[HiQVFR48] &= 0xFB;    /* Disable Vertical stretching */
    }

    moderegs[HiQVXRE2] = ct_video_mode(modeinfo->bitsPerPixel,
                                       modeinfo->greenWeight,
                                       __svgalib_ctSize.HDisplay);

    /* sync. polarities */
    if ((mode->flags & (PHSYNC | NHSYNC))
	&& (mode->flags & (PVSYNC | NVSYNC))) {
	if (mode->flags & (PHSYNC | NHSYNC)) {
	    if (mode->flags & PHSYNC)
		moderegs[HiQVFR08] &= 0xBF;	/* Alt. CRT Hsync positive */
	    else
		moderegs[HiQVFR08] |= 0x40;	/* Alt. CRT Hsync negative */
	}
	if (mode->flags & (PVSYNC | NVSYNC)) {
	    if (mode->flags & PVSYNC)
	        moderegs[HiQVFR08] &= 0x7F;	/* Alt. CRT Vsync positive */
	    else
		moderegs[HiQVFR08] |= 0x80;	/* Alt. CRT Vsync negative */
	}
    }

    if (modeinfo->bitsPerPixel == 16) {
	moderegs[HiQVXR81] = (moderegs[HiQVXR81] & 0xF0) | 0x4; /* 15bpp */
	moderegs[HiQVFR10] |= 0x0C;   /*Colour Panel                 */
	moderegs[HiQVXR20] = 0x10;    /*BitBLT Draw Mode for 16 bpp  */
	if (modeinfo->greenWeight != 5)
	    moderegs[HiQVXR81] |= 0x01;	/*16bpp */
    } else if (modeinfo->bitsPerPixel == 24) {
	moderegs[HiQVXR81] = (moderegs[HiQVXR81] & 0xF0) | 0x6; /* 24bpp */
	moderegs[HiQVXR20] = 0x20;    /*BitBLT Draw Mode for 24 bpp */
    }

    /* STN specific */
    if (IS_STN(__svgalib_ctPanelType)) {
	moderegs[HiQVFR11] &= ~0x03;/* FRC clear                    */
	moderegs[HiQVFR11] &= ~0x8C;/* Dither clear                 */
	moderegs[HiQVFR11] |= 0x01;	/* 16 frame FRC                 */
	moderegs[HiQVFR11] |= 0x84;	/* Dither                       */
	if (CHIPSchipset == CT_555 || CHIPSchipset == CT_8554 ||
		CHIPSchipset == CT_9000) {
	    moderegs[HiQVFR73] &= 0x4f;
	    moderegs[HiQVFR73] |= 0x80;
	    moderegs[HiQVFR73] |= 0x30;
	}

	if (__svgalib_ctPanelType == DD)	/* Shift Clock Mask. Use to get */
	  moderegs[HiQVFR12] |= 0x4;/* rid of line in DSTN screens  */
    }

}


/*----------------------------------------------------------------------*/
/* Check if mode is interlaced						*/
/*----------------------------------------------------------------------*/
static int CHIPS_interlaced(int mode)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_interlaced(%d)\n",mode);
#endif
	/* This driver does not support interlaced mode */
	return FALSE;
}


/*----------------------------------------------------------------------*/
/* Set a mode								*/
/*----------------------------------------------------------------------*/
static int CHIPS_setmode(int mode, int prv_mode)
{
	unsigned char *regs;
	ModeInfo *modeinfo;
	ModeTiming *modetiming;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setmode(%d, %d)\n", mode, prv_mode);
#endif

        if (CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
	     CHIPSchipset == CT_548 || CHIPSchipset == CT_550 ||
	     CHIPSchipset == CT_554 || CHIPSchipset == CT_555 ||
	     CHIPSchipset == CT_8554 || CHIPSchipset == CT_9000 ||
	     CHIPSchipset == CT_4300) {
	    __svgalib_driverspecs->accelspecs->operations = 0;
	    __svgalib_driverspecs->accelspecs->ropOperations = 0;
	    __svgalib_driverspecs->accelspecs->transparencyOperations = 0;
	    __svgalib_driverspecs->accelspecs->ropModes = 0;
	    __svgalib_driverspecs->accelspecs->transparencyModes = 0;
	}

	if (!CHIPS_modeavailable(mode))
	{	return 1;
	}

	if (IS_IN_STANDARD_VGA_DRIVER(mode))
	    return (int) (__svgalib_vga_driverspecs.setmode(mode, prv_mode));

	modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);
	modetiming = malloc(sizeof(ModeTiming));
	if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	    free(modetiming);
	    free(modeinfo);
	    return 1;
	}

#ifdef MODELINE_DEBUG
        /*------------------------------------*/
        fprintf(stderr,"Modetiming:\n");
        fprintf(stderr,"pixelclock - %d, HDisplay - %d, HSyncStart - %d, HSyncEnd - %d, HTotal - %d\n",
               modetiming->pixelClock,modetiming->HDisplay,modetiming->HSyncStart,
               modetiming->HSyncEnd,modetiming->HTotal);
        fprintf(stderr,"VDisplay - %d, VSyncStart - %d, VSyncEnd - %d, VTotal - %d, flags - %d\n",
               modetiming->VDisplay,modetiming->VSyncStart,modetiming->VSyncEnd,
               modetiming->VTotal,modetiming->flags);
        fprintf(stderr,"programmedClock - %d, selectedClockNo - %d, CrtcHDisplay - %d\n",
               modetiming->programmedClock,modetiming->selectedClockNo,modetiming->CrtcHDisplay);
        fprintf(stderr,"CrtcHSyncStart - %d, CrtcHSyncEnd - %d, CrtcHDisplay - %d, CrtcHTotal - %d\n",
               modetiming->CrtcHSyncStart,modetiming->CrtcHSyncEnd,modetiming->CrtcHDisplay,
               modetiming->CrtcHTotal);
        fprintf(stderr,"CrtcVSyncStart - %d, CrtcVSyncEnd - %d, CrtcVTotal - %d\n\n",
               modetiming->CrtcVSyncStart,modetiming->CrtcVSyncEnd,modetiming->CrtcVTotal);

        fprintf(stderr,"ModeInfo:\n");
        fprintf(stderr,"width - %d, height - %d, bytesPerPixel - %d, bitsPerPixel - %d\n",
               (int)modeinfo->width,(int)modeinfo->height,(int)modeinfo->bytesPerPixel,
               (int)modeinfo->bitsPerPixel);
        fprintf(stderr,"colorBits - %d, redWeight - %d, greenWeight - %d, blueWeight - %d\n",
               (int)modeinfo->colorBits,(int)modeinfo->redWeight,(int)modeinfo->greenWeight,
               (int)modeinfo->blueWeight);
        fprintf(stderr,"redOffset - %d, blueOffset - %d, greenOffset - %d\n",
               (int)modeinfo->redOffset,(int)modeinfo->blueOffset,
               (int)modeinfo->greenOffset);
        fprintf(stderr,"redMask - %d, blueMask - %d, greenMask - %d, lineWidth - %d\n",
               (int)modeinfo->redMask,(int)modeinfo->blueMask,(int)modeinfo->greenMask,
               modeinfo->lineWidth);
        fprintf(stderr,"realWidth - %d, realHeight - %d, flags - %d\n\n",
               (int)modeinfo->realWidth,(int)modeinfo->realHeight,modeinfo->flags);

        fprintf(stderr,"CardSpecs:\n");
        fprintf(stderr,"videoMemory - %d, maxPixelClock8bpp - %d, flags - %d\n",
               cardspecs->videoMemory,cardspecs->maxPixelClock8bpp,
               cardspecs->flags);
        fprintf(stderr,"nClocks - %d, maxHorizontalCrtc - %d, mapClock - %p\n",
               cardspecs->nClocks,cardspecs->maxHorizontalCrtc,
               cardspecs->mapClock);
        fprintf(stderr,"matchProgrammableClock - %p, mapHorizontalCrtc - %p\n",
               cardspecs->matchProgrammableClock,cardspecs->mapHorizontalCrtc);

        /*------------------------------------*/
#endif

	regs = malloc(CHIPS_TOTAL_REGS);
	if (ctisHiQV) {
	    CHIPS_HiQV_initializemode(regs, modetiming, modeinfo);
	} else {
	    CHIPS_initializemode(regs, modetiming, modeinfo);
	}
	free(modetiming);

	__svgalib_setregs(regs);
	CHIPS_setregs(regs, mode);

	if (ctFlagsSet & ctFlags_NoBitBlt) return 0;

	if (CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
	    CHIPSchipset == CT_548) {
	    if (PCIcard) {
		/* We are a PCI machine */
		if (__svgalib_modeinfo_linearset & IS_LINEAR) {
		    /* Linear addressing is enabled. So we can use
		     * MMIO with linear addressing. Map the required
		     * memory space
		     */
		    ctMMIO = TRUE;
		    __svgalib_ctMMIOPage = -1;
		    if (__svgalib_ctMMIOBase == NULL) {
			__svgalib_ctMMIOBase = MMIO_mem1;
		    }
		} else {
		    if (CHIPSchipset == CT_545 || CHIPSchipset == CT_546) {
			/* We are a 65545 or 65546 PCI machine. We only
			 * support acceleration on these machines with MMIO.
			 * Hence we have to use page mode to access the
			 * MMIO registers.
			 */
			 ctMMIO = TRUE;
			 __svgalib_ctMMIOPage = 32; /* MMIO starts at 2MBytes */
			 __svgalib_ctMMIOBase = __svgalib_graph_mem;
		    } else {
			/* Let the register address acceleration handle this */
			ctMMIO = FALSE;
		    }
		}
	    }
	}
	
	if (ctisHiQV) {
	    if (__svgalib_modeinfo_linearset & IS_LINEAR) {
		/* Linear addressing is enabled. So we can use
		 * MMIO with linear addressing. Map the required
		 * memory space
		 */
		ctMMIO = TRUE;
		__svgalib_ctMMIOPage = -1;
		if (__svgalib_ctMMIOBase == NULL) {
		    __svgalib_ctMMIOBase=MMIO_mem2;
		}
		__svgalib_ctBltDataWindow = __svgalib_ctMMIOBase + 0x10000;
	    } else {
		/* Use paged addressing mode to program the MMIO registers */
		ctMMIO = TRUE;
		__svgalib_ctMMIOPage = 64; /* MMIO starts at 4MBytes */
		__svgalib_ctMMIOBase = __svgalib_graph_mem;
		__svgalib_ctBltDataWindow = __svgalib_graph_mem;
	    }
	}
	
	if (CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
	    CHIPSchipset == CT_548 || CHIPSchipset == CT_550 ||
	    CHIPSchipset == CT_554 || CHIPSchipset == CT_555 ||
	    CHIPSchipset == CT_8554 || CHIPSchipset == CT_9000 ||
	    CHIPSchipset == CT_4300) {
	    __svgalib_InitializeAcceleratorInterface(modeinfo);

	    __svgalib_driverspecs->accelspecs->operations =
	        ACCELFLAG_FILLBOX | ACCELFLAG_SETFGCOLOR |
		ACCELFLAG_SETBGCOLOR | ACCELFLAG_SCREENCOPY |
		ACCELFLAG_SETRASTEROP | ACCELFLAG_SETTRANSPARENCY |
		ACCELFLAG_SYNC;
    	    __svgalib_driverspecs->accelspecs->ropModes = (1<<ROP_COPY) |
		(1<<ROP_OR) | (1<<ROP_AND) | (1<<ROP_XOR) | (1<<ROP_INVERT);
	    __svgalib_driverspecs->accelspecs->transparencyModes =
		(1<<ENABLE_TRANSPARENCY_COLOR) | (1<<ENABLE_BITMAP_TRANSPARENCY);

	    if ((CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
		CHIPSchipset == CT_548 || CHIPSchipset == CT_4300)
		&& (modeinfo->bitsPerPixel == 24)) {
		__svgalib_driverspecs->accelspecs->ropOperations =
		    ACCELFLAG_SCREENCOPY;
	    } else {
		__svgalib_driverspecs->accelspecs->ropOperations =
	            ACCELFLAG_FILLBOX | ACCELFLAG_SCREENCOPY;
	    }
	    
	    if ((CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
		CHIPSchipset == CT_548 || CHIPSchipset == CT_4300)
		&& (modeinfo->bitsPerPixel == 24)) {
		/* For now PutBitmap is only supported on 6554x's. I
		 * don't have a 65550 machine to debug it on */
		__svgalib_driverspecs->accelspecs->operations |=
		    ACCELFLAG_PUTBITMAP;
		__svgalib_driverspecs->accelspecs->ropOperations |=
		    ACCELFLAG_PUTBITMAP;
		__svgalib_driverspecs->accelspecs->transparencyOperations |=
		    ACCELFLAG_PUTBITMAP;
	    }
	    
	    /* Set the function pointers; availability is handled by flags. */
	    __svgalib_driverspecs->accelspecs->SetFGColor = __svgalib_CHIPS_SetFGColor;
	    __svgalib_driverspecs->accelspecs->SetBGColor = __svgalib_CHIPS_SetBGColor;
	    __svgalib_driverspecs->accelspecs->SetRasterOp = __svgalib_CHIPS_SetRasterOp;
	    __svgalib_driverspecs->accelspecs->SetTransparency = __svgalib_CHIPS_SetTransparency;
	    if (ctMMIO) {
		if (ctisHiQV) {
		    __svgalib_driverspecs->accelspecs->FillBox = __svgalib_CHIPS_hiqv_FillBox;
		    __svgalib_driverspecs->accelspecs->ScreenCopy = CHIPS_hiqv_ScreenCopy;
		    __svgalib_driverspecs->accelspecs->Sync = CHIPS_hiqv_Sync;
		} else {
		    if (modeinfo->bitsPerPixel == 24) {
			__svgalib_driverspecs->accelspecs->FillBox = __svgalib_CHIPS_mmio_FillBox24;
		    } else {
			__svgalib_driverspecs->accelspecs->FillBox = __svgalib_CHIPS_mmio_FillBox;
			__svgalib_driverspecs->accelspecs->PutBitmap = __svgalib_CHIPS_mmio_PutBitmap;
		    }
		    __svgalib_driverspecs->accelspecs->ScreenCopy = CHIPS_mmio_ScreenCopy;
		    __svgalib_driverspecs->accelspecs->Sync = CHIPS_mmio_Sync;
		}
	    } else {
		if (modeinfo->bitsPerPixel == 24) {
		    __svgalib_driverspecs->accelspecs->FillBox = __svgalib_CHIPS_FillBox24;
		} else {	
		    __svgalib_driverspecs->accelspecs->FillBox = __svgalib_CHIPS_FillBox;
		    __svgalib_driverspecs->accelspecs->PutBitmap = __svgalib_CHIPS_PutBitmap;
		}
		__svgalib_driverspecs->accelspecs->ScreenCopy = CHIPS_ScreenCopy;
		__svgalib_driverspecs->accelspecs->Sync = CHIPS_Sync;
	    }
	}
	return 0;
}

/* Enable linear mode (0 to turn off) */

static void CHIPS_setlinear(int addr)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setlinear(0x%X)\n", addr);
#endif

    if (ctisHiQV) {
	port_out_r(0x3D6, 0x0A);
	if (addr)
	    port_out_r(0x3D7, port_in(0x3D7) | 0x02);	/* enable linear mode */
	else
	    port_out_r(0x3D7, port_in(0x3D7) & ~0x02);	/* disable linear mode */
    } else {
	port_out_r(0x3D6, 0x0B);
	if (addr)
	    port_out_r(0x3D7, port_in(0x3D7) | 0x10);	/* enable linear mode */
	else
	    port_out_r(0x3D7, port_in(0x3D7) & ~0x10);	/* disable linear mode */
    }
}

static int CHIPS_linear(int op, int param)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_linear(%d, %d)\n", op, param);
#endif

    if (CHIPSchipset != CT_520) {	/* The 65520 doesn't support it */
	if (op == LINEAR_ENABLE) {
	    CHIPS_setlinear(1);
	    return 0;
	}
	if (op == LINEAR_DISABLE) {
	    CHIPS_setlinear(0);
	    return 0;
	}
	if (op == LINEAR_QUERY_BASE) {

	    /* First deal with the user set MEMBASE */
	    if (ctFlagsSet & ctFlags_SetLinear) {
		switch (param) {
		  case 0:
		    return __svgalib_CHIPS_LinearBase;
		    break;
		  default:
		    return -1;
		    break;
		}
	    }
		
	    __svgalib_CHIPS_LinearBase = -1;
	    
	    if (PCIcard) {
                if (param == 0) {
                    __svgalib_CHIPS_LinearBase = chips_pcilinearbase;
#ifdef DEBUG
                    fprintf(stderr,"CHIPS_linear: base = %08X\n",__svgalib_CHIPS_LinearBase);
#endif
                } else {
                    /* As the above PCI membase probing code is new
                     * keep the old code around too as a fallback
                     */
                    switch (param) {
                        case 1:
                            __svgalib_CHIPS_LinearBase = 0xFE000000;
                            break;
                        case 2:
                            __svgalib_CHIPS_LinearBase = 0xC0000000;
                            break;
                        case 3:
                            __svgalib_CHIPS_LinearBase = 0xFD000000;
                            break;
                        case 4:
                            __svgalib_CHIPS_LinearBase = 0x41000000;
                            break;
                        default:
                            __svgalib_CHIPS_LinearBase = -1;
                            break;
                    }
		}
	    } else {
		if (param == 0) {
		    if (ctisHiQV) {
			port_out_r(0x3D6, 0x6);
			__svgalib_CHIPS_LinearBase = ((0xFF & port_in(0x3D7)) << 24);
			port_out_r(0x3D6, 0x5);
			__svgalib_CHIPS_LinearBase |= ((0x80 & port_in(0x3D7)) << 16);
		    } else {
			port_out_r(0x3D6, 0x8);
			__svgalib_CHIPS_LinearBase = ((0xFF & port_in(0x3D7)) << 20);
		    }
		}
	    }
	    return __svgalib_CHIPS_LinearBase;
	}
    }
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY)
	return 0;		/* No granularity or range. */
    else
	return -1;		/* Unknown function. */
}


/*----------------------------------------------------------------------*/
/* Indentify chipset; return non-zero if detected			*/
/*----------------------------------------------------------------------*/
static int CHIPS_test(void)
{
	unsigned char temp;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_test\n");
#endif

	/*
	 * OK. We have to actually test the hardware. The
	 * EnterLeave() function (described below) unlocks access
	 * to registers that may be locked, and for OSs that require
	 * it, enables I/O access. So we do this before we probe,
	 * even though we don't know for sure that this chipset
	 * is present.
	 */
	CHIPS_EnterLeave(ENTER);

	/*
	 * Here is where all of the probing code should be placed. 
	 * The best advice is to look at what the other drivers are 
	 * doing. If you are lucky, the chipset reference will tell 
	 * how to do this. Other resources include SuperProbe/vgadoc2,
	 * and the Ferraro book.
	 */
	port_out_r(0x3D6, 0x00);
	temp = port_in(0x3D6+1);

/*
 *	Reading 0x103 causes segmentation violation, like 46E8 ???
 *	So for now just force what I want!
 *
 *	Need to look at ioctl(console_fd, PCCONIOCMAPPORT, &ior)
 *	for bsdi!
 */
	CHIPSchipset = 99;

	if (temp != 0xA5)
	{
		if ((temp & 0xF0) == 0x70)
		{	CHIPSchipset = CT_520;
		}
		else
		if ((temp & 0xF0) == 0x80)	/* Could also be a 65525 */
		{	CHIPSchipset = CT_530;
		}
		else
		if ((temp & 0xF8) == 0xC0)
		{	CHIPSchipset = CT_535;
		}
		else
		if ((temp & 0xF8) == 0xD0)
		{	CHIPSchipset = CT_540;
		}
		else
		if ((temp & 0xF8) == 0xD8)
		{
		    switch (temp&0x7) {
		      case 3:
			CHIPSchipset = CT_546;
			break;
		      case 4:
			CHIPSchipset = CT_548;
			break;
		      default:
			CHIPSchipset = CT_545;
		    }
		}
	}

	/* At this point the chip could still be a ct65550, so check */
	if ((temp != 0) && (CHIPSchipset == 99)) {
	    port_out_r(0x3D6, 0x02);
	    temp = port_in(0x03D7);
	    if (temp == 0xE0) {
		CHIPSchipset = CT_550;
		ctisHiQV = TRUE;
	    }
	    if (temp == 0xE4) {
		CHIPSchipset = CT_554;
		ctisHiQV = TRUE;
	    }
	    if (temp == 0xE5) {
		CHIPSchipset = CT_555;
		ctisHiQV = TRUE;
	    }
	    if (temp == 0xF4) {
		CHIPSchipset = CT_8554;
		ctisHiQV = TRUE;
	    }
	    if ((temp == 0xC0) || (temp == 0x30)) { /* 0x30 is for 69030 */
		CHIPSchipset = CT_9000;
		ctisHiQV = TRUE;
	    }
	}

	if (CHIPSchipset == 99)
	{	/* failure, if no good, then leave */

		/*
		 * Turn things back off if the probe is going to fail.
		 * Returning FALSE implies failure, and the server
		 * will go on to the next driver.
		 */
		CHIPS_EnterLeave(LEAVE);

		return(FALSE);
	}


	CHIPS_init(0, 0, 0);

	return TRUE;
}


/*----------------------------------------------------------------------*/
/* CHIPS_EnterLeave --							*/
/*									*/
/* This function is called when the virtual terminal on which the	*/
/* server is running is entered or left, as well as when the server	*/
/* starts up and is shut down. Its function is to obtain and		*/
/* relinquish I/O  permissions for the SVGA device. This includes	*/
/* unlocking access to any registers that may be protected on the	*/
/* chipset, and locking those registers again on exit.			*/
/*									*/
/*----------------------------------------------------------------------*/
static void CHIPS_EnterLeave(Bool enter)
{
	unsigned char temp;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_EnterLeave(%d)\n", enter);
#endif

	/* (taken from XFree86) */

	if (enter)
	{
		/* 
		 * This is a global. The CRTC base address depends on
		 * whether the VGA is functioning in color or mono mode.
		 * This is just a convenient place to initialize this
		 * variable.
		 */
		vgaIOBase = (port_in(0x3CC) & 0x01) ? 0x3D0 : 0x3B0;

		/*
		 * Here we deal with register-level access locks. This
		 * is a generic VGA protection; most SVGA chipsets have
		 * similar register locks for their extended registers
		 * as well.
		 */
		/* Unprotect CRTC[0-7] */
		port_out_r(vgaIOBase + 4, 0x11); temp = port_in(vgaIOBase + 5);
		port_out_r(vgaIOBase + 5, temp & 0x7F);

		/* Enters Setup Mode */
/*		port_out_r(0x46E8, port_in(0x46E8) | 16); */

		/* Extension registers access enable */
/*		port_out_r(0x103, port_in(0x103) | 0x80); */
	}
	else
	{
		/*
		 * Here undo what was done above.
		 */
		/* Exits Setup Mode */
/*		port_out_r(0x46E8, port_in(0x46E8) & 0xEF); */

		/* Extension registers access disable */
/*		port_out_r(0x103, port_in(0x103) & 0x7F); */

		/* Protect CRTC[0-7] */
		port_out_r(vgaIOBase + 4, 0x11); temp = port_in(vgaIOBase + 5);
		port_out_r(vgaIOBase + 5, (temp & 0x7F) | 0x80);
	}
}


/*----------------------------------------------------------------------*/
/* Bank switching function - set 64K bank number			*/
/*----------------------------------------------------------------------*/
static void CHIPS_setpage(int page)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setpage(%d)\n",page);
#endif

	if (ctisHiQV) {
	    port_outw_r(0x3D6, ((page&0x7F) << 8) | 0x0E);
	} else {
#if !defined(seperated_read_write_bank)
	    port_outw_r(0x3D6, (page << 12) | 0x10);	/* bank 0  ( 64k window at 0xA0000 )*/
	    if (CHIPSchipset == CT_4300) {
	      unsigned char tmp;
	      port_out_r(0x3D6, 0x0C);
	      tmp = port_in(0x3D7) & 0xEF;
	      port_outw_r(0x3D6, ((((page & 0x10) | tmp) << 8) | 0x0C));
	    }
#else
	    int	temp;
	    temp = (page << 12);
	    port_outw_r(0x3D6, temp | 0x10);		/* bank 0 ( 32k window at 0xA0000 ) */
	    port_outw_r(0x3D6, temp + ((1 << 11) | 0x11));	/* bank 1 ( 32k window at 0xA8000 ) */
	    if (CHIPSchipset == CT_4300) {
	      unsigned char tmp;
	      port_out_r(0x3D6, 0x0C);
	      tmp = port_in(0x3D7) & 0xAF;
	      port_outw_r(0x3D6, ((((page & 0x10) | (( ((page << 2) + 2) & 0x40) |
					      tmp) << 8) | 0x0C));
	    }
#endif
	}
	
}

/*----------------------------------------------------------------------*/
/* W A R N I N G  :							*/
/* 	when using seperate banks, each bank can only access a maximum	*/
/*	of 32k. bank0 is accessed at 0xA0000 .. 0xA7FFF and bank1	*/
/*	is accessed at 0xA8000 .. 0xAFFFF				*/
/*	The GL library shipped with SVGALIB expects to be able to	*/
/*	access a 64k contiguoius window at 0xA0000 .. 0xAFFFF		*/
/*----------------------------------------------------------------------*/


/*----------------------------------------------------------------------*/
/* Bank switching function - set 32K bank number			*/
/* WARNING: this function uses a granularity or 32k not 64k		*/
/*----------------------------------------------------------------------*/
static void CHIPS_setreadpage(int page)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setreadpage(%d)\n",page);
#endif
    if (CHIPSchipset == CT_4300) {
      unsigned char tmp;
      port_outw_r(0x3D6, (page << 11) | 0x10);	/* bank 0 */
      port_out_r(0x3D6, 0x0C);
      tmp = port_in(0x3D7) & 0xEF;
      port_outw_r(0x3D6, (((((page >> 1) & 0x10) | tmp) << 8) | 0x0C));
    } else {
      port_outw_r(0x3D6, (page << 11) | 0x10);	/* bank 0 */
    }
}

/*----------------------------------------------------------------------*/
/* Bank switching function - set 32K bank number			*/
/* WARNING: this function uses a granularity or 32k not 64k		*/
/*----------------------------------------------------------------------*/
static void CHIPS_setwritepage(int page)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setwritepage(%d)\n",page);
#endif
    if (CHIPSchipset == CT_4300) {
      unsigned char tmp;
      port_outw_r(0x3D6, (page << 11) | 0x11);	/* bank 1 */
      port_out_r(0x3D6, 0x0C);
      tmp = port_in(0x3D7) & 0xBF;
      port_outw_r(0x3D6, (((((page << 1) & 0x40) | tmp) << 8) | 0x0C));
    } else {
      port_outw_r(0x3D6, (page << 11) | 0x11);	/* bank 1 */
    }
}


/*----------------------------------------------------------------------*/
/* Set display start address (not for 16 color modes)			*/
/*----------------------------------------------------------------------*/
static void CHIPS_setdisplaystart(int addr)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setdisplaystart(%d)\n",addr);
#endif

	addr >>= 2;

	/*
	 * These are the generic starting address registers.
	 * (taken from XFree86)
	 */
	port_outw_r(vgaIOBase + 4, (addr & 0x00FF00) | 0x0C);
	port_outw_r(vgaIOBase + 4, ((addr & 0x00FF) << 8) | 0x0D);
	
	/*
	 * Here the high-order bits are masked and shifted, and put into
	 * the appropriate extended registers.
	 */
	/* MH - plug in the high order starting address bits */
	if (ctisHiQV) {
	    port_out_r(0x3D6, 0x09);
	    if ((port_in(0x3D7) & 0x1) == 0x1)
	        port_outw_r(vgaIOBase + 4, ((addr & 0x0F0000) >> 8) | 0x8000 | 0x40);
	} else {
	    port_out_r(0x3D6, 0x0C);
	    port_out_r(0x3D7, ((addr & 0xFF0000) >> 16));
	}
}

/*----------------------------------------------------------------------*/
/* Set logical scanline length (usually multiple of 8)			*/
/*----------------------------------------------------------------------*/
static void CHIPS_setlogicalwidth(int width)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_setlogicalwidth(%d)\n",width);
#endif

	port_outw_r(vgaIOBase + 4, 0x13 | (((width >> 3) << 8) & 0xFF00));
	if (ctisHiQV) {
	    port_outw_r(vgaIOBase + 4, 0x41 | ((width >>3) & 0xF00));
	} else {
	    port_out_r(0x3D6,0x1E);
	    port_out_r(0x3D7,((width>>3)&0xFF));
	    port_out_r(0x3D6,0x0D);
	    port_out_r(0x3D7,(((width>>11)&0x1)|((width>>10)&0x2)));
	}  
}


/*----------------------------------------------------------------------*/
/* Function table							*/
/*----------------------------------------------------------------------*/
DriverSpecs __svgalib_chips_driverspecs =
{
	CHIPS_saveregs,
	CHIPS_setregs,
	nothing,			/* unlock */
	nothing,			/* lock */
	CHIPS_test,
	CHIPS_init,
	CHIPS_setpage,
	CHIPS_setreadpage,
	CHIPS_setwritepage,
	CHIPS_setmode,
	CHIPS_modeavailable,
	CHIPS_setdisplaystart,
	CHIPS_setlogicalwidth,
	CHIPS_getmodeinfo,
	0,				/* bitblt */
	0,				/* imageblt */
	0,				/* fillblt */
	0,				/* hlinelistblt */
	0,				/* bltwait */
	0,				/* extset */
	0,
	CHIPS_linear,			/* linear */
	NULL,				/* Accelspecs */
	NULL,				/* Emulation */
};

/* Chips and Technologies specific config file options.
 * Currently this only handles setting the text clock frequency.
 */

static char *CHIPS_config_options[] =
{
    "TextClockFreq", "LCDPanelSize", "UseModeline", "NoBitBlt", 
    "Use18BitBus", "setuplinear", "Stretch", "Center", "DacSpeed", NULL
};

static char *CHIPS_process_option(int option, int mode, char** dummy)
{
#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_process_option(%d, %d)\n",option, mode);
#endif
/*
 * option is the number of the option string in CHIPS_config_options,
 * mode seems to be a security indicator.
 */
    if (option == 0) {		/* TextClockFreq */
	char *ptr;
	ptr = strtok(NULL, " ");
	/*
	 * This doesn't protect against bad characters
	 * (atof() doesn't detect errors).
	 */
	ctTextClock = (int)(atof(ptr) * 1000.);
    }
    if (option == 1) {		/* LCDPanelSize */
	char *ptr;
	ptr = strtok(NULL, " ");
	/*
	 * This doesn't protect against bad characters
	 * (atoi() doesn't detect errors).
	 */
	__svgalib_ctSize.HDisplay = atoi(ptr);
	ptr = strtok(NULL, " ");
	__svgalib_ctSize.VDisplay = atoi(ptr);
	ctFlagsSet |= ctFlags_LCDPanelSize;
    }
    if (option == 2) {		/* UseModeline */
	ctFlagsSet |= ctFlags_UseModeline;
    }
    if (option == 3) {		/* NoBitBlt */
	ctFlagsSet |= ctFlags_NoBitBlt;
    }
    if (option == 4) {		/* Use18bitBus */
	ctFlagsSet |= ctFlags_Use18BitBus;
    }
    if (option == 5) {		/* setuplinear */
	char c;
	unsigned int value;
	char *ptr;

	ptr = strtok(NULL, " ");
	c = *ptr++;
	c = *ptr++;
	value = 0;
	while ((c = *ptr++)) { /* Double brackets to stop gcc warning */
	    if(c >= '0' && c <= '9')
	        value = (value << 4) | (c - '0');  /*ASCII assumed*/
	    else if(c >= 'A' && c < 'G')
	        value = (value << 4) | (c - 'A'+10);  /*ASCII assumed*/
	    else if(c >= 'a' && c < 'g')
	        value = (value << 4) | (c - 'a'+10);  /*ASCII assumed*/
	}
	__svgalib_CHIPS_LinearBase = value;
	ptr = strtok(NULL, " ");
	ctFlagsSet |= ctFlags_SetLinear;
    }
    if (option == 6) {		/* Stretch */
	/* Enable or disable stretching in horizontal and vertical
	 * directions. Note that the default is to leave this stuff
	 * alone */
	char *ptr;
	ptr = strtok(NULL, " ");
	if (!(strcasecmp(ptr, "ENABLE"))) {
	    ctFlagsSet |= ctFlags_StretchEnable;
	    ctFlagsSet &= ~ctFlags_StretchDisable;
	} else if (!(strcasecmp(ptr, "DISABLE"))) {
	    ctFlagsSet |= ctFlags_StretchDisable;
	    ctFlagsSet &= ~ctFlags_StretchEnable;
	}
    }
    if (option == 7) {		/* Center */
	/* Enable or disable centring of the mode in horizontal and
	 * vertical directions. Note that the default is to leave
	 * this stuff alone */
	char *ptr;
	ptr = strtok(NULL, " ");
	if (!(strcasecmp(ptr, "ENABLE"))) {
	    ctFlagsSet |= ctFlags_CenterEnable;
	    ctFlagsSet &= ~ctFlags_CenterDisable;
	} else if (!(strcasecmp(ptr, "DISABLE"))) {
	    ctFlagsSet |= ctFlags_CenterDisable;
	    ctFlagsSet &= ~ctFlags_CenterEnable;
	}
    }
    if (option == 8) {		/* DacSpeed */
	char *ptr;
	ptr = strtok(NULL, " ");
	/*
	 * This doesn't protect against bad characters
	 * (atof() doesn't detect errors).
	 */
	ctDacSpeed = (int)(atof(ptr) * 1000.);
    }
    return strtok(NULL, " ");
}    

/*----------------------------------------------------------------------*/
/* Initialize chipset (called after detection)				*/
/*----------------------------------------------------------------------*/
static int CHIPS_init(int force, int par1, int par2)
{
    int	temp;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: CHIPS_init(%d, %d, %d)\n", force, par1, par2);
#endif

	if (force) {
	    CHIPS_EnterLeave(ENTER);
	    CHIPSchipset = par1;
	    video_memory = par2;
	    if (CHIPSchipset == CT_550 || CHIPSchipset == CT_554 ||
		CHIPSchipset == CT_555 || CHIPSchipset == CT_8554 ||
		CHIPSchipset == CT_9000)
		ctisHiQV = TRUE;
	} else {
	    /*
	     * do whatever chipset-specific things are necessary
	     * to figure out how much memory (in kBytes) is 
	     * available.
	     */
	    if (ctisHiQV) {
		switch (CHIPSchipset) {
		case CT_9000:
		    /* The ct69000 has 2Mb of SGRAM integrated */
                    /* 69030 has 4MB, needs to be fixed */
		    video_memory = 2048;
		    break;
		case CT_550:
		    port_out_r(0x3D6, 0x43);
		    switch ((port_in(0x3D7) & 0x06) >> 1) {
		    case 0:
			video_memory = 1024;
			break;
		    case 1:
			video_memory = 2048;
			break;
		    case 2:
		    case 3:
		      video_memory = 4096;
		      break;
		    }
		default:
		    port_out_r(0x3D6, 0xE0);
		    switch (port_in(0x3D7) & 0xF) {
		    case 0:
			video_memory = 512;
			break;
		    case 1:
			video_memory = 1024;
			break;
		    case 2:
			video_memory = 1536;
			break;
		    case 3:
			video_memory = 2048;
			break;
		    case 7:
			video_memory = 4096;
			break;
		    default:
			video_memory = 1024;
			break;
		    }
		}
	    } else {
		port_out_r(0x3D6, 0x0F);
		temp = port_in(0x3D7);
		switch (temp & 3) {
		  case 0:
		    video_memory = 256;
		    break;
		  case 1:
		    video_memory = 512;
		    break;
		  case 2:
		    video_memory = 1024;
		    break;
		  case 3:
		    if (CHIPSchipset == CT_4300)
		      video_memory = 2048;
		    else
		      video_memory = 1024;
		    break;
		}
	    }
	}

        /* check whether this is a PCI card */
        if (ctisHiQV) {
            port_out_r(0x3D6,0x08);
            PCIcard = (((port_in(0x3D7) & 1) == 1) ? TRUE : FALSE);
        } else {
            port_out_r(0x3D6,0x01);
            PCIcard = (((port_in(0x3D7) & 7) == 6) ? TRUE : FALSE);
        }

        /* get linear base address from PCI config space */
        if (PCIcard) {
            unsigned int buf[64];

            /* Find PCI config with C&T vendor ID 0x102C */
            if (__svgalib_pci_find_vendor_vga_pos(0x102C,buf)) {
                chips_pcilinearbase = buf[4] & 0xFF800000;
            }
        }

	/* Process the chips and technologies specific config file options */
	__svgalib_read_options(CHIPS_config_options, CHIPS_process_option);

	if (ctisHiQV) {
	    /*test STN / TFT */
	    port_out_r(0x3D0, 0x10);
	    temp = port_in(0x3D1);
	    
	    /* FR10[1:0]: PanelType,                                    */
	    /* 0 = Single Panel Single Drive, 3 = Dual Panel Dual Drive */
	    switch (temp & 0x3) {
	      case 0:
		__svgalib_ctPanelType = TFT;
		break;
	      case 2:
		__svgalib_ctPanelType = DS;
		break;
	      case 3:
		__svgalib_ctPanelType = DD;
		break;
	      default:
		break;
	    }

	    /* test LCD */
	    /* FR01[1:0]:   Display Type, 01 = CRT, 10 = FlatPanel */
	    /* LCD                                                 */
	    port_out_r(0x3D0, 0x01);
	    temp = port_in(0x3D1);
	    if ((temp & 0x03) == 0x02) {
		ctLCD = TRUE;
		ctCRT = FALSE;
	    } else {
		ctLCD = FALSE;
		ctCRT = TRUE;
	    }
            if (ctLCD) {
                /* get H/VDisplay values for screen size */

                unsigned char fr25,tmp1;

		if (!(ctFlagsSet & ctFlags_LCDPanelSize)) { /* read only if necessary */
                    port_out_r(0x3D0,0x25);
                    fr25=port_in(0x3D1);
                    port_out_r(0x3D0,0x20);
                    temp=port_in(0x3D1);
		    __svgalib_ctSize.HDisplay = ((temp + ((fr25 & 0x0F) << 8)) + 1) << 3;
                    port_out_r(0x3D0,0x30);
                    temp=port_in(0x3D1);
                    port_out_r(0x3D0,0x35);
                    tmp1=port_in(0x3D1);
                    __svgalib_ctSize.VDisplay = ((tmp1 & 0x0F) << 8) + temp + 1;
		}
            }
	} else if (CHIPSchipset != CT_4300) {

	    /*test STN / TFT */
	    port_out_r(0x3D6, 0x51);
	    temp = port_in(0x3D7);
	    /* XR51[1-0]: PanelType,                                    */
	    /* 0 = Single Panel Single Drive, 3 = Dual Panel Dual Drive */
	    switch (temp & 0x3) {
	      case 0:
		__svgalib_ctPanelType = TFT;
		break;
	      case 2:
		__svgalib_ctPanelType = DS;
		break;
	      case 3:
		__svgalib_ctPanelType = DD;
		break;
	      default:
		break;
	    }
		
	    /* test LCD */
	    if (temp & 0x4) {
		/* XR51[2]:   Display Type, 0 = CRT, 1 = FlatPanel */
		/* LCD                                             */
		ctLCD = TRUE;
		ctCRT = FALSE;
	    } else {
		ctLCD = FALSE;
		ctCRT = TRUE;
	    }

	    /* screen size */
	    /* 
	     * In LCD mode / dual mode we want to derive the timing
	     *  values from the ones preset by bios
	     */
	    if (ctLCD) {
		unsigned char xr17, tmp1;
		char tmp2;

		port_out_r(0x3D6,0x17);
		xr17=port_in(0x3D7);
		port_out_r(0x3D6,0x1B);
		temp=port_in(0x3D7);
		__svgalib_ctSize.HTotal = ((temp + ((xr17 & 0x01) << 8)) + 5) << 3;
		if (!(ctFlagsSet & ctFlags_LCDPanelSize)) {
		    port_out_r(0x3D6,0x1C);
		    temp=port_in(0x3D7);
		    __svgalib_ctSize.HDisplay = ((temp + ((xr17 & 0x02) << 7)) + 1) << 3;
		}
		port_out_r(0x3D6,0x19);
		temp=port_in(0x3D7);
		__svgalib_ctSize.HRetraceStart = ((temp + ((xr17 & 0x04) << 9))
					+ 1) << 3;
		port_out_r(0x3D6,0x1A);
		tmp1=port_in(0x3D7);
		tmp2 = (tmp1 & 0x1F) + ((xr17 & 0x08) << 2) - (temp & 0x3F);
		__svgalib_ctSize.HRetraceEnd = ((((tmp2 < 0) ? (tmp2 + 0x40) : tmp2)
				       << 3) + __svgalib_ctSize.HRetraceStart);
		port_out_r(0x3D6,0x65);
		tmp1=port_in(0x3D7);
		if (!(ctFlagsSet & ctFlags_LCDPanelSize)) {
		    port_out_r(0x3D6,0x68);
		    temp=port_in(0x3D7);
		    __svgalib_ctSize.VDisplay = ((tmp1 & 0x02) << 7) 
		    + ((tmp1 & 0x40) << 3) + temp + 1;
		}
		port_out_r(0x3D6,0x66);
		temp=port_in(0x3D7);
		__svgalib_ctSize.VRetraceStart = ((tmp1 & 0x04) << 6) 
		    + ((tmp1 & 0x80) << 2) + temp + 1;
		port_out_r(0x3D6,0x64);
		temp=port_in(0x3D7);
		__svgalib_ctSize.VTotal = ((tmp1 & 0x01) << 8)
		    + ((tmp1 & 0x20) << 4) + temp + 2;
#ifdef DEBUG
		fprintf(stderr,"__svgalib_ctSize.VDisplay = %d, __svgalib_ctSize.HDisplay = %d\n",
		__svgalib_ctSize.VDisplay,__svgalib_ctSize.HDisplay);
#endif
	    }
	} else {
	  ctCRT = TRUE;
	  ctLCD = FALSE;
	}
	

	ctVgaIOBaseFlag = (port_in(0x3CC) & 0x01);

	/* Initialize accelspecs structure. */
	if (CHIPSchipset == CT_545 || CHIPSchipset == CT_546 ||
	    CHIPSchipset == CT_548 || CHIPSchipset == CT_550 ||
	    CHIPSchipset == CT_554 || CHIPSchipset == CT_555 ||
	    CHIPSchipset == CT_8554 || CHIPSchipset == CT_9000 ||
	    CHIPSchipset == CT_4300) {
	    __svgalib_chips_driverspecs.accelspecs = malloc(sizeof(AccelSpecs));
	    __svgalib_clear_accelspecs(__svgalib_chips_driverspecs.accelspecs);
	    __svgalib_chips_driverspecs.accelspecs->flags = ACCELERATE_ANY_LINEWIDTH;
	}
	
	if (__svgalib_driver_report)
	{
		fprintf(stderr,"Using C&T 655xx driver (%dK) [%d].\n",
			   video_memory, CHIPSchipset);
	}

	__svgalib_driverspecs = &__svgalib_chips_driverspecs;

	cardspecs = malloc(sizeof(CardSpecs));
	cardspecs->videoMemory = video_memory;
	
	if (ctDacSpeed) {
	  switch(CHIPSchipset) { 
	  case CT_520:
	  case CT_525:
	  case CT_530:
	  case CT_535:
	    cardspecs->maxPixelClock8bpp = ctDacSpeed;
	    cardspecs->maxPixelClock16bpp = 0;
	    cardspecs->maxPixelClock24bpp = 0;
	    break;
	  default:    
	    cardspecs->maxPixelClock8bpp = ctDacSpeed;
	    cardspecs->maxPixelClock16bpp = ctDacSpeed;
	    cardspecs->maxPixelClock24bpp = ctDacSpeed;
	    break;
	  }
	} else {
	  switch(CHIPSchipset) { 
	  case CT_520:
	  case CT_525:
	  case CT_530:
	  case CT_535:
	    port_out_r(0x3D6,0x6C);
	    if (port_in(0x3D7) & 2) {
		cardspecs->maxPixelClock8bpp = 68000;	/* 5V Vcc */
	    } else {
		cardspecs->maxPixelClock8bpp = 56000;	/* 3.3V Vcc */
	    }
	    cardspecs->maxPixelClock16bpp = 0;
	    cardspecs->maxPixelClock24bpp = 0;
	    break;
	  case CT_540:
	  case CT_545:
	    port_out_r(0x3D6,0x6C);
	    if (port_in(0x3D7) & 2) {
		cardspecs->maxPixelClock8bpp = 68000;	/* 5V Vcc */
		cardspecs->maxPixelClock16bpp = 34000;
		cardspecs->maxPixelClock24bpp = 22667;
	    } else {
		cardspecs->maxPixelClock8bpp = 56000;	/* 3.3V Vcc */
		cardspecs->maxPixelClock16bpp = 28322;
		cardspecs->maxPixelClock24bpp = 18667;
	    }
	    break;
	  case CT_546:
	  case CT_548:
	    cardspecs->maxPixelClock8bpp = 80000;
	    cardspecs->maxPixelClock16bpp = 40000;
	    cardspecs->maxPixelClock24bpp = 26667;
	    break;
	  case CT_4300:
	    cardspecs->maxPixelClock8bpp = 85000;
	    cardspecs->maxPixelClock16bpp = 42500;
	    cardspecs->maxPixelClock24bpp = 28333;
	    break;
	  case CT_9000:
	    cardspecs->maxPixelClock8bpp = 220000;
	    cardspecs->maxPixelClock16bpp = 220000;
	    cardspecs->maxPixelClock24bpp = 220000;
	    break;
	  case CT_8554:
	  case CT_555:
	    cardspecs->maxPixelClock8bpp = 110000;
	    cardspecs->maxPixelClock16bpp = 110000;
	    cardspecs->maxPixelClock24bpp = 110000;
	    break;
	  case CT_554:
	    cardspecs->maxPixelClock8bpp = 95000;
	    cardspecs->maxPixelClock16bpp = 95000;
	    cardspecs->maxPixelClock24bpp = 95000;
	    break;
	  case CT_550:
	      port_out_r(0x3D6, 0x04);
	      if ((port_in(0x3D7) & 0xF) < 6) {
		  port_out_r(0x3D0,0x0A);
		  if (port_in(0x3D1) & 2) {
		    cardspecs->maxPixelClock8bpp = 110000;	/* 5V Vcc */
		    cardspecs->maxPixelClock16bpp = 110000;
		    cardspecs->maxPixelClock24bpp = 110000;
		  } else {
		    cardspecs->maxPixelClock8bpp = 80000;	/* 3.3V Vcc */
		    cardspecs->maxPixelClock16bpp = 80000;
		    cardspecs->maxPixelClock24bpp = 80000;
		  }
	      } else {
		  cardspecs->maxPixelClock8bpp = 95000;	/* Revision B */
		  cardspecs->maxPixelClock16bpp = 95000;
		  cardspecs->maxPixelClock24bpp = 95000;
	      }
	    break;
	  }

	  /* Adjust HiQV maximum clocks to meet limits in spec */
	  if (ctisHiQV) {
	    unsigned char tmp, M, N, P, PSN;
	    unsigned int MemClk;

	    /* Probe the memory clock currently in use */
	    port_out_r(0x3D6,0xCC);
	    M = (port_in(0x3D7)  & 0x7F) + 2;
	    port_out_r(0x3D6,0xCD);
	    N = (port_in(0x3D7) & 0x7F) + 2;
	    port_out_r(0x3D6,0xCE);
	    tmp = port_in(0x3D7);
	    PSN = (tmp & 0x1) ? 1 : 4;
	    P = ((tmp & 0x70) >> 4);
	    /* Be careful with the calculation of MemClk as it can overflow */ 
	    MemClk = 4 * Fref / N / 1000;
	    MemClk = MemClk * M / (PSN * (1 << P));

	    port_out_r(0x3D0, 0x1A);
	    tmp = port_in(0x3D1);
	    if ((tmp & 1) && (!(tmp & 0x80))) {
	      /* Extra byte per clock cycle for embedded DSTN framebuffer */
	      cardspecs->maxPixelClock8bpp = min(cardspecs->maxPixelClock8bpp,
			 MemClk * 4 * 0.7 / 2);
	      cardspecs->maxPixelClock16bpp = min(
			 cardspecs->maxPixelClock16bpp, MemClk * 4 * 0.7 / 3);
	      cardspecs->maxPixelClock24bpp = min(
			 cardspecs->maxPixelClock24bpp, MemClk * 4 * 0.7 / 4);
	    } else {
	      cardspecs->maxPixelClock8bpp = min(cardspecs->maxPixelClock8bpp,
			 MemClk * 4 * 0.7 / 1);
	      cardspecs->maxPixelClock16bpp = min(
			 cardspecs->maxPixelClock16bpp, MemClk * 4 * 0.7 / 2);
	      cardspecs->maxPixelClock24bpp = min(
			 cardspecs->maxPixelClock24bpp, MemClk * 4 * 0.7 / 3);
	    }
	  }
	}
	cardspecs->maxPixelClock4bpp = 0; /* Handled by Generic VGA */
	cardspecs->maxPixelClock32bpp = 0; /* Not available at all */

	if ((CHIPSchipset == CT_520) || (CHIPSchipset == CT_525) || 
	    (CHIPSchipset == CT_530)) {
	    cardspecs->flags = 0;
	    cardspecs->nClocks = CHIPS_NUM_CLOCKS;
	    cardspecs->clocks = chips_fixed_clocks;
	} else {
	    cardspecs->flags = CLOCK_PROGRAMMABLE;
	    cardspecs->nClocks = 0;
	    cardspecs->clocks = NULL;
	    cardspecs->matchProgrammableClock = CHIPS_matchProgrammableClock;
	}
	if (ctisHiQV) {
	    cardspecs->maxHorizontalCrtc = 4096;
	} else {
	    cardspecs->maxHorizontalCrtc = 2048;
	}
	cardspecs->mapClock = CHIPS_mapClock;
	cardspecs->mapHorizontalCrtc = CHIPS_mapHorizontalCrtc;


    __svgalib_CHIPS_LinearBase=chips_pcilinearbase;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=__svgalib_CHIPS_LinearBase;
    __svgalib_linear_mem_size=video_memory*0x400;

    MMIO_mem1=
       		mmap(__svgalib_ctMMIOBase, 0x10000, PROT_WRITE, MAP_FIXED | 
	             MAP_SHARED, __svgalib_mem_fd, 
                     __svgalib_CHIPS_LinearBase + 0x200000L);
    MMIO_mem2=
   		mmap(__svgalib_ctMMIOBase, 0x20000, PROT_WRITE, MAP_FIXED | 
		     MAP_SHARED, __svgalib_mem_fd, 
                     __svgalib_CHIPS_LinearBase + 0x400000L);
    
	return 0;
}

/* These are the macro's for setting the ROP's with the 6554x's */
#define ctTOP2BOTTOM            0x100
#define ctBOTTOM2TOP            0x000
#define ctLEFT2RIGHT            0x200
#define ctRIGHT2LEFT            0x000
#define ctSRCMONO               0x800
#define ctPATMONO               0x1000
#define ctBGTRANSPARENT         0x2000
#define ctSRCSYSTEM             0x4000
#define ctPATSOLID              0x80000L

/* These are the macro's for setting the ROP's with the 6555x's */
#define ctHIQVTOP2BOTTOM        0x000
#define ctHIQVBOTTOM2TOP        0x200
#define ctHIQVLEFT2RIGHT        0x000
#define ctHIQVRIGHT2LEFT        0x100
#define ctHIQVSRCSYSTEM         0x400
#define ctHIQVSRCMONO           0x1000
#define ctHIQVPATMONO           0x40000L
#define ctHIQVBGTRANSPARENT     0x22000L
#define ctHIQVPATSOLID          0x80000L

/* These are the macro's for setting the monochrome source
 * expansion with the 6555x's */
#define ctHIQVCLIPLEFT(clip)    (clip&0x3F)
#define ctHIQVCLIPRIGHT(clip)   ((clip&0x3F) << 8)
#define ctHIQVSRCDISCARD(clip)  ((clip&0x3F) << 16)
#define ctHIQVBITALIGN          0x1000000L
#define ctHIQVBYTEALIGN         0x2000000L
#define ctHIQVWORDALIGN         0x3000000L
#define ctHIQVDWORDALIGN        0x4000000L
#define ctHIQVQWORDALIGN        0x5000000L

/* These are the macro functions for programming the Register
 * addressed blitter for the 6554x's */
#define ctBLTWAIT while(port_inw(0x93D2)&0x10){}
#define ctSETROP(op) port_outl_r(0x93D0,op)
#define ctSETSRCADDR(srcAddr) port_outl_r(0x97D0,(srcAddr&0x1FFFFFL))
#define ctSETDSTADDR(dstAddr) port_outl_r(0x9BD0,(dstAddr&0x1FFFFFL))
#define ctSETPITCH(srcPitch,dstPitch) port_outl_r(0x83D0,((dstPitch<<16)|srcPitch))
#define ctSETHEIGHTWIDTHGO(Height,Width) port_outl_r(0x9FD0,((Height<<16)|Width))
#define ctSETBGCOLOR(bgColor) port_outl_r(0x8BD0,(bgColor))
#define ctSETFGCOLOR(fgColor) port_outl_r(0x8FD0,(fgColor))

/* These are the macro functions for programming the MMIO
 * addressed blitter for the 6554x's */
#define ctMMIOBLTWAIT while(*(volatile unsigned int *)(__svgalib_ctMMIOBase + 0x93D0) & \
    0x00100000){}
#define ctMMIOSETROP(op) *(unsigned int *)(__svgalib_ctMMIOBase + 0x93D0) = op
#define ctMMIOSETSRCADDR(srcAddr) *(unsigned int *)(__svgalib_ctMMIOBase + 0x97D0) = \
    srcAddr&0x7FFFFFL
#define ctMMIOSETDSTADDR(dstAddr) *(unsigned int *)(__svgalib_ctMMIOBase + 0x9BD0) = \
    dstAddr&0x7FFFFFL
#define ctMMIOSETPITCH(srcPitch,dstPitch) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x83D0) = ((dstPitch&0xFFFF)<<16)| \
    (srcPitch&0xFFFF)
#define ctMMIOSETHEIGHTWIDTHGO(Height,Width) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x9FD0) = ((Height&0xFFFF)<<16)| \
    (Width&0xFFFF)
#define ctMMIOSETBGCOLOR(bgColor) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x8BD0) = (bgColor)
#define ctMMIOSETFGCOLOR(fgColor) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x8FD0) = (fgColor)

/* These are the macro functions for programming the MMIO
 * addressed blitter for the 6555x's */
#if 0
/* Chips and technologies released an application note saying that
 * with certain batches of chips you couldn't read the blitter registers
 * properly. This could cause some drawing anolomies, use XR20[0] instead
 */
#define ctHIQVBLTWAIT while(*(volatile unsigned int *)(__svgalib_ctMMIOBase + 0x10) & \
    0x80000000){}
#else
#define ctHIQVBLTWAIT port_out_r(0x3D6,0x20); while(port_in(0x3D7)&0x1){}
#endif
#define ctHIQVSETROP(op) *(unsigned int *)(__svgalib_ctMMIOBase + 0x10) = op
#define ctHIQVSETSRCADDR(srcAddr) *(unsigned int *)(__svgalib_ctMMIOBase + 0x18) = \
    srcAddr&0x7FFFFFL
#define ctHIQVSETDSTADDR(dstAddr) *(unsigned int *)(__svgalib_ctMMIOBase + 0x1C) = \
    dstAddr&0x7FFFFFL
#define ctHIQVSETPITCH(srcPitch,dstPitch) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x0) = ((dstPitch&0xFFFF)<<16)| \
    (srcPitch&0xFFFF)
#define ctHIQVSETHEIGHTWIDTHGO(Height,Width) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x20) = ((Height&0xFFFF)<<16)| \
    (Width&0xFFFF)
#define ctHIQVSETMONOCTL(op) \
  *(unsigned int *)(__svgalib_ctMMIOBase + 0xC) = op
#define ctHIQVSETBGCOLOR(bgColor) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x4) = (bgColor)
#define ctHIQVSETFGCOLOR(fgColor) \
    *(unsigned int *)(__svgalib_ctMMIOBase + 0x8) = (fgColor)


static void CHIPS_ScreenCopy(int x1, int y1, int x2, int y2, int w, int h)
{
    int srcaddr, destaddr, op;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: ScreenCopy(%d, %d, %d, %d, %d, %d)\n", x1, y1, x1, y2, w, h);
#endif

    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    op = ctAluConv[ctROP&0xF];
    if (x1 < x2) {
	op |= ctRIGHT2LEFT;
    } else {
	op |= ctLEFT2RIGHT;
    }    
    if (y1 < y2) {
	op |= ctBOTTOM2TOP;
    } else {
	op |= ctTOP2BOTTOM;
    }    
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
	ctBLTWAIT;
    ctSETROP(op);
    ctSETSRCADDR(srcaddr);
    ctSETDSTADDR(destaddr);
    ctSETPITCH(__svgalib_accel_screenpitchinbytes, __svgalib_accel_screenpitchinbytes);
    ctSETHEIGHTWIDTHGO(h, w * __svgalib_accel_bytesperpixel);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	ctBLTWAIT;
}

static void CHIPS_mmio_ScreenCopy(int x1, int y1, int x2, int y2, int w, int h)
{
    int srcaddr, destaddr, op;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: mmio_ScreenCopy(%d, %d, %d, %d, %d, %d)\n", x1, y1, x1, y2, w, h);
#endif

    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    op = ctAluConv[ctROP&0xF];
    if (x1 < x2) {
	op |= ctRIGHT2LEFT;
    } else {
	op |= ctLEFT2RIGHT;
    }    
    if (y1 < y2) {
	op |= ctBOTTOM2TOP;
    } else {
	op |= ctTOP2BOTTOM;
    }
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctMMIOBLTWAIT;
    ctMMIOSETROP(op);
    ctMMIOSETSRCADDR(srcaddr);
    ctMMIOSETDSTADDR(destaddr);
    ctMMIOSETPITCH(__svgalib_accel_screenpitchinbytes, __svgalib_accel_screenpitchinbytes);
    ctMMIOSETHEIGHTWIDTHGO(h, w*__svgalib_accel_bytesperpixel);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctMMIOBLTWAIT;
}

static void CHIPS_hiqv_ScreenCopy(int x1, int y1, int x2, int y2, int w, int h)
{
    int srcaddr, destaddr, op;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: hiqv_ScreenCopy(%d, %d, %d, %d, %d, %d)\n", x1, y1, x1, y2, w, h);
#endif

    srcaddr = BLTBYTEADDRESS(x1, y1);
    destaddr = BLTBYTEADDRESS(x2, y2);
    op = ctAluConv[ctROP&0xF];
    if (x1 < x2) {
	op |= ctHIQVRIGHT2LEFT;
    } else {
	op |= ctHIQVLEFT2RIGHT;
    }    
    if (y1 < y2) {
	op |= ctHIQVBOTTOM2TOP;
    } else {
	op |= ctHIQVTOP2BOTTOM;
    }    
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctHIQVBLTWAIT;
    ctHIQVSETROP(op);
    ctHIQVSETSRCADDR(srcaddr);
    ctHIQVSETDSTADDR(destaddr);
    ctHIQVSETPITCH(__svgalib_accel_screenpitchinbytes, __svgalib_accel_screenpitchinbytes);
    ctHIQVSETHEIGHTWIDTHGO(h, w*__svgalib_accel_bytesperpixel);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctHIQVBLTWAIT;
}


void __svgalib_CHIPS_FillBox(int x, int y, int width, int height)
{
    int destaddr;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: FillBox(%d, %d, %d, %d)\n", x, y, width, height);
#endif

    destaddr = BLTBYTEADDRESS(x, y);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctBLTWAIT;
    ctSETDSTADDR(destaddr);
    ctSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctSETROP(ctAluConv2[ctROP&0xF] | ctTOP2BOTTOM | ctLEFT2RIGHT | ctPATSOLID
	     | ctPATMONO);
    ctSETFGCOLOR(ctFGCOLOR);
    ctSETBGCOLOR(ctFGCOLOR);
    ctSETHEIGHTWIDTHGO(height,width*__svgalib_accel_bytesperpixel);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctBLTWAIT;
}

void __svgalib_CHIPS_mmio_FillBox(int x, int y, int width, int height)
{
    int destaddr;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: mmio_FillBox(%d, %d, %d, %d)\n", x, y, width, height);
#endif

    destaddr = BLTBYTEADDRESS(x, y);
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctMMIOBLTWAIT;
    ctMMIOSETDSTADDR(destaddr);
    ctMMIOSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctMMIOSETROP(ctAluConv2[ctROP&0xF] | ctTOP2BOTTOM | ctLEFT2RIGHT | 
	     ctPATSOLID | ctPATMONO);
    ctMMIOSETFGCOLOR(ctFGCOLOR);
    ctMMIOSETBGCOLOR(ctFGCOLOR);
    ctMMIOSETHEIGHTWIDTHGO(height,width*__svgalib_accel_bytesperpixel);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctMMIOBLTWAIT;
}

void __svgalib_CHIPS_hiqv_FillBox(int x, int y, int width, int height)
{
    int destaddr;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: hiqv_FillBox(%d, %d, %d, %d)\n", x, y, width, height);
#endif

    destaddr = BLTBYTEADDRESS(x, y);
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctHIQVBLTWAIT;
    ctHIQVSETDSTADDR(destaddr);
    ctHIQVSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctHIQVSETROP(ctAluConv2[ctROP&0xF] | ctHIQVTOP2BOTTOM | ctHIQVLEFT2RIGHT | 
	     ctHIQVPATSOLID | ctHIQVPATMONO);
    ctHIQVSETFGCOLOR(ctFGCOLOR);
    ctHIQVSETBGCOLOR(ctFGCOLOR);
    ctHIQVSETHEIGHTWIDTHGO(height,width*__svgalib_accel_bytesperpixel);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctHIQVBLTWAIT;
}

void __svgalib_CHIPS_FillBox24(int x, int y, int width, int height)
{
    static unsigned int dwords[3] = { 0x24499224, 0x92244992, 0x49922449};
    unsigned char pixel1, pixel2, pixel3, fgpixel, bgpixel, xorpixel;
    unsigned int i, op, index, line, destaddr;
    Bool fastfill;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: FillBox24(%d, %d, %d, %d)\n", x, y, width, height);
#endif

    pixel3 = ctFGCOLOR & 0xFF;
    pixel2 = (ctFGCOLOR >> 8) & 0xFF;
    pixel1 = (ctFGCOLOR >> 16) & 0xFF;
    fgpixel = pixel1;
    bgpixel = pixel2;
    xorpixel = 0;
    index = 0;
    fastfill = FALSE;

    /* Test for the special case where two of the byte of the 
     * 24bpp colour are the same. This can double the speed
     */
    if (pixel1 == pixel2) {
	fgpixel = pixel3;
	bgpixel = pixel1;
	fastfill = TRUE;
	index = 1;
    } else if (pixel1 == pixel3) { 
	fgpixel = pixel2;
	bgpixel = pixel1;
	fastfill = TRUE;
	index = 2;
    } else if (pixel2 == pixel3) { 
	fastfill = TRUE;
    } else {
	xorpixel = pixel2 ^ pixel3;
    }

    /* Set up the invariant BitBLT parameters. */
    op = ctSRCMONO | ctSRCSYSTEM | ctTOP2BOTTOM | ctLEFT2RIGHT;

    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctBLTWAIT;
    ctSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctSETSRCADDR(0);
    ctSETFGCOLOR(((((fgpixel&0xFF)<<8)|(fgpixel&0xFF))<<16)|
		     (((fgpixel&0xFF)<<8)|(fgpixel&0xFF)));
    ctSETBGCOLOR(((((bgpixel&0xFF)<<8)|(bgpixel&0xFF))<<16)|
		     (((bgpixel&0xFF)<<8)|(bgpixel&0xFF)));
    destaddr = BLTBYTEADDRESS(x, y);
    ctSETDSTADDR(destaddr);
    ctSETROP(op | ctAluConv[ROP_COPY & 0xf]);
    SIGNALBLOCK;
    ctSETHEIGHTWIDTHGO(height, 3 * width);
    line = 0;
    while (line < height) {
	for (i = 0; i < (((3 * width + 31) & ~31) >> 5); i++) {
	    *(unsigned int *)__svgalib_graph_mem = dwords[((index + i) % 3)];
	}
	line++;
    }
    if (!fastfill) {
	ctBLTWAIT;
	ctSETFGCOLOR(((((xorpixel&0xFF)<<8)|(xorpixel&0xFF))<<16)|
			 (((xorpixel&0xFF)<<8)|(xorpixel&0xFF)));
	ctSETROP(op | ctAluConv[ROP_XOR & 0xf] | ctBGTRANSPARENT);
	ctSETDSTADDR(destaddr);
	ctSETHEIGHTWIDTHGO(height, 3 * width);
	line = 0;
	while (line < height) {
	    for (i = 0; i < (((3 * width + 31) & ~31) >> 5); i++) {
		*(unsigned int *)__svgalib_graph_mem = dwords[((1 + i) % 3)];
	    }
	    line++;
	}
    }
    SIGNALUNBLOCK;
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	ctBLTWAIT;
}

void __svgalib_CHIPS_mmio_FillBox24(int x, int y, int width, int height)
{
    static unsigned int dwords[3] = { 0x24499224, 0x92244992, 0x49922449};
    unsigned char pixel1, pixel2, pixel3, fgpixel, bgpixel, xorpixel;
    unsigned int i, op, index, line, destaddr;
    Bool fastfill;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: mmio_FillBox24(%d, %d, %d, %d)\n", x, y, width, height);
#endif

    pixel3 = ctFGCOLOR & 0xFF;
    pixel2 = (ctFGCOLOR >> 8) & 0xFF;
    pixel1 = (ctFGCOLOR >> 16) & 0xFF;
    fgpixel = pixel1;
    bgpixel = pixel2;
    xorpixel = 0;
    index = 0;
    fastfill = FALSE;

    /* Test for the special case where two of the byte of the 
     * 24bpp colour are the same. This can double the speed
     */
    if (pixel1 == pixel2) {
	fgpixel = pixel3;
	bgpixel = pixel1;
	fastfill = TRUE;
	index = 1;
    } else if (pixel1 == pixel3) { 
	fgpixel = pixel2;
	bgpixel = pixel1;
	fastfill = TRUE;
	index = 2;
    } else if (pixel2 == pixel3) { 
	fastfill = TRUE;
    } else {
	xorpixel = pixel2 ^ pixel3;
    }

    /* Set up the invariant BitBLT parameters. */
    op = ctSRCMONO | ctSRCSYSTEM | ctTOP2BOTTOM | ctLEFT2RIGHT;

    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctMMIOBLTWAIT;
    ctMMIOSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctMMIOSETSRCADDR(0);
    ctMMIOSETFGCOLOR(((((fgpixel&0xFF)<<8)|(fgpixel&0xFF))<<16)|
		     (((fgpixel&0xFF)<<8)|(fgpixel&0xFF)));
    ctMMIOSETBGCOLOR(((((bgpixel&0xFF)<<8)|(bgpixel&0xFF))<<16)|
		     (((bgpixel&0xFF)<<8)|(bgpixel&0xFF)));
    destaddr = BLTBYTEADDRESS(x, y);
    ctMMIOSETDSTADDR(destaddr);
    ctMMIOSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctMMIOSETROP(op | ctAluConv[ROP_COPY & 0xf]);
    SIGNALBLOCK;
    ctMMIOSETHEIGHTWIDTHGO(height, 3 * width);
    line = 0;
    while (line < height) {
	for (i = 0; i < (((3 * width + 31) & ~31) >> 5); i++) {
	    *(unsigned int *)__svgalib_graph_mem = dwords[((index + i) % 3)];
	}
	line++;
    }
    if (!fastfill) {
	ctMMIOBLTWAIT;
	ctMMIOSETFGCOLOR(((((xorpixel&0xFF)<<8)|(xorpixel&0xFF))<<16)|
			 (((xorpixel&0xFF)<<8)|(xorpixel&0xFF)));
	ctMMIOSETROP(op | ctAluConv[ROP_XOR & 0xf] | ctBGTRANSPARENT);
	ctMMIOSETDSTADDR(destaddr);
	ctMMIOSETHEIGHTWIDTHGO(height, 3 * width);
	line = 0;
	while (line < height) {
	    for (i = 0; i < (((3 * width + 31) & ~31) >> 5); i++) {
		*(unsigned int *)__svgalib_graph_mem = dwords[((1 + i) % 3)];
	    }
	    line++;
	}
    }
    SIGNALUNBLOCK;
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
	ctMMIOBLTWAIT;
}


void __svgalib_CHIPS_PutBitmap(int x, int y, int w, int h, void *bitmap)
{
    int destaddr, line;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: PutBitMap(%d, %d, %d, %d)\n", x, y, w, h);
#endif

    destaddr = BLTBYTEADDRESS(x, y);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctBLTWAIT;
    ctSETSRCADDR(0);
    ctSETFGCOLOR(ctFGCOLOR);
    ctSETBGCOLOR(ctBGCOLOR);
    ctSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctSETDSTADDR(destaddr);
    ctSETROP(ctAluConv[ctROP & 0xf] | ctSRCMONO | ctSRCSYSTEM |
	     ctTRANSMODE | ctTOP2BOTTOM | ctLEFT2RIGHT);
    SIGNALBLOCK;
    ctSETHEIGHTWIDTHGO(h,w*__svgalib_accel_bytesperpixel);
    line = 0;
    while (line < h) {
	unsigned int i;
	for (i = 0; i < (((w + 31) & ~31) >> 5); i++) { 
	    *(unsigned int *)__svgalib_graph_mem =
	        __svgalib_byte_reversed[*(unsigned char *)bitmap] +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 1)]) << 8) +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 2)]) << 16) +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 3)]) << 24);
	    bitmap += 4;
	}
	line++;
    }
    SIGNALUNBLOCK;
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctBLTWAIT;
}


void __svgalib_CHIPS_mmio_PutBitmap(int x, int y, int w, int h, void *bitmap)
{
    int destaddr, line;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: mmio_PutBitMap(%d, %d, %d, %d)\n", x, y, w, h);
#endif

    destaddr = BLTBYTEADDRESS(x, y);

    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctMMIOBLTWAIT;
    ctMMIOSETSRCADDR(0);
    ctMMIOSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctMMIOSETDSTADDR(destaddr);
    ctMMIOSETFGCOLOR(ctFGCOLOR);
    ctMMIOSETBGCOLOR(ctBGCOLOR);
    ctMMIOSETROP(ctAluConv[ctROP & 0xf] | ctSRCMONO | ctSRCSYSTEM |
	     ctTRANSMODE | ctTOP2BOTTOM | ctLEFT2RIGHT);
    SIGNALBLOCK;
    ctMMIOSETHEIGHTWIDTHGO(h,w*__svgalib_accel_bytesperpixel);
    if (__svgalib_ctMMIOPage != -1) vga_setpage(0);
    line = 0;
    while (line < h) {
	unsigned int i;
	for (i = 0; i < (((w + 31) & ~31) >> 5); i++) { 
	    *(unsigned int *)__svgalib_graph_mem =
	        __svgalib_byte_reversed[*(unsigned char *)bitmap] +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 1)]) << 8) +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 2)]) << 16) +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 3)]) << 24);
	    bitmap += 4;
	}
	line++;
    }
    SIGNALUNBLOCK;
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctMMIOBLTWAIT;
}

/* This routine is completely untested. I have no idea if this might
 * work. I don't have a 6555x machine to test it on
 */
void __svgalib_CHIPS_hiqv_PutBitmap(int x, int y, int w, int h, void *bitmap)
{
    int destaddr, line;

#ifdef DEBUG
    fprintf(stderr,"CHIPS: hiqv_PutBitMap(%d, %d, %d, %d)\n", x, y, w, h);
#endif

    destaddr = BLTBYTEADDRESS(x, y);

    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (__svgalib_accel_mode & BLITS_IN_BACKGROUND)
        ctHIQVBLTWAIT;
    ctHIQVSETMONOCTL(ctHIQVDWORDALIGN);
    ctHIQVSETSRCADDR(0);
    ctHIQVSETPITCH(0, __svgalib_accel_screenpitchinbytes);
    ctHIQVSETDSTADDR(destaddr);
    ctHIQVSETFGCOLOR(ctFGCOLOR);
    ctHIQVSETBGCOLOR(ctBGCOLOR);
    ctHIQVSETROP(ctAluConv[ctROP & 0xf] | ctHIQVSRCMONO | ctHIQVSRCSYSTEM |
	     ctTRANSMODE | ctHIQVTOP2BOTTOM | ctHIQVLEFT2RIGHT);

    SIGNALBLOCK;
    ctHIQVSETHEIGHTWIDTHGO(h,w*__svgalib_accel_bytesperpixel);
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage + 1);
    line = 0;
    while (line < h) {
	unsigned int i;
	for (i = 0; i < (((w + 31) & ~31) >> 5); i++) { 
	    *(unsigned int *)__svgalib_ctBltDataWindow =
	        __svgalib_byte_reversed[*(unsigned char *)bitmap] +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 1)]) << 8) +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 2)]) << 16) +
	        ((__svgalib_byte_reversed[*(unsigned char *)(bitmap + 3)]) << 24);
	    bitmap += 4;
	}
	line++;
    }

    /* HiQV architecture needs a multiple of Quad-words in total */
    if (((((w + 31) & ~31) >> 5) * h) & 0x1)
        *(unsigned int *)__svgalib_ctBltDataWindow = 0;
    
    SIGNALUNBLOCK;
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    if (!(__svgalib_accel_mode & BLITS_IN_BACKGROUND))
        ctHIQVBLTWAIT;
}

static void CHIPS_Sync(void)
{
    ctBLTWAIT;
}

static void CHIPS_mmio_Sync(void)
{
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    ctMMIOBLTWAIT;
}

static void CHIPS_hiqv_Sync(void)
{
    if (__svgalib_ctMMIOPage != -1) vga_setpage(__svgalib_ctMMIOPage);
    ctHIQVBLTWAIT;
}

void __svgalib_CHIPS_SetFGColor(int fg)
{
    if (ctisHiQV) { 
	switch(__svgalib_accel_bytesperpixel) {
	  case 1:
	    ctFGCOLOR = fg&0xFF;
	    break;
	  case 2:
	    ctFGCOLOR = fg&0xFFFF;
	    break;
	  case 3:
	    ctFGCOLOR = fg&0xFFFFFF;
	    break;
	}
    } else {
	switch (__svgalib_accel_bytesperpixel) {
	  case 1:
	    ctFGCOLOR = ((((fg&0xFF)<<8) | (fg&0xFF)) << 16) |
	        (((fg&0xFF)<<8) | (fg&0xFF));
	    break;
	  case 2:
	    ctFGCOLOR = ((fg&0xFFFF) << 16) | (fg&0xFFFF);
	    break;
	  case 3:
	    /* We don't have 24bpp colour expansion. The 8bpp engine
	     * can be used to simulate this though.
	     */
	    ctFGCOLOR = fg;
	    break;
	}
    }
}

void __svgalib_CHIPS_SetBGColor(int bg)
{
    if (ctisHiQV) { 
	switch(__svgalib_accel_bytesperpixel) {
	  case 1:
	    ctBGCOLOR = bg&0xFF;
	    break;
	  case 2:
	    ctBGCOLOR = bg&0xFFFF;
	    break;
	  case 3:
	    ctBGCOLOR = bg&0xFFFFFF;
	    break;
	}
    } else {
	switch (__svgalib_accel_bytesperpixel) {
	  case 1:
	    ctBGCOLOR = ((((bg&0xFF)<<8) | (bg&0xFF)) << 16) |
	        (((bg&0xFF)<<8) | (bg&0xFF));
	    break;
	  case 2:
	    ctBGCOLOR = ((bg&0xFFFF) << 16) | (bg&0xFFFF);
	    break;
	  case 3:
	    /* We don't have 24bpp colour expansion. The 8bpp engine
	     * can be used to simulate this though.
	     */
	    ctBGCOLOR = bg;
	    break;
	}
    }
}

void __svgalib_CHIPS_SetRasterOp(int rop)
{
    ctROP = rop;
}


void __svgalib_CHIPS_SetTransparency(int mode, int color)
{
    if (mode == DISABLE_BITMAP_TRANSPARENCY) {
	ctTRANSMODE = 0;
	return;
    }
    if (ctisHiQV)
        ctTRANSMODE = ctHIQVBGTRANSPARENT;
    else
        ctTRANSMODE = ctBGTRANSPARENT;
}

static int ct_video_mode(int bpp, int weight_green, int display_size)
{   /* table+code from XFree86 ct_driver.c */
    /*     4 bpp  8 bpp  16 bpp  18 bpp  24 bpp  32 bpp */
    /* 640  0x20   0x30    0x40    -      0x50     -    */
    /* 800  0x22   0x32    0x42    -      0x52     -    */
    /*1024  0x24   0x34    0x44    -      0x54     -    */
    /*1152  0x27   0x37    0x47    -      0x57     -    */
    /*1280  0x28   0x38    0x49    -        -      -    */
    /*1600  0x2C   0x3C    0x4C   0x5D      -      -    */
    /*This value is only for BIOS.... */
    int video_mode = 0;

    switch (bpp) {
    case 4:
	video_mode = 0x20;
	break;
    case 8:
	video_mode = 0x30;
	break;
    case 16:
	video_mode = 0x40;
	if (weight_green != 5)
	    video_mode |= 0x01;
	break;
    default:
	video_mode = 0x50;
	break;
    }

    switch (display_size) {
    case 800:
	video_mode |= 0x02;
	break;
    case 1024:
	video_mode |= 0x04;
	break;
    case 1152:
	video_mode |= 0x07;
	break;
    case 1280:
	video_mode |= 0x08;
	if (bpp == 16)
	    video_mode |= 0x01;
	break;
    case 1600:
	video_mode |= 0x0C;
	if (bpp == 16)
	    video_mode |= 0x01;
	break;
    }

    return(video_mode);
}
