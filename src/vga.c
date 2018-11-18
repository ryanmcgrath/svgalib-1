/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright (C) 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* Changes by Michael Weller. */
/* Modified by Don Secrest to include Tseng ET6000 handling */
/* Changes around the config things by 101 (Attila Lendvai) */

/* The code is a bit of a mess; also note that the drawing functions */
/* are not speed optimized (the gl functions are much faster). */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <sys/wait.h>
#include <sys/syslog.h>
#include <sys/io.h>

#ifdef INCLUDE_VESA_DRIVER
#include <sys/vm86.h>
#include "lrmi.h"
#endif

#include <errno.h>
#include <ctype.h>
#include "libvga.h"
#include "driver.h"
#include "vgapci.h"
#include "vganullio.h"
#include "vgaio.h"
#include "vga_console.h"
#include "mouse/vgamouse.h"
#include "keyboard/vgakeyboard.h"
#include "vgaversion.h"
#include "svgalib_helper.h"

/* Delay in microseconds after a mode is set (screen is blanked during this */
/* time), allows video signals to stabilize */
#define MODESWITCHDELAY 150000

/* Define this to disable video output during mode switches, in addition to */
/* 'turning off the screen', which is always done. */
/* Doesn't look very nice on my Cirrus. */
/* #define DISABLE_VIDEO_OUTPUT */

/* #define DONT_WAIT_VC_ACTIVE */

/* Use /dev/tty instead of /dev/tty0 (the previous behaviour may have been
 * silly). */
#define USE_DEVTTY

//#define SET_TERMIO

#define SETSIG(sa, sig, fun) {\
	sa.sa_handler = fun; \
	sa.sa_flags = SA_RESTART; \
	zero_sa_mask(&(sa.sa_mask)); \
	sigaction(sig, &sa, NULL); \
}

#undef SAVEREGS
#ifdef SAVEREGS
static unsigned char graph_regs[16384];
#endif

/* variables used to shift between monchrome and color emulation */
int __svgalib_CRT_I;			/* current CRT index register address */
int __svgalib_CRT_D;			/* current CRT data register address */
int __svgalib_IS1_R;			/* current input status register address */
static int color_text;		/* true if color text emulation */

uint8_t *BANKED_POINTER=NULL, *LINEAR_POINTER=NULL;
uint8_t *MMIO_POINTER;
uint8_t *SPARSE_MMIO;
static int mmio_mapped=0, mem_mapped=0;
unsigned long __svgalib_banked_mem_base, __svgalib_banked_mem_size;
unsigned long __svgalib_mmio_base, __svgalib_mmio_size=0;
unsigned long __svgalib_linear_mem_base=0, __svgalib_linear_mem_size=0;
unsigned long __svgalib_linear_mem_phys_addr=0;
int __svgalib_linear_mem_fd = -1;

#ifdef _SVGALIB_LRMI
LRMI_callbacks * __svgalib_LRMI_callbacks = NULL;
#endif

/* If == 0 then nothing is defined by the user... */
int __svgalib_default_mode = 0;

struct vgainfo infotable[] =
{
    {80, 25, 16, 160, 0},	/* VGAlib VGA modes */
    {320, 200, 16, 40, 0},
    {640, 200, 16, 80, 0},
    {640, 350, 16, 80, 0},
    {640, 480, 16, 80, 0},
    {320, 200, 256, 320, 1},
    {320, 240, 256, 80, 0},
    {320, 400, 256, 80, 0},
    {360, 480, 256, 90, 0},
    {640, 480, 2, 80, 0},

    {640, 480, 256, 640, 1},	/* VGAlib SVGA modes */
    {800, 600, 256, 800, 1},
    {1024, 768, 256, 1024, 1},
    {1280, 1024, 256, 1280, 1},

    {320, 200, 1 << 15, 640, 2},	/* Hicolor/truecolor modes */
    {320, 200, 1 << 16, 640, 2},
    {320, 200, 1 << 24, 320 * 3, 3},
    {640, 480, 1 << 15, 640 * 2, 2},
    {640, 480, 1 << 16, 640 * 2, 2},
    {640, 480, 1 << 24, 640 * 3, 3},
    {800, 600, 1 << 15, 800 * 2, 2},
    {800, 600, 1 << 16, 800 * 2, 2},
    {800, 600, 1 << 24, 800 * 3, 3},
    {1024, 768, 1 << 15, 1024 * 2, 2},
    {1024, 768, 1 << 16, 1024 * 2, 2},
    {1024, 768, 1 << 24, 1024 * 3, 3},
    {1280, 1024, 1 << 15, 1280 * 2, 2},
    {1280, 1024, 1 << 16, 1280 * 2, 2},
    {1280, 1024, 1 << 24, 1280 * 3, 3},

    {800, 600, 16, 100, 0},	/* SVGA 16-color modes */
    {1024, 768, 16, 128, 0},
    {1280, 1024, 16, 160, 0},

    {720, 348, 2, 90, 0},	/* Hercules emulation mode */

    {320, 200, 1 << 24, 320 * 4, 4},
    {640, 480, 1 << 24, 640 * 4, 4},
    {800, 600, 1 << 24, 800 * 4, 4},
    {1024, 768, 1 << 24, 1024 * 4, 4},
    {1280, 1024, 1 << 24, 1280 * 4, 4},

    {1152, 864, 16, 144, 0},
    {1152, 864, 256, 1152, 1},
    {1152, 864, 1 << 15, 1152 * 2, 2},
    {1152, 864, 1 << 16, 1152 * 2, 2},
    {1152, 864, 1 << 24, 1152 * 3, 3},
    {1152, 864, 1 << 24, 1152 * 4, 4},

    {1600, 1200, 16, 200, 0},
    {1600, 1200, 256, 1600, 1},
    {1600, 1200, 1 << 15, 1600 * 2, 2},
    {1600, 1200, 1 << 16, 1600 * 2, 2},
    {1600, 1200, 1 << 24, 1600 * 3, 3},
    {1600, 1200, 1 << 24, 1600 * 4, 4},

    {320, 240, 256, 320, 1},	
    {320, 240, 1<<15, 320*2, 2},
    {320, 240, 1<<16, 320*2, 2},
    {320, 240, 1<<24, 320*3, 3},
    {320, 240, 1<<24, 320*4, 4},
     
    {400, 300, 256, 400, 1},
    {400, 300, 1<<15, 400*2, 2},
    {400, 300, 1<<16, 400*2, 2},
    {400, 300, 1<<24, 400*3, 3},
    {400, 300, 1<<24, 400*4, 4},
     
    {512, 384, 256, 512, 1},		
    {512, 384, 1<<15, 512*2, 2},
    {512, 384, 1<<16, 512*2, 2},
    {512, 384, 1<<24, 512*3, 3},
    {512, 384, 1<<24, 512*4, 4},

    {960, 720, 256, 960, 1},		
    {960, 720, 1<<15, 960*2, 2},
    {960, 720, 1<<16, 960*2, 2},
    {960, 720, 1<<24, 960*3, 3},
    {960, 720, 1<<24, 960*4, 4},

    {1920, 1440, 256, 1920, 1},		
    {1920, 1440, 1<<15, 1920*2, 2},
    {1920, 1440, 1<<16, 1920*2, 2},
    {1920, 1440, 1<<24, 1920*3, 3},
    {1920, 1440, 1<<24, 1920*4, 4},

    {320, 400, 1<<8,  320,   1},
    {320, 400, 1<<15, 320*2, 2},
    {320, 400, 1<<16, 320*2, 2},
    {320, 400, 1<<24, 320*3, 3},
    {320, 400, 1<<24, 320*4, 4},

    {640, 400, 256, 640, 1},
    {640, 400, 1<<15, 640*2, 2},
    {640, 400, 1<<16, 640*2, 2},
    {640, 400, 1<<24, 640*3, 3},
    {640, 400, 1<<24, 640*4, 4},

    {320, 480, 256, 320, 1},
    {320, 480, 1<<15, 320*2, 2},
    {320, 480, 1<<16, 320*2, 2},
    {320, 480, 1<<24, 320*3, 3},
    {320, 480, 1<<24, 320*4, 4},

    {720, 540, 256, 720, 1},
    {720, 540, 1<<15, 720*2, 2},
    {720, 540, 1<<16, 720*2, 2},
    {720, 540, 1<<24, 720*3, 3},
    {720, 540, 1<<24, 720*4, 4},

    {848, 480, 256, 848, 1},
    {848, 480, 1<<15, 848*2, 2},
    {848, 480, 1<<16, 848*2, 2},
    {848, 480, 1<<24, 848*3, 3},
    {848, 480, 1<<24, 848*4, 4},

    {1072, 600, 256, 1072, 1},
    {1072, 600, 1<<15, 1072*2, 2},
    {1072, 600, 1<<16, 1072*2, 2},
    {1072, 600, 1<<24, 1072*3, 3},
    {1072, 600, 1<<24, 1072*4, 4},

    {1280, 720, 256, 1280, 1},
    {1280, 720, 1<<15, 1280*2, 2},
    {1280, 720, 1<<16, 1280*2, 2},
    {1280, 720, 1<<24, 1280*3, 3},
    {1280, 720, 1<<24, 1280*4, 4},

    {1360, 768, 256, 1360, 1},
    {1360, 768, 1<<15, 1360*2, 2},
    {1360, 768, 1<<16, 1360*2, 2},
    {1360, 768, 1<<24, 1360*3, 3},
    {1360, 768, 1<<24, 1360*4, 4},

    {1800, 1012, 256, 1800, 1},
    {1800, 1012, 1<<15, 1800*2, 2},
    {1800, 1012, 1<<16, 1800*2, 2},
    {1800, 1012, 1<<24, 1800*3, 3},
    {1800, 1012, 1<<24, 1800*4, 4},

    {1920, 1080, 256, 1920, 1},
    {1920, 1080, 1<<15, 1920*2, 2},
    {1920, 1080, 1<<16, 1920*2, 2},
    {1920, 1080, 1<<24, 1920*3, 3},
    {1920, 1080, 1<<24, 1920*4, 4},

    {2048, 1152, 256, 2048, 1},
    {2048, 1152, 1<<15, 2048*2, 2},
    {2048, 1152, 1<<16, 2048*2, 2},
    {2048, 1152, 1<<24, 2048*3, 3},
    {2048, 1152, 1<<24, 2048*4, 4},

    {2048, 1536, 256, 2048, 1},
    {2048, 1536, 1<<15, 2048*2, 2},
    {2048, 1536, 1<<16, 2048*2, 2},
    {2048, 1536, 1<<24, 2048*3, 3},
    {2048, 1536, 1<<24, 2048*4, 4},

    {512, 480, 256, 512, 1},		
    {512, 480, 1<<15, 512*2, 2},
    {512, 480, 1<<16, 512*2, 2},
    {512, 480, 1<<24, 512*3, 3},
    {512, 480, 1<<24, 512*4, 4},

    {400, 600, 256, 400, 1},
    {400, 600, 1<<15, 400*2, 2},
    {400, 600, 1<<16, 400*2, 2},
    {400, 600, 1<<24, 400*3, 3},
    {400, 600, 1<<24, 400*4, 4},

    {400, 300, 256, 100, 0},
    {320, 200, 256, 320, 1},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0}
};

#define MAX_MODES (sizeof(infotable) / sizeof(struct vgainfo))

void (*__svgalib_go_to_background) (void) = 0;
void (*__svgalib_come_from_background) (void) = 0;
static int release_acquire=0;

unsigned char __svgalib_novga = 0;     /* Does not have VGA circuitry on board */
unsigned char __svgalib_vesatext = 0;
unsigned char __svgalib_textprog = 0;  /* run a program when returning to text mode */
unsigned char __svgalib_secondary = 0; /* this is not the main card with VC'S ) */
unsigned char __svgalib_emulatepage = 0; /* don't use 0xa0000 memory */
unsigned char __svgalib_novccontrol = 0; /* this is not the main card with VC'S  */
unsigned char __svgalib_ragedoubleclock = 0;
unsigned char __svgalib_simple = 0;
unsigned char __svgalib_neolibretto100 = 0;
unsigned char __svgalib_nohelper = 0;
static unsigned char __svgalib_nohelper_secure = 1;
unsigned char __svgalib_fbdev_novga = 0;

int biosparams=0;
int biosparam[16];

