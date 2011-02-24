/*
Skeleton chipset driver 
*/

#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "timing.h"
#include "vgaregs.h"
#include "interface.h"
#include "vgapci.h"

#define SKREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define TOTAL_REGS (VGA_TOTAL_REGS + 100)

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory;
static unsigned int linear_base;

static CardSpecs *cardspecs;

static void setpage(int page)
{
}

/* Fill in chipset specific mode information */
static void getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088;
    modeinfo->startaddressrange = memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;

    if (modeinfo->bytesperpixel >= 1) {
		if(linear_base)modeinfo->flags |= CAPABLE_LINEAR;
    }
}

/* Read and save chipset-specific registers */

static int saveregs(unsigned char regs[])
{ 
  int i;

    unlock();		
    return TOTAL_REGS - VGA_TOTAL_REGS;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    int i;

    unlock();		

}


/* Return nonzero if mode is available */

static int modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];
    if (memory * 1024 < info->ydim * info->xbytes)
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

/* Local, called by setmode(). */

static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ /* int k; */
    int tmp, tmptot, tmpss, tmpse, tmpbs, tmpbe, k;
    int offset;
   
    __svgalib_setup_VGA_registers(moderegs, modetiming, modeinfo);

    return ;
}


static int setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(SK_TOTAL_REGS);

    initializemode(moderegs, modetiming, modeinfo, mode);
    free(modetiming);

    __svgalib_setregs(moderegs);	/* Set standard regs. */
    setregs(moderegs, mode);		/* Set extended regs. */
    free(moderegs);

    free(modeinfo);
    return 0;
}


/* Unlock chipset-specific registers */

static void unlock(void)
{
    __svgalib_outcrtc(0x11,__svgalib_incrtc(0x11)&0x7f);
}

static void lock(void)
{
}

#define VENDOR_ID 0xf00
#define CARD_ID 0xba7

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    unsigned int buf[64];
    int found;
    
    found=__svgalib_pci_find_vendor_vga_pos(VENDOR_ID,buf);
    id=(buf[0]>>16)&0xffff;
    if(!found)return 0;
    switch(id) {
        case CARD_ID:
            init(0,0,0);
            return 1;
            break;
        default:
            return 0;
    }
}


/* Set display start address (not for 16 color modes) */
/* Cirrus supports any address in video memory (up to 2Mb) */

static void setdisplaystart(int address)
{ 
  address=address >> 2;
  __svgalib_outcrtc(0x0d,address&0xff);
  __svgalib_outcrtc(0x0c,(address>>8)&0xff);
}


/* Set logical scanline length (usually multiple of 8) */
/* Cirrus supports multiples of 8, up to 4088 */

static void setlogicalwidth(int width)
{   
    int offset = width >> 3;
 
    __svgalib_outcrtc(0x13,offset&0xff);
}

static int linear(int op, int param)
{
    if (op==LINEAR_ENABLE){ return 0;};
    if (op==LINEAR_DISABLE){ return 0;};
    if (op==LINEAR_QUERY_BASE) return linear_base;
    if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) 
		return 0;		/* No granularity or range. */
	return -1;		/* Unknown function. */
}

static int match_programmable_clock(int clock)
{
return clock ;
}

static int map_clock(int bpp, int clock)
{
return clock ;
}

static int map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}

/* Function table (exported) */

DriverSpecs __svgalib_driverspecs =
{
    saveregs,
    setregs,
    unlock,
    lock,
    test,
    init,
    setpage,
    NULL,
    NULL,
    setmode,
    modeavailable,
    setdisplaystart,
    setlogicalwidth,
    getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
};

/* Initialize chipset (called after detection) */

static int init(int force, int par1, int par2)
{
    unsigned int buf[64];
    int found=0;

    unlock();
    if (force) {
	memory = par1;
        chiptype = par2;
    } else {

    };

    found=__svgalib_pci_find_vendor_vga_pos(VENDOR_ID,buf);
    linear_base=0;
    id=(buf[0]>>16)&0xffff;
    if(found) {
        switch(id) {
            case CARD_ID:
                return 1;
                break;
            default:
                return 0;
        }
    }

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using SK driver, %iKB.\n",memory);
    };

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock4bpp = 75000;	
    cardspecs->maxPixelClock8bpp = 160000;	
    cardspecs->maxPixelClock16bpp = 160000;	
    cardspecs->maxPixelClock24bpp = 160000;
    cardspecs->maxPixelClock32bpp = 160000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 2040;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->nClocks =0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_driverspecs;
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=memory*0x400;
    return 0;
}