/* default palette values */
static const unsigned char default_red[256]
=
{0, 0, 0, 0, 42, 42, 42, 42, 21, 21, 21, 21, 63, 63, 63, 63,
 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63,
 0, 16, 31, 47, 63, 63, 63, 63, 63, 63, 63, 63, 63, 47, 31, 16,
 0, 0, 0, 0, 0, 0, 0, 0, 31, 39, 47, 55, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 55, 47, 39, 31, 31, 31, 31, 31, 31, 31, 31,
 45, 49, 54, 58, 63, 63, 63, 63, 63, 63, 63, 63, 63, 58, 54, 49,
 45, 45, 45, 45, 45, 45, 45, 45, 0, 7, 14, 21, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 21, 14, 7, 0, 0, 0, 0, 0, 0, 0, 0,
 14, 17, 21, 24, 28, 28, 28, 28, 28, 28, 28, 28, 28, 24, 21, 17,
 14, 14, 14, 14, 14, 14, 14, 14, 20, 22, 24, 26, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 26, 24, 22, 20, 20, 20, 20, 20, 20, 20, 20,
 0, 4, 8, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 8, 4,
 0, 0, 0, 0, 0, 0, 0, 0, 8, 10, 12, 14, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 14, 12, 10, 8, 8, 8, 8, 8, 8, 8, 8,
 11, 12, 13, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 15, 13, 12,
 11, 11, 11, 11, 11, 11, 11, 11, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned char default_green[256]
=
{0, 0, 42, 42, 0, 0, 21, 42, 21, 21, 63, 63, 21, 21, 63, 63,
 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 31, 47, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 47, 31, 16, 31, 31, 31, 31, 31, 31, 31, 31,
 31, 39, 47, 55, 63, 63, 63, 63, 63, 63, 63, 63, 63, 55, 47, 39,
 45, 45, 45, 45, 45, 45, 45, 45, 45, 49, 54, 58, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 58, 54, 49, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 7, 14, 21, 29, 28, 28, 28, 28, 28, 28, 28, 28, 21, 14, 7,
 14, 14, 14, 14, 14, 14, 14, 14, 14, 17, 21, 24, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 24, 21, 17, 20, 20, 20, 20, 20, 20, 20, 20,
 20, 22, 24, 26, 28, 28, 28, 28, 28, 28, 28, 28, 28, 26, 24, 22,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 8, 12, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 12, 8, 4, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 10, 12, 14, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 12, 10,
 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 13, 15, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 15, 13, 12, 0, 0, 0, 0, 0, 0, 0, 0};
static const unsigned char default_blue[256]
=
{0, 42, 0, 42, 0, 42, 0, 42, 21, 63, 21, 63, 21, 63, 21, 63,
 0, 5, 8, 11, 14, 17, 20, 24, 28, 32, 36, 40, 45, 50, 56, 63,
 63, 63, 63, 63, 63, 47, 31, 16, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 16, 31, 47, 63, 63, 63, 63, 63, 63, 63, 63, 63, 55, 47, 39,
 31, 31, 31, 31, 31, 31, 31, 31, 31, 39, 47, 55, 63, 63, 63, 63,
 63, 63, 63, 63, 63, 58, 54, 49, 45, 45, 45, 45, 45, 45, 45, 45,
 45, 49, 54, 58, 63, 63, 63, 63, 28, 28, 28, 28, 28, 21, 14, 7,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 14, 21, 28, 28, 28, 28,
 28, 28, 28, 28, 28, 24, 21, 17, 14, 14, 14, 14, 14, 14, 14, 14,
 14, 17, 21, 24, 28, 28, 28, 28, 28, 28, 28, 28, 28, 26, 24, 22,
 20, 20, 20, 20, 20, 20, 20, 20, 20, 22, 24, 26, 28, 28, 28, 28,
 16, 16, 16, 16, 16, 12, 8, 4, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 4, 8, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 14, 12, 10,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 10, 12, 14, 16, 16, 16, 16,
 16, 16, 16, 16, 16, 15, 13, 12, 11, 11, 11, 11, 11, 11, 11, 11,
 11, 12, 13, 15, 16, 16, 16, 16, 0, 0, 0, 0, 0, 0, 0, 0};

static unsigned char text_regs[MAX_REGS];	/* VGA registers for saved text mode */

char *__svgalib_TextProg_argv[16]; /* should be enough */
char *__svgalib_TextProg;

/* saved text mode palette values */
static unsigned char text_red[256];
static unsigned char text_green[256];
static unsigned char text_blue[256];

/* saved graphics mode palette values */
static unsigned char graph_red[256];
static unsigned char graph_green[256];
static unsigned char graph_blue[256];

static int prv_mode = TEXT;	/* previous video mode      */
static int flip_mode = TEXT;	/* flipped video mode       */

int CM = TEXT;			/* current video mode       */
struct vgainfo CI;			/* current video parameters */
int COL;			/* current color            */

static int initialized = 0;	/* flag: initialize() called ?  */
static int flip = 0;		/* flag: executing vga_flip() ? */
static int background_fd = -1;

/* svgalib additions: */
int __svgalib_chipset = UNDEFINED;
int __svgalib_driver_report = 1;
	/* report driver used after chipset detection */
int __svgalib_videomemoryused = -1;
int __svgalib_modeX = 0;	/* true after vga_setmodeX() */
int __svgalib_modeflags = 0;	/* copy of flags for current mode */
int __svgalib_critical = 0;	/* indicates blitter is busy */
int __svgalib_screenon = 1;	/* screen visible if != 0 */
int __svgalib_vgacolormode = 1; /* assume color for now. needs to be 
    				   config file option */

static int __svgalib_savemem=0;

RefreshRange __svgalib_horizsync =
{31500U, 0U};			/* horz. refresh (Hz) min, max */
RefreshRange __svgalib_vertrefresh =
{50U, 70U};			/* vert. refresh (Hz) min, max */
int __svgalib_bandwidth=50000;  /* monitor maximum bandwidth (kHz) */
int __svgalib_grayscale = 0;	/* grayscale vs. color mode */
int __svgalib_modeinfo_linearset = 0;	/* IS_LINEAR handled via extended vga_modeinfo */
const int __svgalib_max_modes = MAX_MODES;	/* Needed for dynamical allocated tables in mach32.c */

static unsigned __svgalib_maxhsync[] =
{
    31500, 35100, 35500, 37900, 48300, 56000, 60000
};

static int minx=0, miny=0, maxx=99999, maxy=99999;
static int lastmodenumber = __GLASTMODE;	/* Last defined mode */
static int my_pid = 0;		/* process PID, used with atexit() */
static int __svgalib_readpage = -1;
static int __svgalib_writepage = -1;
static int vga_page_offset = 0;	/* offset to add to all vga_set*page() calls */
static int currentlogicalwidth;
static int currentdisplaystart;
static int mouse_support = 0;
int mouse_open = 0;
static int mouse_mode = 0;
static int mouse_type = -1;
static int mouse_modem_ctl = 0;
char *__svgalib_mouse_device = "/dev/input/mice";
int __svgalib_mouse_flag;
static char *helper_device = "/dev/svga0";
static int __svgalib_oktowrite = 1;
static int modeinfo_mask = ~0;
static int configfile_chipset = UNDEFINED;
static int configfile_params  = 0;
static int configfile_par1    = 0;
static int configfile_par2    = 0;

int __svgalib_mem_fd = -1;	/* /dev/svga file descriptor  */
int __svgalib_tty_fd = -1;	/* /dev/tty file descriptor */
int __svgalib_nosigint = 0;	/* Don't generate SIGINT in graphics mode */
int __svgalib_runinbackground = 0;
int __svgalib_startup_vc = -1;
static int __svgalib_security_revokeallprivs = 1;
static int __svgalib_security_norevokeprivs = 0;
static unsigned fontbufsize = 8192; /* compatibility */

/* Dummy buffer for mmapping grahics memory; points to 64K VGA framebuffer. */
unsigned char *GM;
/* Exported variable (read-only) is shadowed from internal variable, for */
/* better shared library performance. */
unsigned char *graph_mem;

void *__svgalib_physaddr;
int __svgalib_linear_memory_size;

static unsigned long graph_buf_size = 0;
static unsigned char *graph_buf = NULL;		/* saves graphics data during flip */

/* The format of the fontdata seems to differ between banked and LFB access,
   so we save both in case emulatepage gets set between saving and restoring.
   This can happen when using runinbackground in nohelper secure mode. */
static unsigned char *font_buf1_banked=NULL;	/* saved font data - plane 2 */
static unsigned char *font_buf2_banked=NULL;	/* saved font data - plane 3 */
static unsigned char *font_buf1_linear=NULL;	/* saved font data - plane 2 */
static unsigned char *font_buf2_linear=NULL;	/* saved font data - plane 3 */
static unsigned char *text_buf1=NULL;	/* saved text data - plane 0 */
static unsigned char *text_buf2=NULL;	/* saved text data - plane 1 */

struct termios __svgalib_text_termio;	/* text mode termio parameters     */
struct termios __svgalib_graph_termio;	/* graphics mode termio parameters */

int __svgalib_flipchar = '\x1b';		/* flip character - initially  ESCAPE */

/* Chipset specific functions */

DriverSpecs *__svgalib_driverspecs = &__svgalib_vga_driverspecs;

static void (*__svgalib_setpage) (int);	/* gives little faster vga_setpage() */
static void (*__svgalib_setrdpage) (int) = NULL;
static void (*__svgalib_setwrpage) (int) = NULL;

int  (*__svgalib_inmisc)(void);
void (*__svgalib_outmisc)(int);
int  (*__svgalib_incrtc)(int);
void (*__svgalib_outcrtc)(int,int);
int  (*__svgalib_inseq)(int);
void (*__svgalib_outseq)(int,int);
int  (*__svgalib_ingra)(int);
void (*__svgalib_outgra)(int,int);
int  (*__svgalib_inatt)(int);
void (*__svgalib_outatt)(int,int);
void (*__svgalib_attscreen)(int);
void (*__svgalib_inpal)(int,int*,int*,int*);
void (*__svgalib_outpal)(int,int,int,int);
int  (*__svgalib_inis1)(void);

static void readconfigfile(void);
inline void vga_setpage(int p);

DriverSpecs *__svgalib_driverspecslist[] =
{
    NULL,			/* chipset undefined */
    &__svgalib_vga_driverspecs,
#ifdef INCLUDE_ET4000_DRIVER
    &__svgalib_et4000_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_CIRRUS_DRIVER
    &__svgalib_cirrus_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_TVGA_DRIVER
    &__svgalib_tvga8900_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_OAK_DRIVER
    &__svgalib_oak_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_EGA_DRIVER
    &__svgalib_ega_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_S3_DRIVER
    &__svgalib_s3_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ET3000_DRIVER
    &__svgalib_et3000_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_MACH32_DRIVER
    &__svgalib_mach32_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_GVGA6400_DRIVER
    &__svgalib_gvga6400_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ARK_DRIVER
    &__svgalib_ark_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ATI_DRIVER
    &__svgalib_ati_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ALI_DRIVER
    &__svgalib_ali_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_MACH64_DRIVER
    &__svgalib_mach64_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_CHIPS_DRIVER
    &__svgalib_chips_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_APM_DRIVER
    &__svgalib_apm_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_NV3_DRIVER
    &__svgalib_nv3_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_ET6000_DRIVER
    &__svgalib_et6000_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_VESA_DRIVER
    &__svgalib_vesa_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_MX_DRIVER
    &__svgalib_mx_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_PARADISE_DRIVER
    &__svgalib_paradise_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_RAGE_DRIVER
    &__svgalib_rage_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_BANSHEE_DRIVER
    &__svgalib_banshee_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_SIS_DRIVER
    &__svgalib_sis_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_I740_DRIVER
    &__svgalib_i740_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_NEO_DRIVER
    &__svgalib_neo_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_LAGUNA_DRIVER
    &__svgalib_laguna_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_FBDEV_DRIVER
     &__svgalib_fbdev_driverspecs,
#else
     NULL,
#endif
#ifdef INCLUDE_G400_DRIVER
    &__svgalib_g400_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_R128_DRIVER
    &__svgalib_r128_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_SAVAGE_DRIVER
    &__svgalib_savage_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER
    &__svgalib_mil_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_I810_DRIVER
    &__svgalib_i810_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_TRIDENT_DRIVER
    &__svgalib_trident_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_RENDITION_DRIVER
    &__svgalib_rendition_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_G450C2_DRIVER
    &__svgalib_g450c2_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_PM2_DRIVER
    &__svgalib_pm2_driverspecs,
#else
    NULL,
#endif
#ifdef INCLUDE_UNICHROME_DRIVER
    &__svgalib_unichrome_driverspecs,
#else
    NULL,
#endif
};

static char *driver_names[] =
{
"", 
"VGA", 
"ET4000", 
"Cirrus", 
"TVGA", 
"Oak", 
"EGA", 
"S3",
"ET3000", 
"Mach32", 
"GVGA6400",
"ARK",
"ATI",
"ALI",
"Mach64", 
"C&T",
"APM",
"NV3",
"ET6000",
"VESA",
"MX",
"PARADISE",
"RAGE",
"BANSHEE", 
"SIS",
"I740",
"NEOMAGIC",
"LAGUNA",
"FBDev",
"G400",
"R128",
"SAVAGE",
"MILLENNIUM",
"I810",
"TRIDENT",
"RENDITION",
"G450C2",
"PM2",
"UNICHROME",
 NULL};

/* Chipset drivers */

/* vgadrv       Standard VGA (also used by drivers below) */
/* et4000       Tseng ET4000 (from original vgalib) */
/* cirrus       Cirrus Logic GD542x */
/* tvga8900     Trident TVGA 8900/9000 (derived from tvgalib) */
/* oak          Oak Technologies 037/067/077 */
/* egadrv       IBM EGA (subset of VGA) */
/* s3           S3 911 */
/* mach32       ATI MACH32 */
/* ark          ARK Logic */
/* gvga6400     Genoa 6400 (old SVGA) */
/* ati          ATI */
/* ali          ALI2301 */
/* mach64	ATI MACH64 */
/* chips	chips & technologies*/
/* et6000       Tseng ET6000 */         /* DS */

/*#define DEBUG */

/* Debug config file parsing.. */
/*#define DEBUG_CONF */

/* open /dev/svga */
static void open_mem(void)
{
    /*  Ensure that the open will get a file descriptor greater
     *  than 2, else problems can occur with stdio functions
     *  under certain strange conditions:  */
    if ((fcntl(0,F_GETFD) == -1) && (open("/dev/null", O_RDONLY) == -1)){
        perror("/dev/null");
        exit(1);
    }
    if ((fcntl(1,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)){
        perror("/dev/null");
        exit(1);
    }
    if ((fcntl(2,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)){
        perror("/dev/null");
        exit(1);
    }

    if (__svgalib_mem_fd == -1)
    {
        const char *device;
        if (__svgalib_nohelper)
            device = "/dev/mem";
        else
            device = helper_device;
	if ((__svgalib_mem_fd = open(device, O_RDWR)) == -1) {
	    fprintf(stderr,"svgalib: Cannot open %s\n%s\n", device,
	        __svgalib_nohelper?
	        "Are you running this program as root or suid-root?":
		"Is svgalib_helper module loaded?");
	    exit(1);
	}
    }

    /*  Ensure this file is closed if we ever exec something else...  */
    if (fcntl(__svgalib_mem_fd, F_SETFD, 1) == -1) {
	perror("fcntl 808");
 	exit(-1);
    }
    
    if (__svgalib_linear_mem_fd == -1)
        __svgalib_linear_mem_fd = __svgalib_mem_fd;
}

static void __svgalib_get_perm(void)
{
    static int done = 0;

    /* Only do this once. */
    if (done)
	return;
    done = 1;

    /* special case FBDEV without standard VGA modes */
    if ((CHIPSET==FBDEV) && __svgalib_fbdev_novga)
    {
        __svgalib_emulatepage=1;
        __svgalib_novga=1;
        __svgalib_nohelper=1;
    }
    else
    {
    if (__svgalib_nohelper)
    {
	iopl(3);
	ioperm(0, 0x400, 1);
    }
    
    /* Open /dev/svga */
    open_mem();
    }

    __svgalib_open_devconsole();

    /* color or monochrome text emulation? */
    if (CHIPSET != EGA && !__svgalib_novga)
	color_text = __svgalib_vgacolor();
    else
	color_text = 1;		/* EGA is assumed color */

    /* chose registers for color/monochrome emulation */
    if (color_text) {
	__svgalib_CRT_I = CRT_IC;
	__svgalib_CRT_D = CRT_DC;
	__svgalib_IS1_R = IS1_RC;
    } else {
	__svgalib_CRT_I = CRT_IM;
	__svgalib_CRT_D = CRT_DM;
	__svgalib_IS1_R = IS1_RM;
    }
}

static void __svgalib_giveup_perm(void)
{
    if (CHIPSET != FBDEV) {
        __svgalib_linear_mem_phys_addr = __svgalib_linear_mem_base;
        /* Try get a pure handle to the LFB which can safely be left open
           at all times, this will only work with 2.6 kernels.
           This could be done in helper mode too, but we don't know the
           real pci ID there, only the helper module id. Also this
           is not usefull in helpermode since mem_fd must always be left
           open in helpermode. */
        if (__svgalib_nohelper) {
            unsigned int bus, device, fn;
            char buf[256];
            FILE *f;
            int i, fd;
            
            bus=(__svgalib_pci_card_found_at&0xff00)>>8;
            device=(__svgalib_pci_card_found_at&0xf8)>>3;
            fn=__svgalib_pci_card_found_at&0x07;
            snprintf(buf, 256, "/sys/bus/pci/devices/0000:%02u:%02x.%u/resource",
                bus, device, fn);
                
            f = fopen(buf, "r");
            if (f) {
                for (i=0; fgets(buf, 256, f); i++) {
                    if (strtoul(buf, NULL, 16) == __svgalib_linear_mem_base) {
                       snprintf(buf, 256,
                          "/sys/bus/pci/devices/0000:%02u:%02x.%u/resource%i",
                          bus, device, fn, i);
                       fd = open(buf, O_RDWR);
                       if (fd != -1) {
                           __svgalib_linear_mem_fd = fd;
                           __svgalib_linear_mem_base = 0;
#ifdef DEBUG
                           fprintf(stderr, "svgalib: debug: Opened: %s as LFB\n",
                               buf);
#endif
                       }
                       break;
                    }
                }
                fclose(f);
            }
        }
    }

    /* mmap graphics memory */
    map_mem();
    map_mmio();

    if(__svgalib_nohelper && __svgalib_nohelper_secure &&
       (__svgalib_mem_fd != -1))
    {
        close(__svgalib_mem_fd);
        if (__svgalib_linear_mem_fd == __svgalib_mem_fd)
            __svgalib_linear_mem_fd = -1;
        __svgalib_mem_fd = -1;
    }

    if ((__svgalib_linear_mem_fd == -1) && __svgalib_emulatepage)
    {
        fprintf(stderr,
            "svgalib: Warning your config requires banking to be emulated. This is not\n"
            "  possible in NoHelper mode, applications which use bankedmode even if LFB\n"
            "  is available will fail!\n");
    }

    __svgalib_setpage = __svgalib_driverspecs->__svgalib_setpage;
    if (!__svgalib_emulatepage)
    {
        __svgalib_setrdpage = __svgalib_driverspecs->__svgalib_setrdpage;
        __svgalib_setwrpage = __svgalib_driverspecs->__svgalib_setwrpage;
    }

    /* DEBUG, REMOVEME force banking to test setpage workarounds */
    /* __svgalib_modeinfo_linearset &= ~LINEAR_USE; */
}

void __svgalib_delay(void)
{
    int i;
    for (i = 0; i < 10; i++);
}

static void slowcpy(unsigned char *dest, unsigned char *src, unsigned bytes)
{
	if(dest==NULL || src==NULL || (long)dest==-1L || (long)src==-1L) return;
    while (bytes > 0) {
		*(uint32_t *)dest = *(uint32_t *)src;
        dest+=4;
        src+=4;
        bytes-=4;
    }
}

#define TEXT_SIZE 65536

static unsigned char *vmem_buf;

static void restore_text(void)
{

          __svgalib_outseq(0x02,0x01);

	  slowcpy(GM, text_buf1, TEXT_SIZE);

          __svgalib_outseq(0x02,0x02);

	  slowcpy(GM, text_buf2, TEXT_SIZE);

   	  if(__svgalib_savemem)
              memcpy(LINEAR_POINTER,vmem_buf,__svgalib_savemem);
};

static void save_text(void)
{

   	text_buf1 = malloc(TEXT_SIZE * 2);
	
    text_buf2 = text_buf1 + TEXT_SIZE;

    __svgalib_outgra(0x04,0x00);

	slowcpy(text_buf1, GM, TEXT_SIZE);

    /* save font data in plane 3 */
    __svgalib_outgra(0x04,0x01);

    slowcpy(text_buf2, GM, TEXT_SIZE);

  	if(__svgalib_savemem) {
            vmem_buf=malloc(__svgalib_savemem);
            memcpy(vmem_buf,LINEAR_POINTER,__svgalib_savemem);
    }
};

int __svgalib_saveregs(unsigned char *regs)
{
    int i;

    if (__svgalib_chipset == EGA || __svgalib_novga) {
	/* Special case: Don't save standard VGA registers. */
	return chipset_saveregs(regs);
    }
    /* save VGA registers */
    for (i = 0; i < CRT_C; i++) {
        regs[CRT + i] = __svgalib_incrtc(i);
    }
    for (i = 0; i < ATT_C; i++) {
	regs[ATT + i] = __svgalib_inatt(i);
    }
    for (i = 0; i < GRA_C; i++) {
	regs[GRA + i] = __svgalib_ingra(i);
    }
    for (i = 0; i < SEQ_C; i++) {
	regs[SEQ + i] = __svgalib_inseq(i);
    }
    regs[MIS] = __svgalib_inmisc();

    i = chipset_saveregs(regs);	/* save chipset-specific registers */
    /* i : additional registers */
    if (!SCREENON) {		/* We turned off the screen */
        __svgalib_attscreen(0x20);
    }
    return CRT_C + ATT_C + GRA_C + SEQ_C + 1 + i;
}


int __svgalib_setregs(const unsigned char *regs)
{
    int i;

    if(__svgalib_novga) return 1;

    if (__svgalib_chipset == EGA) {
	/* Enable graphics register modification */
	port_out(0x00, GRA_E0);
	port_out(0x01, GRA_E1);
    }
    /* update misc output register */
    __svgalib_outmisc(regs[MIS]);

    /* synchronous reset on */
    __svgalib_outseq(0x00,0x01);

    /* write sequencer registers */
    __svgalib_outseq(0x01,regs[SEQ + 1] | 0x20);
    for (i = 2; i < SEQ_C; i++) {
       __svgalib_outseq(i,regs[SEQ + i]);
    }

    /* synchronous reset off */
    __svgalib_outseq(0x00,0x03);

    if (__svgalib_chipset != EGA) {
	/* deprotect CRT registers 0-7 */
        __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    }
    /* write CRT registers */
    for (i = 0; i < CRT_C; i++) {
        __svgalib_outcrtc(i,regs[CRT + i]);
    }

    /* write graphics controller registers */
    for (i = 0; i < GRA_C; i++) {
        __svgalib_outgra(i,regs[GRA+i]);
    }

    /* write attribute controller registers */
    for (i = 0; i < ATT_C; i++) {
        __svgalib_outatt(i,regs[ATT+i]);
    }

    return 0;
}

/* We invoke the old interrupt handler after setting text mode */
/* We catch all signals that cause an exit by default (aka almost all) */
static char sig2catch[] =
{SIGHUP, SIGINT, SIGQUIT, SIGILL,
 SIGTRAP, SIGIOT, SIGBUS, SIGFPE,
 SIGSEGV, SIGPIPE, SIGALRM, SIGTERM,
 SIGXCPU, SIGXFSZ, SIGVTALRM,
/* SIGPROF ,*/ SIGPWR};
static struct sigaction old_signal_handler[sizeof(sig2catch)];

struct vt_mode __svgalib_oldvtmode;

static void restoretextmode(void)
{
    /* handle unexpected interrupts - restore text mode and exit */
    keyboard_close();
    /* Restore a setting screwed by keyboard_close (if opened in graphicsmode) */
    __svgalib_set_texttermio();
    if (CM != TEXT)
	vga_setmode(TEXT);

    if (!__svgalib_screenon)
	vga_screenon();

    if (__svgalib_tty_fd >= 0) {
        if (!__svgalib_secondary) {
	    ioctl(__svgalib_tty_fd, KDSETMODE, KD_TEXT);
            ioctl(__svgalib_tty_fd, VT_SETMODE, &__svgalib_oldvtmode);
	}
    }

    if((__svgalib_textprog&3)==3){
       pid_t child;
       if((child=fork())==0){
           execv(__svgalib_TextProg,__svgalib_TextProg_argv);
       } else {
           waitpid(child,NULL,0);
       };
    };
}

static void idle_accel(void) {
    /* wait for the accel to finish, we assume one of the both interfaces suffices */
    if (vga_ext_set(VGA_EXT_AVAILABLE, VGA_AVAIL_ACCEL) & ACCELFLAG_SYNC)
        vga_accel(ACCEL_SYNC);
    else if (vga_getmodeinfo(CM)->haveblit & HAVE_BLITWAIT)
	vga_blitwait();
}

static void signal_handler(int v)
{
    int i;

    /* If we have accelerated functions, possibly wait for the
     * blitter to finish. I hope the PutBitmap functions disable
     * interrupts while writing data to the screen, otherwise
     * this will cause an infinite loop.
     */
    idle_accel();
    
    restoretextmode();
    fprintf(stderr,"svgalib: Signal %d: %s received%s.\n", v, strsignal(v),
	   (v == SIGINT) ? " (ctrl-c pressed)" : "");

    for (i = 0; i < sizeof(sig2catch); i++)
	if (sig2catch[i] == v) {
	    sigaction(v, old_signal_handler + i, NULL);
	    raise(v);
	    break;
	}
    if (i >= sizeof(sig2catch)) {
		fprintf(stderr,"svgalib: Aieeee! Illegal call to signal_handler, raising segfault.\n");
		raise(SIGSEGV);
    }
}

int __svgalib_getchipset(void)
{
    readconfigfile();		/* Make sure the config file is read. */

#ifdef _SVGALIB_LRMI
    if (!__svgalib_LRMI_callbacks)
		vga_set_LRMI_callbacks(&__svgalib_default_LRMI_callbacks);
#endif

    if (CHIPSET != UNDEFINED)
        return CHIPSET;

/* Unlike the others, the FBDev test needs to be before __svgalib_get_perm() */
#ifdef INCLUDE_FBDEV_DRIVER_TEST
    if (!__svgalib_driverspecslist[FBDEV]->disabled &&
	__svgalib_fbdev_driverspecs.test())
	CHIPSET = FBDEV;
#endif

    __svgalib_get_perm();

    if (CHIPSET == UNDEFINED) {
	CHIPSET = VGA;		/* Protect against recursion */
#ifdef INCLUDE_NV3_DRIVER_TEST
	if (!__svgalib_driverspecslist[NV3]->disabled && __svgalib_nv3_driverspecs.test())
	    CHIPSET = NV3;
	else
#endif
#ifdef INCLUDE_TRIDENT_DRIVER_TEST
	if (!__svgalib_driverspecslist[TRIDENT]->disabled && __svgalib_trident_driverspecs.test())
	    CHIPSET = TRIDENT;
	else
#endif
#ifdef INCLUDE_RENDITION_DRIVER_TEST
	if (!__svgalib_driverspecslist[RENDITION]->disabled && __svgalib_rendition_driverspecs.test())
	    CHIPSET = RENDITION;
	else
#endif
#ifdef INCLUDE_G400_DRIVER_TEST
	if (!__svgalib_driverspecslist[G400]->disabled && __svgalib_g400_driverspecs.test())
	    CHIPSET = G400;
	else
#endif
#ifdef INCLUDE_PM2_DRIVER_TEST
	if (!__svgalib_driverspecslist[PM2]->disabled && __svgalib_pm2_driverspecs.test())
	    CHIPSET = PM2;
	else
#endif
#ifdef INCLUDE_UNICHROME_DRIVER_TEST
	if (!__svgalib_driverspecslist[UNICHROME]->disabled && __svgalib_unichrome_driverspecs.test())
	    CHIPSET = UNICHROME;
	else
#endif
#ifdef INCLUDE_SAVAGE_DRIVER_TEST
	if (!__svgalib_driverspecslist[SAVAGE]->disabled && __svgalib_savage_driverspecs.test())
	    CHIPSET = SAVAGE;
	else
#endif
#ifdef INCLUDE_MILLENNIUM_DRIVER_TEST
	if (!__svgalib_driverspecslist[MILLENNIUM]->disabled && __svgalib_mil_driverspecs.test())
	    CHIPSET = MILLENNIUM;
	else
#endif
#ifdef INCLUDE_R128_DRIVER_TEST
	if (!__svgalib_driverspecslist[R128]->disabled && __svgalib_r128_driverspecs.test())
	    CHIPSET = R128;
	else
#endif
#ifdef INCLUDE_BANSHEE_DRIVER_TEST
	if (!__svgalib_driverspecslist[BANSHEE]->disabled && __svgalib_banshee_driverspecs.test())
	    CHIPSET = BANSHEE;
	else
#endif
#ifdef INCLUDE_SIS_DRIVER_TEST
	if (!__svgalib_driverspecslist[SIS]->disabled && __svgalib_sis_driverspecs.test())
	    CHIPSET = SIS;
	else
#endif
#ifdef INCLUDE_I740_DRIVER_TEST
	if (!__svgalib_driverspecslist[I740]->disabled && __svgalib_i740_driverspecs.test())
	    CHIPSET = I740;
	else
#endif
#ifdef INCLUDE_I810_DRIVER_TEST
	if (!__svgalib_driverspecslist[I810]->disabled && __svgalib_i810_driverspecs.test())
	    CHIPSET = I810;
	else
#endif
#ifdef INCLUDE_LAGUNA_DRIVER_TEST
	if (!__svgalib_driverspecslist[LAGUNA]->disabled && __svgalib_laguna_driverspecs.test())
	    CHIPSET = LAGUNA;
	else
#endif
#ifdef INCLUDE_RAGE_DRIVER_TEST
	if (!__svgalib_driverspecslist[RAGE]->disabled && __svgalib_rage_driverspecs.test())
	    CHIPSET = RAGE;
	else
#endif
#ifdef INCLUDE_MX_DRIVER_TEST
	if (!__svgalib_driverspecslist[MX]->disabled && __svgalib_mx_driverspecs.test())
	    CHIPSET = MX;
	else
#endif
#ifdef INCLUDE_NEO_DRIVER_TEST
	if (!__svgalib_driverspecslist[NEOMAGIC]->disabled && __svgalib_neo_driverspecs.test())
	    CHIPSET = NEOMAGIC;
	else
#endif
#ifdef INCLUDE_CHIPS_DRIVER_TEST
	if (!__svgalib_driverspecslist[CHIPS]->disabled && __svgalib_chips_driverspecs.test())
	    CHIPSET = CHIPS;
	else
#endif
#ifdef INCLUDE_MACH64_DRIVER_TEST
	if (!__svgalib_driverspecslist[MACH64]->disabled && __svgalib_mach64_driverspecs.test())
	    CHIPSET = MACH64;
	else
#endif
#ifdef INCLUDE_MACH32_DRIVER_TEST
	if (!__svgalib_driverspecslist[MACH32]->disabled && __svgalib_mach32_driverspecs.test())
	    CHIPSET = MACH32;
	else
#endif
#ifdef INCLUDE_EGA_DRIVER_TEST
	if (!__svgalib_driverspecslist[EGA]->disabled && __svgalib_ega_driverspecs.test())
	    CHIPSET = EGA;
	else
#endif
#ifdef INCLUDE_ET6000_DRIVER_TEST                   /* DS */
	if (!__svgalib_driverspecslist[ET6000]->disabled && __svgalib_et6000_driverspecs.test())    /* This must be before */
	    CHIPSET = ET6000;                       /* ET4000 or the card  */
	else                                        /* will be called et4k */
#endif
#ifdef INCLUDE_ET4000_DRIVER_TEST
	if (!__svgalib_driverspecslist[ET4000]->disabled && __svgalib_et4000_driverspecs.test())
	    CHIPSET = ET4000;
	else
#endif
#ifdef INCLUDE_TVGA_DRIVER_TEST
	if (!__svgalib_driverspecslist[TVGA8900]->disabled && __svgalib_tvga8900_driverspecs.test())
	    CHIPSET = TVGA8900;
	else
#endif
#ifdef INCLUDE_CIRRUS_DRIVER_TEST
	    /* The Cirrus detection is not very clean. */
	if (!__svgalib_driverspecslist[CIRRUS]->disabled && __svgalib_cirrus_driverspecs.test())
	    CHIPSET = CIRRUS;
	else
#endif
#ifdef INCLUDE_OAK_DRIVER_TEST
	if (!__svgalib_driverspecslist[OAK]->disabled && __svgalib_oak_driverspecs.test())
	    CHIPSET = OAK;
	else
#endif
#ifdef INCLUDE_PARADISE_DRIVER_TEST
	if (!__svgalib_driverspecslist[PARADISE]->disabled && __svgalib_paradise_driverspecs.test())
	    CHIPSET = PARADISE;
	else
#endif
#ifdef INCLUDE_S3_DRIVER_TEST
	if (!__svgalib_driverspecslist[S3]->disabled && __svgalib_s3_driverspecs.test())
	    CHIPSET = S3;
	else
#endif
#ifdef INCLUDE_ET3000_DRIVER_TEST
	if (!__svgalib_driverspecslist[ET3000]->disabled && __svgalib_et3000_driverspecs.test())
	    CHIPSET = ET3000;
	else
#endif
#ifdef INCLUDE_ARK_DRIVER_TEST
	if (!__svgalib_driverspecslist[ARK]->disabled && __svgalib_ark_driverspecs.test())
	    CHIPSET = ARK;
	else
#endif
#ifdef INCLUDE_GVGA6400_DRIVER_TEST
	if (!__svgalib_driverspecslist[GVGA6400]->disabled && __svgalib_gvga6400_driverspecs.test())
	    CHIPSET = GVGA6400;
	else
#endif
#ifdef INCLUDE_ATI_DRIVER_TEST
	if (!__svgalib_driverspecslist[ATI]->disabled && __svgalib_ati_driverspecs.test())
	    CHIPSET = ATI;
	else
#endif
#ifdef INCLUDE_ALI_DRIVER_TEST
	if (!__svgalib_driverspecslist[ALI]->disabled && __svgalib_ali_driverspecs.test())
	    CHIPSET = ALI;
	else
#endif
#ifdef INCLUDE_APM_DRIVER_TEST
/* Note: On certain cards this may toggle the video signal on/off which
   is ugly. Hence we test this last. */
	if (!__svgalib_driverspecslist[APM]->disabled && __svgalib_apm_driverspecs.test())
	    CHIPSET = APM;
	else
#endif
#ifdef INCLUDE_VESA_DRIVER_TEST
	if (!__svgalib_driverspecslist[VESA]->disabled && __svgalib_vesa_driverspecs.test())
	    CHIPSET = VESA;
	else
#endif
	if (!__svgalib_driverspecslist[VGA]->disabled && __svgalib_vga_driverspecs.test())
	    CHIPSET = VGA;
	else
	    /* else */
	{
	    fprintf(stderr, "svgalib: Cannot find EGA or VGA graphics device.\n");
	    exit(1);
	}
    }
    __svgalib_giveup_perm();
    return CHIPSET;
}

void vga_setchipset(int c)
{
    CHIPSET = c;

    DPRINTF("Setting chipset\n");

    if (c == UNDEFINED)
		return;
	
    if (__svgalib_driverspecslist[c] == NULL) {
		fprintf(stderr,"svgalib: Invalid chipset. The driver may not be compiled in.\n");
		CHIPSET = UNDEFINED;
		return;
    }
    
    /* we need to read the configfile here to get the correct value
       for __svgalib_nohelper and __svgalib_nohelper_secure */
    readconfigfile();
    
#ifdef _SVGALIB_LRMI
    if (!__svgalib_LRMI_callbacks)
		vga_set_LRMI_callbacks(&__svgalib_default_LRMI_callbacks);
#endif
	__svgalib_driverspecslist[c]->disabled = 0;
    __svgalib_get_perm();
    __svgalib_driverspecslist[c]->init(0, 0, 0);
    __svgalib_giveup_perm();
}

void vga_setchipsetandfeatures(int c, int par1, int par2)
{
    CHIPSET = c;

    DPRINTF("Forcing chipset and features\n");

    /* we need to read the configfile here to get the correct value
       for __svgalib_nohelper and __svgalib_nohelper_secure */
    readconfigfile();
    
#ifdef _SVGALIB_LRMI
    if (!__svgalib_LRMI_callbacks)
		vga_set_LRMI_callbacks(&__svgalib_default_LRMI_callbacks);
#endif
    __svgalib_get_perm();
    __svgalib_driverspecslist[c]->init(1, par1, par2);
    __svgalib_giveup_perm();

    DPRINTF("Finished forcing chipset and features\n");
}

void vga_disablechipset(int c)
{
    DPRINTF("Disabling chipset %i\n", c);

    if (c == UNDEFINED)
	return;
    __svgalib_driverspecslist[c]->disabled = 1;
}


static void savepalette(unsigned char *red, unsigned char *green,
			unsigned char *blue)
{
    int i;

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->savepalette) 
        return (__svgalib_driverspecs->emul->savepalette(red, green, blue));

    if (CHIPSET == EGA || __svgalib_novga) 
	return;

    /* save graphics mode palette */

    for (i = 0; i < 256; i++) {
        int r,g,b;
        __svgalib_inpal(i,&r,&g,&b);
	*(red++) = r;
	*(green++) = g;
	*(blue++) = b;
    }
}

static void restorepalette(const unsigned char *red,
		   const unsigned char *green, const unsigned char *blue)
{
    int i;

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->restorepalette) 
        return (__svgalib_driverspecs->emul->restorepalette(red, green, blue));

    if (CHIPSET == EGA || __svgalib_novga)
	return;

    /* restore saved palette */
    /* read RGB components - index is autoincremented */
    for (i = 0; i < 256; i++) {
	__svgalib_outpal(i,*(red++),*(green++),*(blue++));
    }
}

/* Virtual console switching */
static int forbidvtrelease = 0;
static int forbidvtacquire = 0;
static int lock_count = 0;
static int release_flag = 0;

static void __svgalib_takevtcontrol(void);

void __svgalib_flipaway(void);
static void __svgalib_flipback(void);

int inrestore;

static void __svgalib_releasevt_signal(int n)
{
    DPRINTF("Release request. r_a=%i  %i\n", release_acquire, inrestore);

    if (lock_count) {
	release_flag = 1;
	return;
    }
    
    release_acquire=!release_acquire;
    forbidvtacquire = 1;
    if (forbidvtrelease) {
	forbidvtacquire = 0;
	ioctl(__svgalib_tty_fd, VT_RELDISP, 0);
	return;
    }

    if (__svgalib_go_to_background) (__svgalib_go_to_background) ();

    __svgalib_flipaway();

    if((__svgalib_textprog&3)==3){
        pid_t child;
       if((child=fork())==0){
           execv(__svgalib_TextProg,__svgalib_TextProg_argv);
       } else {
           waitpid(child,NULL,0);
       };  
    };

    ioctl(__svgalib_tty_fd, VT_RELDISP, 1);

    DPRINTF("Finished release.\n");

    forbidvtacquire = 0;

    /* Suspend program until switched to again. */
    if (!__svgalib_runinbackground) {
        DPRINTF("Suspended.\n");
        __svgalib_oktowrite = 0;
		__svgalib_waitvtactive();
        DPRINTF("Waked.\n");
    }
}

static void __svgalib_acquirevt_signal(int n)
{
    DPRINTF("Acquisition request. r_a=%i  %i\n",release_acquire, inrestore);

    release_acquire=!release_acquire;
    
    forbidvtrelease = 1;
    if (forbidvtacquire) {
	forbidvtrelease = 0;
	return;
    }

    __svgalib_flipback();
    ioctl(__svgalib_tty_fd, VT_RELDISP, VT_ACKACQ);

    DPRINTF("Finished acquisition.\n");

    forbidvtrelease = 0;
    if (__svgalib_come_from_background)
	(__svgalib_come_from_background) ();
    __svgalib_oktowrite = 1;
}

void __svgalib_takevtcontrol(void)
{
    struct sigaction siga;
    struct vt_mode newvtmode;

    ioctl(__svgalib_tty_fd, VT_GETMODE, &__svgalib_oldvtmode);
    newvtmode = __svgalib_oldvtmode;
    newvtmode.mode = VT_PROCESS;	/* handle VT changes */
    newvtmode.relsig = SVGALIB_RELEASE_SIG;	/* I didn't find SIGUSR1/2 anywhere */
    newvtmode.acqsig = SVGALIB_ACQUIRE_SIG;	/* in the kernel sources, so I guess */
    /* they are free */
    SETSIG(siga, SVGALIB_RELEASE_SIG, __svgalib_releasevt_signal);
    SETSIG(siga, SVGALIB_ACQUIRE_SIG, __svgalib_acquirevt_signal); 
    ioctl(__svgalib_tty_fd, VT_SETMODE, &newvtmode);
}

#ifdef LINEAR_DEBUG
void dump_mem(unsigned char *name)
{
  unsigned char bu[128];
  sprintf(bu,"cat /proc/%d/maps > /tmp/%s",getpid(),name);
  system(bu);
}
#endif

static void __vga_map(void)
{
    GM = (unsigned char *) BANKED_POINTER;
    graph_mem = GM;	/* Exported variable. */
}

static void __vga_atexit(void)
{
    if (getpid() == my_pid)	/* protect against forked processes */
	restoretextmode();
    if (__svgalib_tty_fd >= 0 && __svgalib_startup_vc > 0)
	    ioctl(__svgalib_tty_fd, VT_ACTIVATE, __svgalib_startup_vc);
}

static void setcoloremulation(void)
{
    /* shift to color emulation */
    __svgalib_CRT_I = CRT_IC;
    __svgalib_CRT_D = CRT_DC;
    __svgalib_IS1_R = IS1_RC;
    __svgalib_vgacolormode=1;
    if (CHIPSET != EGA && !__svgalib_novga)  
        __svgalib_outmisc(__svgalib_inmisc()|0x01);
}

static void initialize(void)
{
    int i;
    struct sigaction siga;

	DTP((stderr, "initialize\n"));

#ifdef _SVGALIB_LRMI
	if (!__svgalib_LRMI_callbacks)
		vga_set_LRMI_callbacks(&__svgalib_default_LRMI_callbacks);
#endif

	__svgalib_open_devconsole();
	if ((!__svgalib_novccontrol)&&(__svgalib_tty_fd < 0)) {
		exit(1);
    }

    /* Make sure that textmode is restored at exit(). */
	if (my_pid == 0)
		my_pid = getpid();
    atexit(__vga_atexit);

#ifndef DONT_WAIT_VC_ACTIVE
    __svgalib_waitvtactive();
#endif

#ifndef SET_TERMIO
    /* save text mode termio parameters */
    ioctl(0, TCGETS, &__svgalib_text_termio);

    __svgalib_graph_termio = __svgalib_text_termio;

    /* change termio parameters to allow our own I/O processing */
    __svgalib_graph_termio.c_iflag &= ~(BRKINT | PARMRK | INPCK | IUCLC | IXON | IXOFF);
    __svgalib_graph_termio.c_iflag |= (IGNBRK | IGNPAR);

    __svgalib_graph_termio.c_oflag &= ~(ONOCR);

    __svgalib_graph_termio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH);

	if (__svgalib_nosigint)
		__svgalib_graph_termio.c_lflag &= ~ISIG;	/* disable interrupt */
    else
		__svgalib_graph_termio.c_lflag |=  ISIG;	/* enable interrupt */

    __svgalib_graph_termio.c_cc[VMIN] = 1;
    __svgalib_graph_termio.c_cc[VTIME] = 0;
    __svgalib_graph_termio.c_cc[VSUSP] = 0;	/* disable suspend */
#endif

    __svgalib_disable_interrupt();	/* Is reenabled later by set_texttermio */

    __svgalib_getchipset();		/* make sure a chipset has been selected */
    chipset_unlock();

    /* disable text output to console */
    if (!__svgalib_secondary) {
		ioctl(__svgalib_tty_fd, KDSETMODE, KD_GRAPHICS);
    }

    __svgalib_takevtcontrol();	/* HH: Take control over VT */

    __vga_map();

    /* disable video */
    vga_screenoff();

    /* Sanity check: (from painful experience) */
	i = __svgalib_saveregs(text_regs);
	if (i > MAX_REGS) {
		fprintf(stderr,"svgalib: FATAL internal error:\n");
		fprintf(stderr,"Set MAX_REGS at least to %d in src/driver.h and recompile everything.\n", 
				i);
		exit(1);
    }

    /* This appears to fix the Trident 8900 rebooting problem. */
    if (__svgalib_chipset == TVGA8900) {
		text_regs[EXT + 11] = __svgalib_inseq(0x0c);
		text_regs[EXT + 12] = __svgalib_incrtc(0x1f);
    }

    /* save text mode palette */
    savepalette(text_red, text_green, text_blue);

    /* shift to color emulation */
    setcoloremulation();

    /* save font data - first select a 16 color graphics mode */
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->savefont) {
		__svgalib_driverspecs->emul->savefont();
    } else if(!__svgalib_novga) {
#if 1
        __svgalib_driverspecs->setmode(GPLANE16, prv_mode);
        save_text();

        /* Allocate space for textmode font. */
        if (!__svgalib_emulatepage)
        {
            font_buf1_banked = malloc(FONT_SIZE * 2);
            font_buf2_banked = font_buf1_banked + FONT_SIZE;
        }
        if (LINEAR_POINTER)
        {
            font_buf1_linear = malloc(FONT_SIZE * 2);
            font_buf2_linear = font_buf1_linear + FONT_SIZE;
        }

    /* save font data in plane 2 */
        __svgalib_outgra(0x04,0x02);
        if (!__svgalib_emulatepage)
            slowcpy(font_buf1_banked, GM, FONT_SIZE);
        if (LINEAR_POINTER)
            slowcpy(font_buf1_linear, LINEAR_POINTER, FONT_SIZE);

        /* save font data in plane 3 */
        __svgalib_outgra(0x04,0x03);
        if (!__svgalib_emulatepage)
            slowcpy(font_buf2_banked, GM, FONT_SIZE);
        if (LINEAR_POINTER)
            slowcpy(font_buf2_linear, LINEAR_POINTER, FONT_SIZE);
#endif
    }
    initialized = 1;

    /* do our own interrupt handling */
    for (i = 0; i < sizeof(sig2catch); i++) {
	siga.sa_handler = signal_handler;
	siga.sa_flags = 0;
	zero_sa_mask(&(siga.sa_mask));
	sigaction((int) sig2catch[i], &siga, old_signal_handler + i);
    }

    /* vga_unlockvc(); */
}

inline void vga_setpage(int p)
{
    p += vga_page_offset;
    
	if ((p == __svgalib_readpage) && (p == __svgalib_writepage))
		return;

	if (__svgalib_emulatepage) {
		unsigned long offset = __svgalib_linear_mem_base;
   		if (__svgalib_linear_mem_fd == -1) { /* oops */
			
            fprintf(stderr,
                 "svgalib: Warning: cannot change page when emulating banking in NoHelper mode.\n");
            return;
        }
#ifdef __alpha__
		if (offset)
			offset += 0x300000000ULL;
#endif
		mmap(BANKED_POINTER, 65536, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED,
				__svgalib_linear_mem_fd, offset + (p << 16));
		
    } else {
    	(*__svgalib_setpage) (p);
	}
    
    __svgalib_readpage  = p;
    __svgalib_writepage = p;
}


void vga_setreadpage(int p)
{
    p += vga_page_offset;
    if (p == __svgalib_readpage)
	return;
    if (!__svgalib_setrdpage || __svgalib_runinbackground)
    {
        fprintf(stderr,
            "svgalib: Warning: vga_setreadpage is not supported with your config. Program\n"
            "  should check vga_getmodeinfo(mode)->flags before calling vga_setreadpage\n");
        return;
    }
    (*__svgalib_setrdpage) (p);
    __svgalib_readpage = p;
}


void vga_setwritepage(int p)
{
    p += vga_page_offset;
    if (p == __svgalib_writepage)
	return;
    if (!__svgalib_setwrpage || __svgalib_runinbackground)
    {
        fprintf(stderr,
            "svgalib: Warning: vga_setwritepage is not supported with your config. Program\n"
            "  should check vga_getmodeinfo(mode)->flags before calling vga_setwritepage\n");
        return;
    }
    (*__svgalib_setwrpage) (p);
    __svgalib_writepage = p;
}

void vga_safety_fork(void (*shutdown_routine) (void))
{
    pid_t childpid;
    int child_status, oldkbmode;

    if (initialized) {
	fprintf(stderr,"svgalib: warning: vga_safety_fork() called when already initialized\n");
	goto no_fork;
    }
    initialize();

    /*
     * get current keyboard mode:
     *  If this didn't suffice we claim we are on an old system and just don't
     *  need to restore it.
     */
    ioctl(__svgalib_tty_fd, KDGKBMODE, &oldkbmode);

    childpid = fork();
    if (childpid < 0) {
      no_fork:
	fprintf(stderr,"svgalib: warning: can't fork to enhance reliability; proceeding anyway");
	return;
    }
    if (childpid) {
	ioctl(__svgalib_tty_fd, (int) TIOCNOTTY, (char *)0);
	for (;;) {
	    while (waitpid(childpid, &child_status, WUNTRACED) != childpid);

	    if (shutdown_routine)
		shutdown_routine();

	    vga_setmode(TEXT);	/* resets termios as well */
	    ioctl(__svgalib_tty_fd, KDSKBMODE, oldkbmode);

	    if (WIFEXITED(child_status))
		exit(WEXITSTATUS(child_status));

	    if (WCOREDUMP(child_status))
		fprintf(stderr,"svgalib:vga_safety_fork: Core dumped!\n");

	    if (WIFSIGNALED(child_status)) {
		fprintf(stderr,"svgalib:vga_safety_fork: Killed by signal %d, %s.\n",
		       WTERMSIG(child_status),
		       strsignal(WTERMSIG(child_status)));
		exit(1);
	    }
	    if (WIFSTOPPED(child_status)) {
		fprintf(stderr,"svgalib:vga_safety_fork: Stopped by signal %d, %s.\n",
		       WSTOPSIG(child_status),
		       strsignal(WSTOPSIG(child_status)));
		fprintf(stderr,"\aWARNING! Continue stopped svgalib application at own risk. You are better\n"
		     "off killing it NOW!\n");
		continue;
	    }
	}
    }
    /* These need to be done again because the child doesn't inherit them.  */
    __svgalib_get_perm();

    /*
     * Actually the mmap's are inherited anyway (and not all are remade here),
     * but it does not really harm.
     */
    __vga_map();

    /*
     * We might still want to do vc switches.
     */

    __svgalib_takevtcontrol();
}

static void prepareforfontloading(void)
{
    if (__svgalib_chipset == CIRRUS) {
        __svgalib_outseq(0x0f, __svgalib_inseq(0x0f) | 0x40 );
    }
}

static void fontloadingcomplete(void)
{
    if (__svgalib_chipset == CIRRUS) {
        __svgalib_outseq(0x0f, __svgalib_inseq(0x0f) & 0xbf );
    }
}


int vga_setmode(int mode)
{
    int modeflags=mode&0xfffff000;

    DPRINTF("setmode %i from %i\n", mode, CM);

	if(mode==-1)return vga_version;
    
	mode&=0xfff;
    
	if (!initialized)
		initialize();

	if (!vga_hasmode(mode))
		return -1;
       
/*    if (!flip)
   vga_lockvc(); */
	if(CM)vga_waitretrace();
    __svgalib_disable_interrupt();

    prv_mode = CM;
    CM = mode;

    /* disable video */
    vga_screenoff();

    if (!__svgalib_secondary && (prv_mode==TEXT) && (mode!=TEXT)) {
		ioctl(__svgalib_tty_fd, KDSETMODE, KD_GRAPHICS);
    }

    if(!__svgalib_novga) {
	    /* Should be more robust (eg. grabbed X modes) */
		if (__svgalib_getchipset() == ET4000
		     && prv_mode != G640x480x256
	    	 && SVGAMODE(prv_mode))
		    chipset_setmode(G640x480x256, prv_mode);

		/* This is a hack to get around the fact that some C&T chips
		 * are programmed to ignore syncronous resets. So if we are
		 * a C&T wait for retrace start
		 */
		if (__svgalib_getchipset() == CHIPS) {
			while ((__svgalib_inis1() & 0x08) == 0x08 );/* wait VSync off */
	 		while ((__svgalib_inis1() & 0x08) == 0 );   /* wait VSync on  */
 			__svgalib_outseq(0x07,0x00);  /* reset hsync - just in case...  */
		}
	}

    if (mode == TEXT) {
		/* Returning to textmode. */

		if (SVGAMODE(prv_mode))
			vga_setpage(0);
		
		/* The extended registers are restored either by the */
		/* chipset setregs function, or the chipset setmode function. */
		
		/* restore font data - first select a 16 color graphics mode */
		/* Note: this should restore the old extended registers if */
		/* setregs is not defined for the chipset. */
		if (__svgalib_novga) __svgalib_driverspecs->setmode(TEXT, prv_mode);
		if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->restorefont) {
 			__svgalib_driverspecs->emul->restorefont(); 
	 		chipset_setregs(text_regs, mode);
		} else if(!__svgalib_novga) {
#if 1
	  		__svgalib_driverspecs->setmode(GPLANE16, prv_mode);
	  		if (CHIPSET != EGA)
	  			/* restore old extended regs */
	  			chipset_setregs(text_regs, mode);
#if 1
	  		/* disable Set/Reset Register */
  			__svgalib_outgra(0x01,0x00);
			
	  		prepareforfontloading();
			
	  		restore_text();
			
	  		/* restore font data in plane 2 - necessary for all VGA's */
	  		__svgalib_outseq(0x02,0x04);
			
          if (__svgalib_emulatepage)
	      slowcpy(LINEAR_POINTER, font_buf1_linear, FONT_SIZE);
          else
	      slowcpy(GM, font_buf1_banked, FONT_SIZE);
			
	  		/* restore font data in plane 3 - necessary for Trident VGA's */
	  		__svgalib_outseq(0x02,0x08);
          if (__svgalib_emulatepage)
	      slowcpy(LINEAR_POINTER, font_buf2_linear, FONT_SIZE);
          else
	      slowcpy(GM, font_buf2_banked, FONT_SIZE);
	  		fontloadingcomplete();
#endif
	  		/* change register adresses if monochrome text mode */
	  		/* EGA is assumed to use color emulation. */
	  		if (!color_text) {
	  			__svgalib_CRT_I = CRT_IM;
	  			__svgalib_CRT_D = CRT_DM;
		  		__svgalib_IS1_R = IS1_RM;
  				__svgalib_vgacolormode=0;
  				__svgalib_outmisc(__svgalib_inmisc() & 0xfe);
	  		}
#else
	  		__svgalib_driverspecs->setmode(TEXT, prv_mode);
	  		if (CHIPSET != EGA)
	  			/* restore old extended regs */
	  			chipset_setregs(text_regs, mode);
  			__svgalib_outseq(0,1);
  			__svgalib_outseq(2,4);
  			__svgalib_outseq(4,7);
  			__svgalib_outseq(0,3);
  			__svgalib_outgra(4,2);
  			__svgalib_outgra(5,0);
  			__svgalib_outgra(6,0);
          if (__svgalib_emulatepage)
	      slowcpy(LINEAR_POINTER, font_buf1_linear, FONT_SIZE);
          else
	      slowcpy(GM, font_buf1_banked, FONT_SIZE);
  			__svgalib_outseq(2,2);
	  		slowcpy(GM, text_buf2, FONT_SIZE);
  			__svgalib_outseq(2,1);
          if (__svgalib_emulatepage)
	      slowcpy(LINEAR_POINTER, font_buf1_linear, FONT_SIZE);
          else
	      slowcpy(GM, font_buf1_banked, FONT_SIZE);
  			__svgalib_outseq(0,1);
  			__svgalib_outseq(2,3);
  			__svgalib_outseq(4,3);
  			__svgalib_outseq(0,3);
  			__svgalib_outgra(4,0);
  			__svgalib_outgra(5,16);
  			__svgalib_outgra(6,14);
  			
#endif
		} else chipset_setregs(text_regs, mode);
		
		/* restore text mode VGA registers */
		__svgalib_setregs(text_regs);
		
		/* restore saved palette */
		restorepalette(text_red, text_green, text_blue);
		
		/* Set VMEM to some minimum value .. probably pointless.. */
		{
			vga_claimvideomemory(12);
		}
		
		/*      if (!flip) */
		/* enable text output - restores the screen contents */
		
        if (!__svgalib_secondary) {
			ioctl(__svgalib_tty_fd, KDSETMODE, KD_TEXT);
		}
		
		/* now wait for signal to stabilize, but don't do it on C&T chips. */
		/* This is needed to restore correct text mode stretching.         */
#ifdef MODESWITCH_DELAY
		if (__svgalib_chipset != CHIPS)
			usleep(MODESWITCHDELAY);
#endif
		
		/* enable video */
		vga_screenon();
		
		if (!flip)
			/* restore text mode termio */
			__svgalib_set_texttermio();
	} else { /* Setting a graphics mode. */
		vga_modeinfo *modeinfo;
#if 0
        if(prv_mode == TEXT) {
              __svgalib_outseq(0,1);
              __svgalib_outseq(2,4);
              __svgalib_outseq(4,7);
              __svgalib_outseq(0,3);
              __svgalib_outgra(4,2);
              __svgalib_outgra(5,0);
              __svgalib_outgra(6,0);
              if (__svgalib_emulatepage)
		      slowcpy(font_buf1_linear, LINEAR_POINTER, FONT_SIZE);
              else
		      slowcpy(font_buf1_banked, GM, FONT_SIZE);
              __svgalib_outgra(4,1);
		      slowcpy(text_buf2, GM, FONT_SIZE);
              __svgalib_outseq(4,0);
              if (__svgalib_emulatepage)
		      slowcpy(font_buf1_linear, LINEAR_POINTER, FONT_SIZE);
              else
		      slowcpy(font_buf1_banked, GM, FONT_SIZE);
              __svgalib_outseq(0,1);
              __svgalib_outseq(2,3);
              __svgalib_outseq(4,3);
              __svgalib_outseq(0,3);
              __svgalib_outgra(4,0);
              __svgalib_outgra(5,16);
              __svgalib_outgra(6,14);
        }
#endif
		/* disable text output */
#if 0
        if (!__svgalib_secondary) {
			ioctl(__svgalib_tty_fd, KDSETMODE, KD_GRAPHICS);
        }
#endif
		if (SVGAMODE(prv_mode)) {
			/* The current mode is an SVGA mode, and we now want to */
			/* set a standard VGA mode. Make sure the extended regs */
			/* are restored. */
			/* Also used when setting another SVGA mode to hopefully */
			/* eliminate lock-ups. */
			vga_setpage(0);
			chipset_setregs(text_regs, mode);
			/* restore old extended regs */
		}
		/* shift to color emulation */
		setcoloremulation();
		
		CI.xdim = infotable[mode].xdim;
		CI.ydim = infotable[mode].ydim;
		CI.colors = infotable[mode].colors;
		CI.xbytes = infotable[mode].xbytes;
		CI.bytesperpixel = infotable[mode].bytesperpixel;
		
		if (chipset_setmode(mode, prv_mode)) {
			/* Darn setmode failed, this really shouldn't happen,
			   but alas it does (atleast for fbdev) */
			/* Force CM to an svgalib mode so that vga_setpage(0) gets
			   called by vga_setmode(TEXT) */
			CM = G640x480x256;
			vga_setmode(TEXT);
			fprintf(stderr, "svgalib: error: vga_setmode(chipset_setmode) failed,"
				   " textmode has been restored\n");
			return -1;
		}
		
		modeinfo = vga_getmodeinfo(mode);
		MODEX = ((MODEFLAGS = modeinfo->flags) & IS_MODEX);
		
		/* Set default claimed memory (moved here from initialize - Michael.) */
		if (mode == G320x200x256)
			VMEM = 65536;
		else if (STDVGAMODE(mode))
			VMEM = 256 * 1024;	/* VGA cards have 256KB ram. */
		else {
			VMEM = modeinfo->linewidth * modeinfo->height;
			CI.xbytes = modeinfo->linewidth;
		}
		
		if (!flip) {
			/* set default palette */
			if (CI.colors <= 256)
				restorepalette(default_red, default_green, default_blue);
			
			/* clear screen (sets current color to 15) */
			__svgalib_readpage = __svgalib_writepage = -1;
			if(!(modeflags&0x8000))vga_clear();
			
			if (SVGAMODE(CM))
				vga_setpage(0);
		}
		__svgalib_readpage = __svgalib_writepage = -1;
		currentlogicalwidth = CI.xbytes;
		currentdisplaystart = 0;
		
#ifdef MODESWITCH_DELAY
		usleep(MODESWITCHDELAY);	/* wait for signal to stabilize */
#endif
		
		/* enable video */
		if (!flip)
			vga_screenon();
		
		if (mouse_support && mouse_open) {
			/* vga_lockvc(); */
			mouse_setxrange(0, CI.xdim - 1);
			mouse_setyrange(0, CI.ydim - 1);
			mouse_setwrap(MOUSE_NOWRAP);
			mouse_mode = mode;
		} 
		if (__svgalib_emulatepage && (modeinfo->flags & CAPABLE_LINEAR) &&
				!(modeinfo->flags & LINEAR_USE)) {
			__svgalib_driverspecs->linear(LINEAR_ENABLE, 0);
		}
		
		if (!flip) /* set graphics mode termio */
			__svgalib_set_graphtermio();
		else if (__svgalib_kbd_fd < 0)
			__svgalib_enable_interrupt();
	}
	
	/*    if (!flip)
	 *       vga_unlockvc(); */
	return 0;
}

void vga_gettextfont(void *font)
{
    unsigned int getsize;
    unsigned char *font_buf1;
    
    if (__svgalib_emulatepage)
        font_buf1 = font_buf1_linear;
    else
        font_buf1 = font_buf1_banked;

    /* robert@debian.org, May, 26th 2002: check for valid font_buf buffer */
    if (!font_buf1) {
	 syslog(LOG_DEBUG, "svgalib: uninitialized variable: font_buf1");
	 return;
    }	 
	    
    getsize = fontbufsize;
    if (getsize > FONT_SIZE)
	getsize = FONT_SIZE;
    memcpy(font, font_buf1, getsize);
    if (fontbufsize > getsize)
	memset(((char *)font) + getsize, 0, (size_t)(fontbufsize - getsize));
}

void vga_puttextfont(void *font)
{
    unsigned int putsize;
    unsigned char *font_buf1, *font_buf2;
    
    if (__svgalib_emulatepage) {
        font_buf1 = font_buf1_linear;
        font_buf2 = font_buf2_linear;
    } else {
        font_buf1 = font_buf1_banked;
        font_buf2 = font_buf2_banked;
    }
    
    /* robert@debian.org, May, 26th 2002: check for valid font_buf buffer */
    if (!font_buf1 || !font_buf2) {
	 syslog(LOG_DEBUG, "svgalib: uninitialized variable: font_buf1 or font_buf2");
	 return;
    }	 

    putsize = fontbufsize;
    if (putsize > FONT_SIZE)
	putsize = FONT_SIZE;
    memcpy(font_buf1, font, putsize);
    memcpy(font_buf2, font, putsize);
    if (putsize < FONT_SIZE) {
	memset(font_buf1 + putsize, 0, (size_t)(FONT_SIZE - putsize));
	memset(font_buf2 + putsize, 0, (size_t)(FONT_SIZE - putsize));
    }

}

void vga_gettextmoderegs(void *regs)
{
    memcpy(regs, text_regs, MAX_REGS);
}

void vga_settextmoderegs(void *regs)
{
    memcpy(text_regs, regs, MAX_REGS);
}

int vga_getcurrentmode(void)
{
    return CM;
}

int vga_getcurrentchipset(void)
{
    return __svgalib_getchipset();
}

void vga_disabledriverreport(void)
{
    DREP = 0;
}

vga_modeinfo *vga_getmodeinfo(int mode)
{
    static vga_modeinfo modeinfo;
    int is_modeX = (CM == mode) && MODEX;

    DTP((stderr,"getmodeinfo %i\n",mode));

    modeinfo.linewidth = infotable[mode].xbytes;
    __svgalib_getchipset();
	if (mode > vga_lastmodenumber())
		return NULL;
    modeinfo.width = infotable[mode].xdim;
    modeinfo.height = infotable[mode].ydim;
    modeinfo.bytesperpixel = infotable[mode].bytesperpixel;
    modeinfo.colors = infotable[mode].colors;
    if (is_modeX) {
		modeinfo.linewidth = modeinfo.width / 4;
		modeinfo.bytesperpixel = 0;
    }
    if (mode == TEXT) {
		modeinfo.flags = HAVE_EXT_SET;
		return &modeinfo;
    }
    modeinfo.flags = 0;
    if ((STDVGAMODE(mode) && mode != G320x200x256) || is_modeX)
		__svgalib_vga_driverspecs.getmodeinfo(mode, &modeinfo);
    else
		/* Get chipset specific info for SVGA modes and */
		/* 320x200x256 (chipsets may support more pages) */
		chipset_getmodeinfo(mode, &modeinfo);

    if (modeinfo.colors == 256 && modeinfo.bytesperpixel == 0)
		modeinfo.flags |= IS_MODEX;
    if (mode > __GLASTMODE)
		modeinfo.flags |= IS_DYNAMICMODE;
    if (__svgalib_emulatepage || __svgalib_runinbackground)
        modeinfo.flags &= ~HAVE_RWPAGE;

    /* Maskout CAPABLE_LINEAR if requested by config file */
    modeinfo.flags &= modeinfo_mask;

    /* Many cards have problems with linear 320x200x256 mode */
    if(mode==G320x200x256)modeinfo.flags &= (~CAPABLE_LINEAR) & (~LINEAR_USE);

    /* Signal if linear support has been enabled */
    if (modeinfo.flags & CAPABLE_LINEAR) {
		modeinfo.flags |= __svgalib_modeinfo_linearset;
    }

    return &modeinfo;
}

int vga_hasmode(int mode)
{
    vga_modeinfo *modeinfo;
    DTP((stderr,"hasmode %i\n",mode));

    __svgalib_getchipset();		/* Make sure the chipset is known. */
    if (mode == TEXT)
		return 1;

    if (mode < 0 || mode > lastmodenumber)
		return 0;
	
	if(infotable[mode].xdim<minx || infotable[mode].xdim>maxx || 
			infotable[mode].ydim<miny || infotable[mode].ydim>maxy)
		return 0;
	
	if(__svgalib_emulatepage && STDVGAMODE(mode) && (mode!=G320x200x256))
        return 0;

    if (!chipset_modeavailable(mode))
        return 0;

    modeinfo = vga_getmodeinfo(mode);
    if (__svgalib_emulatepage && (mode!=G320x200x256) &&
        !(modeinfo->flags & (CAPABLE_LINEAR)))
        return 0;

    return 1;
}


int vga_lastmodenumber(void)
{
    __svgalib_getchipset();
    return lastmodenumber;
}


int __svgalib_addmode(int xdim, int ydim, int cols, int xbytes, int bytespp)
{
    int i;

    for (i = 0; i <= lastmodenumber; ++i)
	if (infotable[i].xdim == xdim &&
	    infotable[i].ydim == ydim &&
	    infotable[i].colors == cols &&
	    infotable[i].bytesperpixel == bytespp &&
	    infotable[i].xbytes == xbytes)
	    return i;
    if (lastmodenumber >= MAX_MODES - 1)
	return -1;		/* no more space available */

    if(xdim<minx || xdim>maxx || ydim<miny || ydim>maxy)
	return -1;
	
    ++lastmodenumber;
    infotable[lastmodenumber].xdim = xdim;
    infotable[lastmodenumber].ydim = ydim;
    infotable[lastmodenumber].colors = cols;
    infotable[lastmodenumber].xbytes = xbytes;
    infotable[lastmodenumber].bytesperpixel = bytespp;

    return lastmodenumber;
}

int vga_setcolor(int color)
{
    switch (CI.colors) {
    case 2:
	if (color != 0)
	    color = 15;
    case 16:			/* update set/reset register */
   	__svgalib_outgra(0x00,color&0x0f);
	break;
    default:
	COL = color;
	break;
    }
    return 0;
}


int vga_screenoff(void)
{
    int tmp = 0;

    SCREENON = 0;

    if(__svgalib_novga) return 0; 

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenoff) {
	tmp = __svgalib_driverspecs->emul->screenoff();
    } else {
	/* turn off screen for faster VGA memory acces */
	if ((CHIPSET != EGA) && !__svgalib_novga) {
            __svgalib_outseq(0x01,__svgalib_inseq(0x01) | 0x20);
	}
	/* Disable video output */
#ifdef DISABLE_VIDEO_OUTPUT
	__svgalib_attscreen(0);
#endif
    }

    return tmp;
}


int vga_screenon(void)
{
    int tmp = 0;

    SCREENON = 1;
    if(__svgalib_novga) return 0; 
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->screenon) {
	tmp = __svgalib_driverspecs->emul->screenon();
    } else {
	/* turn screen back on */
	if ((CHIPSET != EGA) && !__svgalib_novga) {
            __svgalib_outseq(0x01,__svgalib_inseq(0x01) & 0xdf);
	}
/* #ifdef DISABLE_VIDEO_OUTPUT */
	/* enable video output */
	__svgalib_attscreen(0x20);
/* #endif */
    }

    return 0;
}


int vga_getxdim(void)
{
    return CI.xdim;
}


int vga_getydim(void)
{
    return CI.ydim;
}


int vga_getcolors(void)
{
    return CI.colors;
}

int vga_white(void)
{
    switch (CI.colors) {
    case 2:
    case 16:
    case 256:
	return 15;
    case 1 << 15:
	return 32767;
    case 1 << 16:
	return 65535;
    case 1 << 24:
	return (1 << 24) - 1;
    }
    return CI.colors - 1;
}

int vga_claimvideomemory(int m)
{
    vga_modeinfo *modeinfo;
    int cardmemory;

    modeinfo = vga_getmodeinfo(CM);
    if (m < VMEM)
	return 0;
    if (modeinfo->colors == 16)
	cardmemory = modeinfo->maxpixels / 2;
    else
	cardmemory = (modeinfo->maxpixels * modeinfo->bytesperpixel
		      + 2) & 0xffff0000;
    /* maxpixels * bytesperpixel can be 2 less than video memory in */
    /* 3 byte-per-pixel modes; assume memory is multiple of 64K */
    if (m > cardmemory)
	return -1;
    VMEM = m;
    return 0;
}

int vga_setmodeX(void)
{
    switch (CM) {
    case TEXT:
/*    case G320x200x256: */
    case G320x240x256:
    case G320x400x256:
    case G360x480x256:
    case G400x300x256X:
	return 0;
    }
    if (CI.colors == 256 && VMEM < 256 * 1024) {
        __svgalib_outseq(0x04, (__svgalib_inseq(0x04) & 0xf7) | 0x04);
        __svgalib_outcrtc(0x14, __svgalib_incrtc(0x14) & 0xbf );
        __svgalib_outcrtc(0x17, __svgalib_incrtc(0x17) | 0x40);
	CI.xbytes = CI.xdim / 4;
	vga_setpage(0);
	MODEX = 1;
	return 1;
    }
    return 0;
}

static int saved_readpage  = -1;
static int saved_writepage = -1;
static int saved_logicalwidth=0;
static int saved_displaystart=0;
static int saved_modeX=0;
static int saved_linear=0;
static unsigned char saved_crtc[CRT_C];
static int saved_lfb_fd=-1;
static unsigned long saved_lfb_base=0;
static unsigned long saved_lfb_size=0;
static int saved_emulatepage=0;

static void alloc_graph_buf(int size) {
    if(__svgalib_runinbackground) {
        char filename[256];
        snprintf(filename, 255, "/tmp/.svga.bgr.%i.%li",getpid(),random());
        background_fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, S_IREAD|S_IWRITE);
        unlink(filename);
        ftruncate(background_fd, size);
        graph_buf = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                         background_fd, 0);
    } else {
        if ((graph_buf = malloc(size)) == NULL) {
	    fprintf(stderr,"Cannot allocate memory for VGA state\n");
	    vga_setmode(TEXT);
	    exit(1);
        }
    }
    graph_buf_size= size;
}

static void free_graph_buf(void) {
    if(__svgalib_runinbackground) {
        munmap(graph_buf, graph_buf_size);
        close(background_fd);
    } else {
        free(graph_buf);
    }
    graph_buf=NULL;
    graph_buf_size=0;
}

static void savestate(void)
{
    int i;

    vga_screenoff();

    savepalette(graph_red, graph_green, graph_blue);

    saved_readpage  = __svgalib_readpage;
    saved_writepage = __svgalib_writepage;
    saved_logicalwidth = currentlogicalwidth;
    saved_displaystart = currentdisplaystart;
    saved_modeX = MODEX;
	saved_linear = __svgalib_modeinfo_linearset&IS_LINEAR;

    if(STDVGAMODE(CM)) {
        vga_getcrtcregs(saved_crtc); /* save for tweaked modes */
    }

    if (CM == G320x200x256 && VMEM <= 65536) {
		/* 320x200x256 is a special case; only 64K is addressable */
		/* (unless more has been claimed, in which case we assume */
		/* SVGA bank-switching) */
        alloc_graph_buf(GRAPH_SIZE);
		memcpy(graph_buf, GM, GRAPH_SIZE);
    } else if (MODEX || CM == G800x600x16 || (STDVGAMODE(CM) && CM != G320x200x256)) {
		/* for planar VGA modes, save the full 256K */
		__svgalib_vga_driverspecs.setmode(GPLANE16, prv_mode);
		alloc_graph_buf(4*GRAPH_SIZE);
		for (i = 0; i < 4; i++) {
	    	/* save plane i */
            __svgalib_outgra(4,i);
	    	memcpy(graph_buf + i * GRAPH_SIZE, GM, GRAPH_SIZE);
		}
    } else if (CI.colors == 16) {
		int page, size, sbytes;
		unsigned char *sp;

		size = VMEM;
		alloc_graph_buf(4*size);
		sp = graph_buf;
		for (page = 0; size > 0; ++page) {
	    	vga_setpage(page);
	    	sbytes = (size > GRAPH_SIZE) ? GRAPH_SIZE : size;
	    	for (i = 0; i < 4; i++) {
				/* save plane i */
                __svgalib_outgra(4,i);
				memcpy(sp, GM, sbytes);
				sp += sbytes;
		    }
	    	size -= sbytes;
		}
    } else {			/* SVGA, and SVGA 320x200x256 if videomemoryused > 65536 */
		int size;
		int page;

		size = VMEM;
		alloc_graph_buf(size);
		if( CAN_USE_LINEAR ) {
			memcpy(graph_buf, LINEAR_POINTER, size);
		} else {
			page = 0;
			while (size >= 65536) {
				vga_setpage(page);
				memcpy(graph_buf + page * 65536, GM, 65536);
				page++;
				size -= 65536;
			}
			if (size > 0) {
				vga_setpage(page);
				memcpy(graph_buf + page * 65536, GM, size);
			}
		}
	}
}

static void restorestate(void)
{
    int i;
    vga_modeinfo *modeinfo;

    vga_screenoff();

    if (saved_modeX)
		vga_setmodeX();

    if(STDVGAMODE(CM)) {
        vga_setcrtcregs(saved_crtc); /* restore tweaked modes */
    }

    restorepalette(graph_red, graph_green, graph_blue);

	if(saved_linear)
		vga_setlinearaddressing();

	modeinfo = vga_getmodeinfo(CM);
	if (__svgalib_emulatepage && (modeinfo->flags & CAPABLE_LINEAR) &&
	    !(modeinfo->flags & LINEAR_USE)) {
		__svgalib_driverspecs->linear(LINEAR_ENABLE, 0);
	}
    
    if (CM == G320x200x256 && VMEM <= 65536) {
		memcpy(GM, graph_buf, 65536);
    } else if (MODEX || CM == G800x600x16 || (STDVGAMODE(CM) && CM != G320x200x256)) {
		int setresetreg, planereg;
		/* disable Set/Reset Register */
		setresetreg = __svgalib_ingra(0x01);
        __svgalib_outgra(0x01,0x00);
		planereg = __svgalib_ingra(0x02);

		for (i = 0; i < 4; i++) {
	    	/* restore plane i */
           	__svgalib_outseq(0x02,1<<i);
		    memcpy(GM, graph_buf + i * GRAPH_SIZE, GRAPH_SIZE);
		}
        __svgalib_outgra(0x01,setresetreg);
        __svgalib_outseq(0x02,planereg);
    } else if (CI.colors == 16) {
		int page, size, rbytes;
		unsigned char *rp;
		int setresetreg, planereg;

		/* disable Set/Reset Register */
		if (CHIPSET == EGA)
			setresetreg = 0;
		else
			setresetreg = __svgalib_ingra(0x01);

		__svgalib_outgra(0x01,0x00);
		
		if (CHIPSET == EGA)
			planereg = 0;
		else
			planereg = __svgalib_ingra(0x02);

		size = VMEM;
		rp = graph_buf;
		for (page = 0; size > 0; ++page) {
			vga_setpage(page);
			rbytes = (size > GRAPH_SIZE) ? GRAPH_SIZE : size;
			for (i = 0; i < 4; i++) {
				/* save plane i */
				__svgalib_outseq(0x02,1<<i);
				memcpy(GM, rp, rbytes);
				rp += rbytes;
			}
			size -= rbytes;
		}

		__svgalib_outgra(0x01,setresetreg);
		__svgalib_outseq(0x02,planereg);
	} else {
		/* vga_modeinfo *modeinfo; */
		int size;
		int page;
		size = VMEM;

		DPRINTF("Restoring %dK of video memory.\n", (size + 2) / 1024);

		if( CAN_USE_LINEAR ) {
			memcpy(LINEAR_POINTER, graph_buf, size);
		} else {
			page = 0;
			while (size >= 65536) {
				vga_setpage(page);
				memcpy(GM, graph_buf + page * 65536, 65536);
				size -= 65536;
				page++;
			}
			if (size > 0) {
				vga_setpage(page);
				memcpy(GM, graph_buf + page * 65536, size);
			}
		}
	}

	if (saved_logicalwidth != CI.xbytes)
		vga_setlogicalwidth(saved_logicalwidth);
    if ((saved_readpage == saved_writepage) && (saved_readpage != -1))
		vga_setpage(saved_readpage);
    else
    {
        if (saved_readpage != -1)
            vga_setreadpage(saved_readpage);
        if (saved_writepage != -1)
            vga_setwritepage(saved_writepage);
    }
    if (saved_displaystart != 0) {
		if (CHIPSET != VGA && CHIPSET != EGA) {
    		currentdisplaystart = saved_displaystart;
			__svgalib_driverspecs->setdisplaystart(saved_displaystart);
		}
	}

    vga_screenon();

    free_graph_buf();
}


int vga_getch(void)
{
    int fd;
    char c;
    
    if (CM == TEXT)
	return -1;
    
    fd = __svgalib_novccontrol ? fileno(stdin) : __svgalib_tty_fd;

    while ((read(fd, &c, 1) < 0) && (errno == EINTR));

    return c;
}

/* I have kept the slightly funny 'flip' terminology. */

void __svgalib_flipaway(void)
{
    /* Leaving console. */
#ifdef MODESWITCH_DELAY
	usleep(10);
#endif
    	flip_mode = CM;

	__joystick_flip_vc(0);

	if (CM != TEXT) {
            /* wait for any blitter operation to finish */
            idle_accel();
            /* Save state and go to textmode. */
#ifdef SAVEREGS
            chipset_saveregs(graph_regs);
            __svgalib_saveregs(graph_regs);
#endif
            savestate();
            flip = 1;
            if(!__svgalib_secondary)vga_setmode(TEXT);
            flip = 0;

            if (__svgalib_runinbackground) {
                /* Save current lfb settings, make lfb-settings
                   point to background_fd, force emulatepage */
                saved_lfb_fd      = __svgalib_linear_mem_fd;
                saved_lfb_base    = __svgalib_linear_mem_base;
                saved_lfb_size    = __svgalib_linear_mem_size;
                saved_emulatepage = __svgalib_emulatepage;
                __svgalib_linear_mem_fd   = background_fd;
                __svgalib_linear_mem_base = 0;
                __svgalib_linear_mem_size = graph_buf_size;
                __svgalib_emulatepage     = 1;
                /* savestate may have changed the page, so restore
                   the saved page pointer */
                __svgalib_readpage = __svgalib_writepage = saved_readpage;
                /* map background_fd over BANKED_POINTER and LINEAR_POINTER */
                map_banked(MAP_FIXED);
                map_linear(MAP_FIXED);
            }
        }
}

static void __svgalib_flipback(void)
{
    /* Entering console. */
    /* Hmmm... and how about unlocking anything someone else locked? */
    __joystick_flip_vc(1);

    chipset_unlock();
    if (flip_mode != TEXT) {
        if (__svgalib_runinbackground) {
            /* make lfb-settings point to real lfb-fd */
            __svgalib_linear_mem_fd   = saved_lfb_fd;
            __svgalib_linear_mem_base = saved_lfb_base;
            __svgalib_linear_mem_size = saved_lfb_size;
            __svgalib_emulatepage     = saved_emulatepage;
            /* we've been running in the background so the current
               readpage is the correct one to restore not the saved one. */
            saved_readpage = saved_writepage = __svgalib_readpage;
            /* map the real memory over BANKED and LINEAR_POINTER */
            map_banked(MAP_FIXED);
            map_linear(MAP_FIXED);
        }
	/* Restore graphics mode and state. */
        if(!__svgalib_secondary) {
	  		flip = 1;
	  		vga_setmode(flip_mode);
	  		flip = 0;
#ifdef SAVEREGS
  			chipset_setregs(graph_regs,flip_mode);
  			__svgalib_setregs(graph_regs);
#endif
	  		restorestate();
  			if(__svgalib_cursor_status>=0) __svgalib_cursor_restore();
		}  
	}
}

int vga_flip(void)
{
    if (CM != TEXT) {		/* save state and go to textmode */
		savestate();
		flip_mode = CM;
		flip = 1;
		vga_setmode(TEXT);
		flip = 0;
    } else {			/* restore graphics mode and state */
		flip = 1;
		vga_setmode(flip_mode);
		flip = 0;
		restorestate();
    }
    return 0;
}

int vga_setflipchar(int c)
/* This function is obsolete. Retained for VGAlib compatibility. */
{
    __svgalib_flipchar = c;

    return 0;
}

void vga_setlogicalwidth(int w)
{
    __svgalib_driverspecs->setlogicalwidth(w);
    currentlogicalwidth = w;
}

void vga_setdisplaystart(int a)
{
    currentdisplaystart = a;
    if (CHIPSET != VGA && CHIPSET != EGA)
		if (MODEX || CI.colors == 16) {
	    	/* We are currently using a Mode X-like mode on a */
			/* SVGA card, use the standard VGA function */
			/* that works properly for Mode X. */
			/* Same goes for 16 color modes. */
			__svgalib_vga_driverspecs.setdisplaystart(a);
			return;
		}
    /* Call the regular display start function for the chipset */
	if( (MODEFLAGS & IOCTL_SETDISPLAY) && !__svgalib_nohelper)
		ioctl(__svgalib_mem_fd, SVGAHELPER_SETDISPLAYSTART, a);	
	else
		__svgalib_driverspecs->setdisplaystart(a);
}

void vga_bitblt(int srcaddr, int destaddr, int w, int h, int pitch)
{
    __svgalib_driverspecs->bitblt(srcaddr, destaddr, w, h, pitch);
}

void vga_imageblt(void *srcaddr, int destaddr, int w, int h, int pitch)
{
    __svgalib_driverspecs->imageblt(srcaddr, destaddr, w, h, pitch);
}

void vga_fillblt(int destaddr, int w, int h, int pitch, int c)
{
    __svgalib_driverspecs->fillblt(destaddr, w, h, pitch, c);
}

void vga_hlinelistblt(int ymin, int n, int *xmin, int *xmax, int pitch,
		      int c)
{
    __svgalib_driverspecs->hlinelistblt(ymin, n, xmin, xmax, pitch, c);
}

void vga_blitwait(void)
{
    __svgalib_driverspecs->bltwait();
}

int vga_ext_set(unsigned what,...)
{
    va_list params;
    register int retval = 0;

    switch(what) {
	case VGA_EXT_AVAILABLE:
	    /* Does this use of the arglist corrupt non-AVAIL_ACCEL ext_set? */
	    va_start(params, what);
	    switch (va_arg(params, int)) {
		case VGA_AVAIL_ACCEL:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->operations;
		break;
	    case VGA_AVAIL_ROP:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->ropOperations;
		break;
	    case VGA_AVAIL_TRANSPARENCY:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->transparencyOperations;
		break;
	    case VGA_AVAIL_ROPMODES:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->ropModes;
		break;
	    case VGA_AVAIL_TRANSMODES:
		if (__svgalib_driverspecs->accelspecs != NULL)
		    retval = __svgalib_driverspecs->accelspecs->transparencyModes;
		break;
	    case VGA_AVAIL_SET:
		retval = (1 << VGA_EXT_PAGE_OFFSET) |
			 (1 << VGA_EXT_FONT_SIZE);	/* These are handled by us */
		break;
	    }
	    va_end(params);
	    break;
	case VGA_EXT_PAGE_OFFSET:
	    /* Does this use of the arglist corrupt it? */
	    va_start(params, what);
	    retval = vga_page_offset;
	    vga_page_offset = va_arg(params, int);
	    va_end(params);
	    return retval;
	case VGA_EXT_FONT_SIZE:
	    va_start(params, what);
	    what = va_arg(params, unsigned int);
	    va_end(params);
	    if (!what)
		return FONT_SIZE;
	    retval = fontbufsize;
	    fontbufsize = what;
	    return retval;
    }
    if ((CM != TEXT) && (MODEFLAGS & HAVE_EXT_SET)) {
	va_start(params, what);
	retval |= __svgalib_driverspecs->ext_set(what, params);
	va_end(params);
    }
    return retval;
}

/* Parse a string for options.. str is \0-terminated source,
   commands is an array of char ptrs (last one is NULL) containing commands
   to parse for. (if first char is ! case sensitive),
   func is called with ind the index of the detected command.
   func has to return the ptr to the next unhandled __svgalib_token returned by token(&nptr).
   Use __svgalib_token(&nptr) to get the next token from the file..
   mode is 1 when reading from conffile and 0 when parsing the env-vars. This is to
   allow disabling of dangerous (hardware damaging) options when reading the ENV-Vars
   of Joe user.
   Note: We use strtok, that is str is destroyed! */

char *__svgalib_token(char **ptr) 
{
    char *p=*ptr;

    if (!p)
        return NULL;
    
    while(*p==' ')p++;
    
    if(*p != '\0' ) {
        char *t;
        t=p;
        while((*t != '\0') && (*t != ' '))t++;
        if(*t==' ') {
            *t='\0';
            t++;
        }
        *ptr=t;
        return p;
    } else {
        *ptr=NULL;
        return NULL; 
    }
}

static void parse_string(char *str, char **commands, char *(*func) (int ind, int mode, char **nptr), int mode)
{
    int index;
    char *ptr, **curr, **nptr, *tmp;

    /*Pass one, delete comments,ensure only whitespace is ' ' */
    for (ptr = str; *ptr; ptr++) {
		if (*ptr == '#') {
			while (*ptr && (*ptr != '\n')) {
				*ptr++ = ' ';
			}
			if (*ptr)
				*ptr = ' ';
		} else if (isspace(*ptr)) {
			*ptr = ' ';
		}
    }
    /*Pass two, parse commands */
    nptr=&tmp;
    tmp=str;
    ptr = __svgalib_token(nptr);
    while (ptr) {
#ifdef DEBUG_CONF
		fprintf(stderr,"Parsing: %s\n", ptr);
#endif
		for (curr = commands, index = 0; *curr; curr++, index++) {
#ifdef DEBUG_CONF
			fprintf(stderr,"Checking: %s\n", *curr);
#endif
			if (**curr == '!') {
				if (!strcmp(*curr + 1, ptr)) {
					ptr = (*func) (index, mode, nptr);
					break;
				}
			} else {
			if (!strcasecmp(*curr, ptr)) {
				ptr = (*func) (index, mode, nptr);
				break;
			}
	    }
	}
	if (!*curr)		/*unknown command */
	    ptr = __svgalib_token(nptr);	/* skip silently til' next command */
    }
}

static int allowoverride = 0;	/* Allow dangerous options in ENV-Var or in */
				/* the $HOME/.svgalibrc */

static void process_config_file(FILE *file, int mode, char **commands,
		 			char *(*func)(int ind, int mode, char **nptr)) {
    struct stat st;
    char *buf, *ptr;
    int i;
    
    fstat(fileno(file), &st);	/* Some error analysis may be fine here.. */
    if ( (buf = alloca(st.st_size + 1)) == 0) {	/* + a final \0 */
        fprintf(stderr,"svgalib: out of mem while parsing config file !\n");
        return;
    }
    fread(buf, 1, st.st_size, file);
    for (i = 0, ptr = buf; i < st.st_size; i++, ptr++) {
        if (!*ptr)
            *ptr = ' ';			/* Erase any maybe embedded \0 */
    }
    *ptr = 0;					/* Trailing \0 */
    parse_string(buf, commands, func, mode);	/* parse config file */
}

/* This is a service function for drivers. Commands and func are as above.
   The following config files are parsed in this order:
    - /etc/vga/libvga.conf (#define SVGALIB_CONFIG_FILE)
    - ~/.svgalibrc
    - the file where env variavle SVGALIB_CONFIG_FILE points
    - the env variable SVGALIB_CONFIG (for compatibility, but I would remove
      it, we should be more flexible...  Opinions ?)
    - MW: I'd rather keep it, doesn't do too much harm and is sometimes nice
      to have.
*/
void __svgalib_read_options(char **commands, char *(*func) (int ind, int mode, char **nptr)) {
    FILE *file=NULL;
    char *buf = NULL, *ptr;
    int i;
    
    if ((ptr = getenv("SVGALIB_CARD"))) {
        char configfilename[256];
        snprintf(configfilename, 256, "%s.%s", SVGALIB_CONFIG_FILE, ptr);
        file = fopen(configfilename, "r");
        if (!file)
           fprintf(stderr, "svgalib: Warning config file \'%s\' not found,\n"
                   "using default configfile \'%s\'\n", configfilename, SVGALIB_CONFIG_FILE); 
    }
    if (!file)
        file = fopen(SVGALIB_CONFIG_FILE, "r");
    if (file) {
#ifdef DEBUG_CONF
    fprintf(stderr,"Processing config file\n");
#endif
      process_config_file(file, 1, commands, func);
      fclose(file);
    } else {
	fprintf(stderr, "svgalib: Error: configuration file \'%s\' not found.\n", SVGALIB_CONFIG_FILE);
	exit(1);
    }

    if ( (ptr = getenv("HOME")) != 0) {
      char *filename; 

      filename = alloca(strlen(ptr) + 20);
      if (!filename) {
	fprintf(stderr,"svgalib: out of mem while parsing SVGALIB_CONFIG_FILE !\n");
      } else {
	strcpy(filename, ptr);
	strcat(filename, "/.svgalibrc");
	if ( (file = fopen(filename, "r")) != 0) {
#ifdef DEBUG_CONF
	  fprintf(stderr,"Processing config file \'%s\'\n", filename);
#endif
	  process_config_file(file, allowoverride, commands, func);
	  fclose(file);
	}
      }
    }

    if ( (ptr = getenv("SVGALIB_CONFIG_FILE")) != 0) {
      if ( (file = fopen(ptr, "r")) != 0) {
#ifdef DEBUG_CONF
  fprintf(stderr,"Processing config file \'%s\'\n", ptr);
#endif
	process_config_file(file, allowoverride, commands, func);
	fclose(file);
      } else {
        fprintf(stderr, "svgalib: warning: config file \'%s\', pointed to by SVGALIB_CONFIG_FILE, not found !\n", ptr);
      }
    }

    if ( (ptr = getenv("SVGALIB_CONFIG")) != 0  &&  (i = strlen(ptr)) != 0) {
      buf = alloca(i + 1);
      if (!buf) {
	fprintf(stderr,"svgalib: out of mem while parsing SVGALIB_CONFIG !\n");
      } else {
	strcpy(buf, ptr);		/* Copy for safety and strtok!! */
#ifdef DEBUG_CONF
	fprintf(stderr,"Parsing env variable \'SVGALIB_CONFIG\'\n");
#endif
	parse_string(buf, commands, func, allowoverride);
      }
    }
}

static void map_vgaio(void)
{
    __svgalib_inmisc=__svgalib_vga_inmisc;
    __svgalib_outmisc=__svgalib_vga_outmisc;
    __svgalib_incrtc=__svgalib_vga_incrtc;
    __svgalib_outcrtc=__svgalib_vga_outcrtc;
    __svgalib_inseq=__svgalib_vga_inseq;
    __svgalib_outseq=__svgalib_vga_outseq;
    __svgalib_ingra=__svgalib_vga_ingra;
    __svgalib_outgra=__svgalib_vga_outgra;
    __svgalib_inatt=__svgalib_vga_inatt;
    __svgalib_outatt=__svgalib_vga_outatt;
    __svgalib_attscreen=__svgalib_vga_attscreen;
    __svgalib_inpal=__svgalib_vga_inpal;
    __svgalib_outpal=__svgalib_vga_outpal;
    __svgalib_inis1=__svgalib_vga_inis1;
};

static void map_vganullio(void)
{
    __svgalib_inmisc=__svgalib_vganull_inmisc;
    __svgalib_outmisc=__svgalib_vganull_outmisc;
    __svgalib_incrtc=__svgalib_vganull_incrtc;
    __svgalib_outcrtc=__svgalib_vganull_outcrtc;
    __svgalib_inseq=__svgalib_vganull_inseq;
    __svgalib_outseq=__svgalib_vganull_outseq;
    __svgalib_ingra=__svgalib_vganull_ingra;
    __svgalib_outgra=__svgalib_vganull_outgra;
    __svgalib_inatt=__svgalib_vganull_inatt;
    __svgalib_outatt=__svgalib_vganull_outatt;
    __svgalib_attscreen=__svgalib_vganull_attscreen;
    __svgalib_inpal=__svgalib_vganull_inpal;
    __svgalib_outpal=__svgalib_vganull_outpal;
    __svgalib_inis1=__svgalib_vganull_inis1;
};
/* Configuration file, mouse interface, initialization. */

static int configfileread = 0;	/* Boolean. */

/* What are these m0 m1 m... things ? Shouldn't they be removed ? */
static char *vga_conf_commands[] = {
/*  0 */    "mouse", "monitor", "!m", "!M", "chipset", "overrideenable", 
/*  6 */	"!m0", "!m1", "!m2", "!m3", "!m4", "!m9", 
/* 12 */	"!M0", "!M1", "!M2", "!M3", "!M4", "!M5", "!M6", 
/* 19 */	"nolinear", "linear",
/* 21 */	"!C0", "!C1", "!C2", "!C3", "!C4", "!C5", "!C6", "!C7", "!C8", "!C9",
/* 31 */    "!c0", "!c1", "monotext", "colortext", "!m5",
/* 36 */    "leavedtr", "cleardtr", "setdtr", "leaverts", "clearrts",
/* 41 */    "setrts", "grayscale", "horizsync", "vertrefresh", "modeline",
/* 46 */    "security","mdev", "default_mode", "nosigint", "sigint",
/* 51 */    "joystick0", "joystick1", "joystick2", "joystick3",
/* 55 */    "textprog", "vesatext", "vesasave", "secondary", "bandwidth", 
/* 60 */    "novccontrol", "newmode", "noprocpci", "vesatextmode", "device",
/* 65 */    "ragedoubleclock", "include", "nullio", "helper", "biosparams",
/* 70 */	"pcistart", "dimensions", "neomagiclibretto100",
/* 73 */	"nohelper", "nohelper_insecure", "fbdev_novga",
    NULL};

static char *conf_mousenames[] =
{
  "Microsoft", "MouseSystems", "MMSeries", "Logitech", "Busmouse", "PS2",
    "MouseMan", "gpm", "Spaceball", "none", "IntelliMouse", "IMPS2", "pnp", 
    "WacomGraphire", "DRMOUSE4DS", "ExplorerPS2", "unconfigured", NULL};

static int check_digit(char *ptr, char *digits)
{
    if (ptr == NULL)
	return 0;
    return strlen(ptr) == strspn(ptr, digits);
}

static char *param_needed(int command)
{
    fprintf(stderr,
        "svgalib: config: \'%s\' requires parameter(s)",
        vga_conf_commands[command]);
    return NULL;
}

static char *check_digit_fail(int command, char *ptr, char *usage)
{
    if (!ptr)
        return param_needed(command);
    fprintf(stderr,
        "svgalib: config: Error \'%s\' parameter: %s, is not a number)\n%s",
        vga_conf_commands[command], ptr, usage? usage:"");
    
    return ptr; /* Allow a second parse of str */
}

static char *override_denied(int command, char **nptr)
{
    fprintf(stderr,
        "svgalib: config: \'%s\' override from environment or .svgalibrc denied.\n",
        vga_conf_commands[command]);
    return __svgalib_token(nptr);
}

static char *process_option(int command, int mode, char **nptr)
{
    static char digits[] = ".0123456789";
    char *ptr, **tabptr, *ptb;
    int i, j;
    float f;

#ifdef DEBUG_CONF
    fprintf(stderr,"command %d detected.\n", command);
#endif
    switch (command) {
    case 5:
	if (!mode)
	    return override_denied(command, nptr);
#ifdef DEBUG_CONF
	fprintf(stderr,"Allow override\n");
#endif
	    allowoverride = 1;
	break;
    case 0:			/* mouse */
    case 2:			/* m */
	if (!(ptr = __svgalib_token(nptr)))
	    return param_needed(command);

	if (check_digit(ptr, digits + 1) &&
	    ((i=atoi(ptr)) >= 0) &&
	    (i<=9)) {
	    mouse_type = i;
	} else {		/* parse for symbolic name.. */
	    for (i = 0, tabptr = conf_mousenames; *tabptr; tabptr++, i++) {
		if (!strcasecmp(ptr, *tabptr)) {
		    mouse_type = i;
#ifdef DEBUG_DRMOUSE4DS
	fprintf(stderr, "mouse type: %d: %s \n", i, conf_mousenames[i]);
#endif
		    return __svgalib_token(nptr);
		}
	    }
	    fprintf(stderr,"svgalib: config: Illegal mouse setting: {mouse|m} %s\n"
		   "Correct usage: {mouse|m} mousetype\n"
		   "where mousetype is one of 0, 1, 2, 3, 4, 5, 6, 7, 9,\n",
		   ptr);
	    for (tabptr = conf_mousenames, i = 0; *tabptr; tabptr++, i++) {
		if (i == MOUSE_NONE)
		    continue;
		fprintf(stderr,"%s, ", *tabptr);
	    }
	    fprintf(stderr,"or none.\n");
	    return ptr;		/* Allow a second parse of str */
	}
	break;
    case 1:			/* monitor */
    case 3:			/* M */
	ptr = __svgalib_token(nptr);
	if (check_digit(ptr, digits + 1)) {	/* It is an int.. */
            if (!mode)
                return override_denied(command, nptr);
	    i = atoi(ptr);
	    if (i < 7) {
	        __svgalib_horizsync.max = __svgalib_maxhsync[i];
	    } else {
                __svgalib_horizsync.max = i * 1000.0f;
	    }
	} else if (check_digit(ptr, digits)) {	/* It is a float.. */
	    if (!mode)
                return override_denied(command, nptr);
	    f = atof(ptr);
	    __svgalib_horizsync.max = f * 1000.0f;
	} else {
	    return check_digit_fail(command, ptr,
		   "Correct usage: {monitor|M} monitortype\n"
		   "where monitortype is one of 0, 1, 2, 3, 4, 5, 6, or\n"
		   "maximal horz. scan frequency in khz.\n"
		   "Example: monitor 36.5\n");
	}
	break;
    case 4:			/* chipset */
	if (CHIPSET != UNDEFINED)
		return NULL;

	if (!(ptr = __svgalib_token(nptr)))
	    return param_needed(command);
	/*First param is chipset */
	for (i = 0, tabptr = driver_names; *tabptr; tabptr++, i++) {
	    if (!strcasecmp(ptr, *tabptr)) {
		if (!__svgalib_driverspecslist[i]) {
		    fprintf(stderr,"svgalib: config: Warning chipset driver %s is NOT compiled in.\n",
			ptr);
                    return __svgalib_token(nptr);
		}
		if (__svgalib_driverspecslist[i]->disabled) {
		    fprintf(stderr,"svgalib: config: Warning chipset driver %s has been disabled.\n",
			ptr);
		    return __svgalib_token(nptr);
		}
		ptr = __svgalib_token(nptr);
		if (!check_digit(ptr, digits + 1)) {
                    if (!mode)
                        return override_denied(command, nptr);
                    configfile_chipset = i;
                    return ptr;
                }
		    j = atoi(ptr);
		    ptr = __svgalib_token(nptr);
		if (!check_digit(ptr, digits + 1))
		    return check_digit_fail(command, ptr,
		        "Correct usage: chipset driver [par1 par2]\n");
                if (!mode)
                    return override_denied(command, nptr);
                
                configfile_chipset = i;
                configfile_params  = 1;
                configfile_par1    = j;
                configfile_par2    = atoi(ptr);
			return __svgalib_token(nptr);
	    }
	}
	fprintf(stderr,"svgalib: config: Illegal chipset setting: chipset %s\n", ptr);
	fprintf(stderr,"Correct usage: chipset driver [par1 par2]\n"
	     "where driver is one of:\n");
	ptb = "%s";
	for (i = 0, tabptr = driver_names; *tabptr; tabptr++, i++) {
	    if (__svgalib_driverspecslist[i] != NULL) {
		fprintf(stderr,ptb, *tabptr);
		ptb = ", %s";
	    }
	}
	fprintf(stderr,"\npar1 and par2 are driver dependant integers.\n"
	     "Example: Chipset VGA    or\n"
	     "Chipset VGA 0 512\n");
        return ptr;		/* Allow a second parse of str */
    case 6:			/* oldstyle config: m0-m4 */
    case 7:
    case 8:
    case 9:
    case 10:
	mouse_type = command - 6;
	break;
    case 11:			/* m9 */
	mouse_type = MOUSE_NONE;
	break;
    case 12:			/* oldstyle config: M0-M6 */
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
        if (!mode)
            return override_denied(command, nptr);
	    __svgalib_horizsync.max = __svgalib_maxhsync[command - 12];
	break;
    case 19:			/*nolinear */
	modeinfo_mask &= ~CAPABLE_LINEAR;
	break;
    case 20:			/*linear */
	modeinfo_mask |= CAPABLE_LINEAR;
	break;
    case 21:			/* oldstyle chipset C0 - C9 */
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
	if (!mode)
            return override_denied(command, nptr);
	vga_setchipset(command - 21);
	break;
    case 31:			/* c0-c1 color-text selection */
        if (!mode)
            return override_denied(command, nptr);
	color_text = 0;
	break;
    case 32:
        if (!mode)
            return override_denied(command, nptr);
	color_text = 1;
	break;
    case 33:
    case 34:
	if (!mode)
            return override_denied(command, nptr);
	color_text = command - 32;
	break;
    case 35:			/* Mouse type 5 - "PS2". */
	mouse_type = 5;
	break;
    case 36:
	mouse_modem_ctl &= ~(MOUSE_CHG_DTR | MOUSE_DTR_HIGH);
	break;
    case 37:
	mouse_modem_ctl &= ~MOUSE_DTR_HIGH;
	mouse_modem_ctl |= MOUSE_CHG_DTR;
	break;
    case 38:
	mouse_modem_ctl |= (MOUSE_CHG_RTS | MOUSE_RTS_HIGH);
	break;
    case 39:
	mouse_modem_ctl &= ~(MOUSE_CHG_RTS | MOUSE_RTS_HIGH);
	break;
    case 40:
	mouse_modem_ctl &= ~MOUSE_RTS_HIGH;
	mouse_modem_ctl |= MOUSE_CHG_RTS;
	break;
    case 41:
	mouse_modem_ctl |= (MOUSE_CHG_RTS | MOUSE_RTS_HIGH);
	break;
    case 42:			/* grayscale */
	__svgalib_grayscale = 1;
	break;
    case 43:			/* horizsync */
	ptr = __svgalib_token(nptr);
	if (check_digit(ptr, digits))
	{
	    f = atof(ptr);
	ptr = __svgalib_token(nptr);
	    if (check_digit(ptr, digits))
	    {
                __svgalib_horizsync.min = f * 1000;
                __svgalib_horizsync.max = atof(ptr) * 1000;
                break;
	    }
	}
	return check_digit_fail(command, ptr,
		   "Correct usage: HorizSync min_kHz max_kHz\n"
		   "Example: HorizSync 31.5 36.5\n");
    case 44:			/* vertrefresh */
	ptr = __svgalib_token(nptr);
	if (check_digit(ptr, digits))
	{
	    f = atof(ptr);
	ptr = __svgalib_token(nptr);
	    if (check_digit(ptr, digits))
	    {
                __svgalib_vertrefresh.min = f;
                __svgalib_vertrefresh.max = atof(ptr);
                break;
	    }
	}
	return check_digit_fail(command, ptr,
		   "Correct usage: VertRefresh min_Hz max_Hz\n"
		   "Example: VertRefresh 50 70\n");
    case 45:{			/* modeline */
	    MonitorModeTiming mmt;
	    const struct {
		char *name;
		int val;
	    } options[] = {
		{
		    "-hsync", NHSYNC
		},
		{
		    "+hsync", PHSYNC
		},
		{
		    "-vsync", NVSYNC
		},
		{
		    "+vsync", PVSYNC
		},
		{
		    "interlace", INTERLACED
		},
		{
		    "interlaced", INTERLACED
		},
		{
		    "doublescan", DOUBLESCAN
		},
		{
		    "tvmode", TVMODE
		},
		{
		    "tvpal", TVPAL
		},
		{
		    "tvntsc", TVNTSC
		}
	    };
#define ML_NR_OPTS (sizeof(options)/sizeof(*options))

	    /* Skip the name of the mode */
            if (!(ptr = __svgalib_token(nptr)))
                return param_needed(command);

	    ptr = __svgalib_token(nptr);
	    if (check_digit(ptr, digits))
	    mmt.pixelClock = atof(ptr) * 1000;
            else
                return check_digit_fail(command, ptr, NULL);

#define ML_GETINT(x) \
	ptr = __svgalib_token(nptr); \
	if (check_digit(ptr, digits+1)) \
	    x = atoi(ptr); \
        else \
            return check_digit_fail(command, ptr, NULL);

	    ML_GETINT(mmt.HDisplay);
	    ML_GETINT(mmt.HSyncStart);
	    ML_GETINT(mmt.HSyncEnd);
	    ML_GETINT(mmt.HTotal);
	    ML_GETINT(mmt.VDisplay);
	    ML_GETINT(mmt.VSyncStart);
	    ML_GETINT(mmt.VSyncEnd);
	    ML_GETINT(mmt.VTotal);
	    mmt.flags = 0;
	    while ((ptr = __svgalib_token(nptr))) {
		for (i = 0; i < ML_NR_OPTS; i++)
		    if (!strcasecmp(ptr, options[i].name))
                      {
                        mmt.flags |= options[i].val;
                        break;
                      }
		if (i == ML_NR_OPTS)
		    break;
	    }
#undef ML_GETINT
#undef ML_NR_OPTS

	    __svgalib_addusertiming(&mmt);
	    return ptr; /* no modeline option so most likely new config command */
	}
    case 46:
	if ( (ptr = __svgalib_token(nptr)) ) {
	    if (!strcasecmp("revoke-all-privs", ptr)) {
                 if (!mode)
                     return override_denied(command, nptr);
		 __svgalib_security_revokeallprivs = 1;
		 break;
	    } else if (!strcasecmp("compat", ptr)) {
                 if (!mode)
                     return override_denied(command, nptr);
		 __svgalib_security_revokeallprivs = 0;
		 break;
	    }
	    fprintf(stderr,
	        "svgalib: config: Illegal security setting: Security %s\n",
	        ptr);
	    return ptr;		/* Allow a second parse of str */
	} 
	return param_needed(command);
    case 47:
	ptr = __svgalib_token(nptr);
	if (ptr) {
	    __svgalib_mouse_device = strdup(ptr);
	    if (__svgalib_mouse_device == NULL) {
		fprintf(stderr,"svgalib: Fatal error: out of memory.\n");
		exit(1);
	    }
	} else
	    return param_needed(command);
	break;
    case 48:		/* default_mode */
	if ( (ptr = __svgalib_token(nptr)) != 0) {
	 int mode = vga_getmodenumber(ptr);
	  if (mode != -1) {
	    __svgalib_default_mode = mode;
	  } else {
	    fprintf(stderr,"svgalib: config: illegal mode \'%s\' for \'%s\'\n",
	   			  ptr, vga_conf_commands[command]);
            return ptr;		/* Allow a second parse of str */
	  }
	} else
	    return param_needed(command);
	break;
    case 49: /* nosigint */
	__svgalib_nosigint = 1;
	break;
    case 50: /* sigint */
	__svgalib_nosigint = 0;
	break;
    case 51: /* joystick0 */
    case 52: /* joystick1 */
    case 53: /* joystick2 */
    case 54: /* joystick3 */
	if (! (ptr = __svgalib_token(nptr)) )
            return param_needed(command);

	if (__joystick_devicenames[command - 51])
	    free(__joystick_devicenames[command - 51]);
	__joystick_devicenames[command - 51] = strdup(ptr);
	if (!__joystick_devicenames[command - 51]) {
            fprintf(stderr,"svgalib: Fatal error: out of memory.\n");
            exit(1);
        }
	break;
    case 55: /* TextProg */
        if (!(ptr = __svgalib_token(nptr)))
            return param_needed(command);

	if (!mode && __svgalib_nohelper) {
	    /* skip textprog args */
            while(((ptr=__svgalib_token(nptr))!=NULL) && strcmp(ptr,"END")) {}
            return override_denied(command, nptr);
	}

        __svgalib_textprog|=2;
	__svgalib_TextProg = strdup(ptr);
	if (!__svgalib_TextProg) {
            fprintf(stderr,"svgalib: Fatal error: out of memory.\n");
            exit(1);
        }
        i=1;
        while(((ptr=__svgalib_token(nptr))!=NULL) &&
	       (i< ((sizeof(__svgalib_TextProg_argv) / sizeof(char *)) + 1)) &&
	       strcmp(ptr,"END")){
	   __svgalib_TextProg_argv[i]=strdup(ptr);
	   if (!__svgalib_TextProg_argv[i]) {
                fprintf(stderr,"svgalib: Fatal error: out of memory.\n");
                exit(1);
           }
	   i++;
        };
        __svgalib_TextProg_argv[i]=NULL;
        ptb=strrchr(__svgalib_TextProg,'/');
        __svgalib_TextProg_argv[0]=ptb?ptb + 1:__svgalib_TextProg;
        break;
    case 56:
#ifdef INCLUDE_VESA_DRIVER
        __svgalib_vesatext=1;
        break;
#else
       fprintf(stderr,"svgalib: Warning: VESA support not enabled!\n");
       break;
#endif
    case 57: /* Vesa save bitmap */  
#ifdef INCLUDE_VESA_DRIVER
       ptr = __svgalib_token(nptr);
       if(check_digit(ptr, digits+1)) {
         j = atoi(ptr);
         __svgalib_VESA_savebitmap=j;
       } else
         return check_digit_fail(command, ptr, NULL);
#else
       fprintf(stderr,"svgalib: Warning: VESA support not enabled!\n");
#endif
       break;
    case 58:
        __svgalib_secondary=1;
        break;
    case 59:		/* max bandwidth */
	    ptr = __svgalib_token(nptr);
	    if (check_digit(ptr, digits+1)) {
	        int f = atoi(ptr);
                if (!mode)
                    return override_denied(command, nptr);
	        if (f<31000)f=31000;
	        __svgalib_bandwidth = f;
	    } else
	        return check_digit_fail(command, ptr,
                                "Correct usage: Bandwidth bandwidth\n"
                                "Example: Bandwidth 50000\n");
	break;
    case 60:
        __svgalib_novccontrol=1;
        break;
    case 61: {
        int x,y,c,p,b;
    	ptr = __svgalib_token(nptr); 
        if(check_digit(ptr, digits+1))
        	x = atoi(ptr);
			else return check_digit_fail(command, ptr, NULL);

		ptr = __svgalib_token(nptr);
        if(check_digit(ptr, digits+1))
        	y = atoi(ptr);
			else return check_digit_fail(command, ptr, NULL);

		ptr = __svgalib_token(nptr);
		if(check_digit(ptr, digits+1))
			c = atoi(ptr);
			else return check_digit_fail(command, ptr, NULL);

		ptr = __svgalib_token(nptr);
		if(check_digit(ptr, digits+1))
        	p = atoi(ptr);
			else return check_digit_fail(command, ptr, NULL);

		ptr = __svgalib_token(nptr);
		if(check_digit(ptr, digits+1))
        	b = atoi(ptr);
			else return check_digit_fail(command, ptr, NULL);
        
		vga_addmode(x,y,c,p,b);
        };
        break;
    case 62:
        fprintf(stderr,"svgalib: config: Warning: Direct PCI access is not available anymore, so option \"noprocpci\" ignored\n");
        break;
    case 63: /* Vesa text mode number */  
#ifdef INCLUDE_VESA_DRIVER
       ptr = __svgalib_token(nptr);
	if(check_digit(ptr, digits+1)){
		if (!mode)
			return override_denied(command, nptr);
		j = atoi(ptr);
		__svgalib_VESA_textmode=j;
	} else return check_digit_fail(command, ptr, NULL);
#else
       fprintf(stderr,"svgalib: Warning: VESA support not enabled!\n");
#endif
       break;
    case 64:			/* device number */
		ptr = __svgalib_token(nptr);
		if (check_digit(ptr, digits+1) && ((j = atoi(ptr)) >= 0) && (j <= 16)){
			__svgalib_pci_helper_idev=j;
		} else return check_digit_fail(command, ptr, NULL);
		break;
    case 65:
	if (!mode) return override_denied(command, nptr);
        __svgalib_ragedoubleclock=1;
        break;
    case 66: { /* include, beware of loops */
		FILE *file;
		if (!(ptr = __svgalib_token(nptr)))
			return param_needed(command);
		if ( (file = fopen(ptr, "r")) != 0) {
#ifdef DEBUG_CONF
			fprintf(stderr,"Processing config file \'%s\'\n", ptr);
#endif
			process_config_file(file, mode, vga_conf_commands, process_option);
			fclose(file);
		} else {
			fprintf(stderr, "svgalib: config: file \'%s\' not found.\n", ptr);
		}
	 		 } 
        break;
    case 67:
        map_vganullio(); /* drivers may override this */
        break;
    case 68: /* helper */
		ptr = __svgalib_token(nptr);
		if (ptr) {
			helper_device = strdup(ptr);
			if (helper_device == NULL) {
			fputs("svgalib: Fatal error: out of memory.\n",stderr);
			exit(1);
			}
		} else
 			return param_needed(command);
		break;
	case 69:
		for (i=0; i<16; i++) {
   			ptr=__svgalib_token(nptr);
			if(!check_digit(ptr,digits+1)) {
				if (mode)
					biosparams=i;
				return ptr;
			}
			if (mode)
				biosparam[i]=atoi(ptr);
		}
		if (mode)
			biosparams=1;
		else
			return override_denied(command, nptr);
		break;
    case 70:            /* pci initial values */
		ptr = __svgalib_token(nptr);
		if(check_digit(ptr, digits+1) &&
                   ((i = atoi(ptr)) < 16) &&
                   (i >= 0)) {
			ptr = __svgalib_token(nptr);
			if (check_digit(ptr, digits+1) &&
					((j = atoi(ptr)) < 256) &&
					(j >= 0)) {
				__svgalib_pci_nohelper_idev = i*256 + j;
				break;
			}
		}
		check_digit_fail(command, ptr,
				"Correct usage: PCIStart initial_bus initial_dev"
				"Example: PCIStart 1 0\n");
		break;
	case 71: { /* dimemsions */
	 		 int _minx, _miny, _maxx, _maxy;
	 		 
	 		 ptr = __svgalib_token(nptr);
	 		 if (check_digit(ptr, digits+1))
	 			 _minx = atoi(ptr);
			 else return check_digit_fail(command, ptr, NULL);
			
			 ptr = __svgalib_token(nptr);
 			 if (check_digit(ptr, digits+1))
				 _miny = atoi(ptr);
	 		 else return check_digit_fail(command, ptr, NULL);
			
			 ptr = __svgalib_token(nptr);
 			 if (check_digit(ptr, digits+1))
 				 _maxx = atoi(ptr);
 			 else return check_digit_fail(command, ptr, NULL);
			
			 ptr = __svgalib_token(nptr);
 			 if (check_digit(ptr, digits+1))
 				 _maxy = atoi(ptr);
 			 else return check_digit_fail(command, ptr, NULL);
 			 
 			 if (!mode) return override_denied(command, nptr);
 			 
 			 minx = _minx;
 			 miny = _miny;
 			 maxx = _maxx;
 			 maxy = _maxy;
			 }
		break;
	case 72:
		if (!mode)
			return override_denied(command, nptr);
		__svgalib_neolibretto100=1;
		break;
	case 73:
		if (!mode)
			return override_denied(command, nptr);
		__svgalib_nohelper = 1;
		break;
	case 74:
		if (!mode)
			return override_denied(command, nptr);
		__svgalib_nohelper = 1;
		__svgalib_nohelper_secure = 0;
		break;		
	case 75:
		__svgalib_fbdev_novga=1;
		break;
  	}

	return __svgalib_token(nptr);
}

static void readconfigfile(void)
{
    if (configfileread)
		return;

	configfileread = 1;
    mouse_type = -1;

    map_vgaio();

    __svgalib_read_options(vga_conf_commands, process_option);

#ifndef __PPC
    if(__svgalib_secondary)
#endif
         __svgalib_emulatepage=1;

	/* Can't access /dev/mem after init, so we can't do vga modes with fbdev.
       Note we must do this check before calling setchipset! */
    if(__svgalib_nohelper && __svgalib_nohelper_secure)
        __svgalib_fbdev_novga = 1;

	if ((CHIPSET == UNDEFINED) && (configfile_chipset != UNDEFINED))
    {
        if (configfile_params)
            vga_setchipsetandfeatures(configfile_chipset, configfile_par1,
                configfile_par2);
        else
            vga_setchipset(configfile_chipset);
    }
    if (mouse_type == -1) {
		mouse_type = MOUSE_MICROSOFT;	/* Default. */
		fprintf(stderr,"svgalib: Assuming Microsoft mouse.\n");
    }
    if (__svgalib_horizsync.max == 0U) {
		/* Default monitor is low end SVGA/8514. */
		__svgalib_horizsync.min = 31500U;
		__svgalib_horizsync.max = 35500U;
		fprintf(stderr,"svgalib: Assuming low end SVGA/8514 monitor (35.5 KHz).\n");
    }
#ifdef DEBUG_CONF
    fprintf(stderr,"Mouse is: %d Monitor is: H(%5.1f, %5.1f) V(%u,%u)\n", mouse_type,
      __svgalib_horizsync.min / 1000.0, __svgalib_horizsync.max / 1000.0,
	   __svgalib_vertrefresh.min, __svgalib_vertrefresh.max);
    fprintf(stderr,"Mouse device is: %s",__svgalib_mouse_device);
#endif

}

int vga_getmousetype(void)
{
    readconfigfile();
    return mouse_type | mouse_modem_ctl;
}

int vga_getmonitortype(void)
{				/* obsolete */
    int i;
    readconfigfile();
    for (i = 1; i <= MON1024_72; i++)
	if (__svgalib_horizsync.max < __svgalib_maxhsync[i])
	    return i - 1;

    return MON1024_72;
}

void vga_setmousesupport(int s)
{
    DTP((stderr,"setmousesupport %i\n",s));
    mouse_support = s;
}

void vga_lockvc(void)
{
    lock_count++;
    if (flip)
	__svgalib_waitvtactive();
}

void vga_unlockvc(void)
{
    if (--lock_count <= 0) {
	lock_count = 0;
	if (release_flag) {
	    release_flag = 0;
	    __svgalib_releasevt_signal(SVGALIB_RELEASE_SIG);
	}
    }
}

void vga_runinbackground(int stat, ...)
{
    va_list params;
    
    va_start(params,stat);
   
    /* getchipset opens, maps and perhaps closes the LFB,
       we need an open LFB to be able to runinbackground.
       Also we need to know the chipset for getmodeinfo(). */
    __svgalib_getchipset();
    
	if(__svgalib_novccontrol)return;
	
    switch (stat) {
    	case VGA_GOTOBACK:
			__svgalib_go_to_background = va_arg(params, void *);
			break;
	    case VGA_COMEFROMBACK:
			__svgalib_come_from_background = va_arg(params, void *);
			break;
	    default:
			if ( ( stat &&  __svgalib_runinbackground) ||
	   				(!stat && !__svgalib_runinbackground) )
   				break; /* nothing todo */
	 		if (stat) {
	   			if (__svgalib_readpage != __svgalib_writepage) {
		 			fprintf(stderr,
			   				"svgalib: Warning runinbackground does not support seperate\n"
					 		"read- and writepages, ignoring runinbackground request.\n");
		  			break;
	  			}
				/* If we don't have /dev/mem available we can't do runinbackground
				 * with the real banked mem, see if we do have an fd for the LFB
				 * and if it is ok to switch to emulating pages. */
	  			if (__svgalib_mem_fd == -1) {
					int hasmode = 0;
					if (__svgalib_linear_mem_fd != -1)
					{
					    if (!__svgalib_emulatepage)
					    {
						/* see if we can do CM with emulatepage */
						__svgalib_emulatepage = 1;
						hasmode = vga_hasmode(CM);
						if (hasmode) {
							__svgalib_setrdpage=NULL;
							__svgalib_setwrpage=NULL;
							if (CM != TEXT) {
								vga_modeinfo *modeinfo = vga_getmodeinfo(CM);
								(*__svgalib_setpage)(vga_page_offset);
								if ((modeinfo->flags & CAPABLE_LINEAR) &&
									!(modeinfo->flags & LINEAR_USE)) {
									__svgalib_driverspecs->linear(LINEAR_ENABLE, 0);
								}
							}
							map_banked(MAP_FIXED);
						}
						else
							__svgalib_emulatepage = 0;
					    }
					    else
					    	hasmode = 1;
					}
					if (!hasmode) {
						fprintf(stderr,
								"svgalib: Warning runinbackground not supported "
								"in nohelper mode, ignoring\n"
								"  runinbackground request. Program should check "
								"vga_runinbackground_version\n"
								"  before calling vga_runinbackground\n");
						break;
					}
				}
			}
			else /* if (!stat) */
            	__svgalib_waitvtactive();

	        __svgalib_runinbackground = stat;
	}
}

/*  Program can check, if it is safe to in background. */
int vga_runinbackground_version(void)
{
	const int runinbackground_version = 3;
	int hasmode;

        /* getchipset opens, maps and perhaps closes the LFB,
	   we need an open LFB to be able to runinbackground.
	   Also we need to know the chipset for getmodeinfo(). */
        __svgalib_getchipset();
    
	if (__svgalib_mem_fd != -1)
		return runinbackground_version;
	
	/* we don't have /dev/mem available so we can't do runinbackground
	   with the real banked mem, see if we do have an fd for the LFB
	   and if it is ok to switch to emulating pages. */
        if (__svgalib_linear_mem_fd == -1)
                return 0;

        if (__svgalib_emulatepage)
                return runinbackground_version;
	
	/* see if we can do CM with emulatepage */
	__svgalib_emulatepage = 1;
	hasmode = vga_hasmode(CM);
	__svgalib_emulatepage = 0;
	if (hasmode)
                return runinbackground_version;
	
	return 0;
}

int vga_oktowrite(void)
{
    if(__svgalib_secondary)return 1;
    return __svgalib_oktowrite;
}

void vga_chipset_setregs(unsigned char regs[])
{
    chipset_setregs(regs, TEXT); /* Why TEXT? Can't think of smthg else*/
}

void vga_chipset_saveregs(unsigned char regs[])
{
    chipset_saveregs(regs);
}

#ifdef _SVGALIB_LRMI
int vga_set_LRMI_callbacks(LRMI_callbacks * LRMI) {

	if (__svgalib_LRMI_callbacks)
		return 0;
    
	__svgalib_LRMI_callbacks = LRMI;
    
	return 1;
}
#endif

int vga_simple_init(void)
{

    __svgalib_simple = 1;
    __svgalib_novccontrol = 1;
    __svgalib_driver_report=0;
    return vga_init();

}

int vga_initf(int flags) {
    if(flags&1)
        __svgalib_novccontrol=2;
    if(flags&2)
        __svgalib_secondary=2;
    return vga_init();
}

int vga_init(void)
{
    int retval = -1;

	if(initialized) return 0;
	
    /*
     * Make sure we know where our stdout/stderr are going
     *  (based on code by Kevin Vajk)
     */
    if ((fcntl(0,F_GETFD) == -1) && (open("/dev/null", O_RDONLY) == -1)){
        perror("/dev/null");
        goto bail;
    }
    if ((fcntl(1,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)){
        perror("/dev/null");
        goto bail;
    }
    if ((fcntl(2,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)){
        perror("/dev/null");
        goto bail;
    }
    retval = 0;

    DTP((stderr,"init\n"));

    /* Make sure the chipset is known. This will also:
       -read the config file
       -set the default LRMI callbacks
       -call open_mem()
       -call __svgalib_open_devconsole(); */
    __svgalib_getchipset();		

    if(__svgalib_driver_report) {
        fprintf(stderr,"svgalib %s\n", versionstr);
    }

#ifdef SET_TERMIO
    if(!__svgalib_novccontrol) {
            /* save text mode termio parameters */
            ioctl(0, TCGETS, &__svgalib_text_termio);
    
            __svgalib_graph_termio = __svgalib_text_termio;
    
            /* change termio parameters to allow our own I/O processing */
            __svgalib_graph_termio.c_iflag &= ~(BRKINT | PARMRK | INPCK | IUCLC | IXON | IXOFF);
            __svgalib_graph_termio.c_iflag |= (IGNBRK | IGNPAR);
    
            __svgalib_graph_termio.c_oflag &= ~(ONOCR);
    
            __svgalib_graph_termio.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | NOFLSH);
            if (__svgalib_nosigint)
	        __svgalib_graph_termio.c_lflag &= ~ISIG;	/* disable interrupt */
            else
	        __svgalib_graph_termio.c_lflag |=  ISIG;	/* enable interrupt */
    
            __svgalib_graph_termio.c_cc[VMIN] = 1;
            __svgalib_graph_termio.c_cc[VTIME] = 0;
            __svgalib_graph_termio.c_cc[VSUSP] = 0;	/* disable suspend */
    }
#endif

    DPRINTF("svgalib: Opening mouse (type = %x).\n", mouse_type | mouse_modem_ctl);

    if(mouse_type != MOUSE_NONE ) {
        __svgalib_mouse_flag=mouse_type | mouse_modem_ctl;
        if (mouse_init(__svgalib_mouse_device, mouse_type | mouse_modem_ctl, MOUSE_DEFAULTSAMPLERATE))
           fprintf(stderr,"svgalib: Failed to initialize mouse.\n");
        else
           mouse_open = 1;
    }

    /* Michael: I assume this is a misunderstanding, when svgalib was developed,
       there were no saved uids, thus setting effective uid sufficed... */
bail:
	if ( !__svgalib_security_norevokeprivs ) {
		if ( __svgalib_security_revokeallprivs == 1 ) {
			setuid(getuid());  
			setgid(getgid());
		}
		seteuid(getuid());
		setegid(getgid());
	}
	
	return retval;
}

void vga_norevokeprivs(void) {
	__svgalib_security_norevokeprivs = 1;
}

int vga_addtiming( int pixelClock,
   		   int HDisplay,	
                   int HSyncStart,
                   int HSyncEnd,
                   int HTotal,
                   int VDisplay,
                   int VSyncStart,
                   int VSyncEnd,
                   int VTotal,
                   int flags) {

   MonitorModeTiming mmt;

   mmt.pixelClock=pixelClock;
   mmt.HDisplay=HDisplay;
   mmt.HSyncStart=HSyncStart;
   mmt.HSyncEnd=HSyncEnd;
   mmt.HTotal=HTotal;
   mmt.VDisplay=VDisplay;
   mmt.VSyncStart=VSyncStart;
   mmt.VSyncEnd=VSyncEnd;
   mmt.VTotal=VTotal;
   mmt.flags=flags;
   
   __svgalib_addusertiming(&mmt);

   return 1;

};

int vga_addmode(int xdim, int ydim, int cols, 
                          int xbytes, int bytespp)
{
   int i;

   i=__svgalib_addmode(xdim, ydim, cols, xbytes, bytespp);

   return i;
};

int vga_getcrtcregs(unsigned char *regs) {
    int i;

    for (i = 0; i < CRT_C; i++) {
        regs[i] = __svgalib_incrtc(i);
    }
    return 0;
}

int vga_setcrtcregs(unsigned char *regs) {
    int i;
    
    if(!STDVGAMODE(CM)) return -1; 
    
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
    for (i = 0; i < CRT_C; i++) {
        __svgalib_outcrtc(i,regs[i]);
    }

    return 0;
}

int __svgalib_vgacolor(void)
{
   if (__svgalib_vgacolormode==-1)
       __svgalib_vgacolormode=__svgalib_inmisc()& 0x01;
   return __svgalib_vgacolormode;
};

void vga_dpms(int i) {
    int s1, c17;
    switch(i) {
        case 1: /* standby - not on vga */
            s1=0x20;
            c17=0x80;
            break;
        case 2:	/* suspend - not on vga */
            s1=0x20;
            c17=0x80;
            break;
        case 3: /* display off */
            s1=0x20;
            c17=0;
            break;
        default: /* display on */
            s1=0;
            c17=0x80;
            break;
    }
    __svgalib_outseq(0,1);
    s1|=__svgalib_inseq(1)&~0x20;
    __svgalib_outseq(1,s1);
    c17|=__svgalib_incrtc(0x17)&~0x80;
    usleep(10000);
    __svgalib_outcrtc(0x17,c17);
    __svgalib_outseq(0,3);
}

void map_mmio() {
    unsigned long offset;

    if(mmio_mapped) return;

#ifdef __alpha__
    offset = 0x300000000ULL;
#else
    offset = 0;
#endif

    if(__svgalib_mmio_size) {
        mmio_mapped=1;
        MMIO_POINTER=mmap( 0, __svgalib_mmio_size, PROT_READ | PROT_WRITE,
            		   MAP_SHARED, __svgalib_mem_fd, (off_t) __svgalib_mmio_base + offset);
#ifdef __alpha__ 
    	SPARSE_MMIO=mmap(0, __svgalib_mmio_size<<5, PROT_READ | PROT_WRITE,
            		   MAP_SHARED, __svgalib_mem_fd, 
                           (off_t) 0x200000000LL + (__svgalib_mmio_base<<5));

#endif
    } else {
        MMIO_POINTER=NULL;
        SPARSE_MMIO=NULL;
    }
}

void map_mem() {
    if(mem_mapped) return;
    map_banked(0);
    map_linear(0);
    mem_mapped=1;
}

void map_banked(int flags) {
    if(__svgalib_banked_mem_size==0)__svgalib_banked_mem_size=0x10000;
    if (__svgalib_emulatepage)
    {
        unsigned long offset = __svgalib_linear_mem_base;
#ifdef __alpha__
        if (offset)
            offset += 0x300000000ULL;
#endif
        if(__svgalib_readpage == -1)
            __svgalib_readpage = __svgalib_writepage = 0;
        
        BANKED_POINTER=mmap(BANKED_POINTER, 65536, PROT_READ|PROT_WRITE,
            MAP_SHARED|flags, __svgalib_linear_mem_fd,
            offset + (__svgalib_readpage<<16));
    } else {
		unsigned long offset;
    
#ifdef __alpha__
		offset = 0x300000000ULL;
#else
		offset = 0;
#endif
		BANKED_POINTER=mmap(BANKED_POINTER, __svgalib_banked_mem_size,
			PROT_READ|PROT_WRITE, MAP_SHARED|flags, __svgalib_mem_fd,
			(off_t) __svgalib_banked_mem_base + offset);
	}
}

void map_linear(int flags){
	/* If we're called for the inital mmap (flags==0) check there
	 * is linear memory, otherwise check if we did the initial mmap */
	if((!flags && __svgalib_linear_mem_size) || LINEAR_POINTER) {
		unsigned long offset = __svgalib_linear_mem_base;
#ifdef __alpha__
		if (offset)
			offset += 0x300000000ULL;
#endif
		LINEAR_POINTER=mmap(LINEAR_POINTER, __svgalib_linear_mem_size,
				PROT_READ | PROT_WRITE, MAP_SHARED|flags,
				__svgalib_linear_mem_fd, offset);
	}
}

void unmap_linear(void) {
	munmap(LINEAR_POINTER, __svgalib_linear_mem_size);
	__svgalib_linear_mem_size = 0;
	LINEAR_POINTER = NULL;
}
