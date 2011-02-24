/*
Matrox Mystique(1064)/G100/G200/G400/G450 chipset driver 

Based on the XFree86 (4.1.0) mga driver.

Tested only on G450 and Mystique. 


*/

#include <stdlib.h>
#include <stdio.h>		
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "timing.h"
#include "vgaregs.h"
#include "interface.h"
#include "vgapci.h"
#include "mga.h"
#define SECONDCRTC 1
#include "mga_g450pll.c"
#include "vgammvgaio.h"

#define SKREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define TOTAL_REGS (100)

static int init(int, int, int);
static void unlock(void);
static void lock(void);

static int memory, pciposition;
static unsigned int linear_base, mmio_base;

static CardSpecs *cardspecs;

static int inDAC(int i) {
    *(MMIO_POINTER + 0x3c00) = i;
    return *(MMIO_POINTER + 0x3c0a);
}

static void outDAC(int i, int d) {
    *(MMIO_POINTER + 0x3c00) = i;
    *(MMIO_POINTER + 0x3c0a) = d;
}

/* Fill in chipset specific mode information */
static void getmodeinfo(int mode, vga_modeinfo *modeinfo)
{

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 8184;
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
    unsigned int *iregs=(unsigned int *)regs;
    unlock();		

    for(i=0;i<0x22;i++) regs[i]=inDAC(i+0x80);

    iregs[9]=INREG(MGAREG_C2CTL);
    iregs[10]=INREG(MGAREG_C2DATACTL);
    iregs[11]=INREG(MGAREG_C2HPARAM);
    iregs[12]=INREG(MGAREG_C2HSYNC);
    iregs[13]=INREG(MGAREG_C2VPARAM);
    iregs[14]=INREG(MGAREG_C2VSYNC);
    iregs[15]=INREG(MGAREG_C2OFFSET);
    iregs[16]=INREG(MGAREG_C2STARTADD0);

    return 40;
}

/* Set chipset-specific registers */

static void setregs(const unsigned char regs[], int mode)
{  
    int i;
    unsigned int *iregs=(unsigned int *)regs;
    unlock();		

    for(i=0;i<0x22;i++) outDAC(i+0x80,regs[i]);

    OUTREG(MGAREG_C2CTL,iregs[9]);
    OUTREG(MGAREG_C2DATACTL,iregs[10]);
    OUTREG(MGAREG_C2HPARAM,iregs[11]);
    OUTREG(MGAREG_C2HSYNC,iregs[12]);
    OUTREG(MGAREG_C2VPARAM,iregs[13]);
    OUTREG(MGAREG_C2VSYNC,iregs[14]);
    OUTREG(MGAREG_C2OFFSET,iregs[15]);
    OUTREG(MGAREG_C2STARTADD0,iregs[16]);

}


/* Return nonzero if mode is available */

static int modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;

    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return 0;
   
    info = &__svgalib_infotable[mode];

    if((info->colors==16) || (info->colors==256))return 0;

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

static void initializemode(unsigned char *moderegs,
			    ModeTiming * modetiming, ModeInfo * modeinfo, int mode)
{ 
    unsigned int *iregs=(unsigned int *)moderegs;
    
    float f_out;


    G450SetPLLFreq(f_out);

    return ;
}


static int setmode(int mode, int prv_mode)
{
    unsigned char *moderegs;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;
    int i;

    if (!modeavailable(mode))
	return 1;

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
	free(modetiming);
	free(modeinfo);
	return 1;
    }

    moderegs = malloc(TOTAL_REGS);

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
}

static void lock(void)
{
}


#define VENDOR_ID 0x102b

/* Indentify chipset, initialize and return non-zero if detected */

static int test(void)
{
    int found, id;
    unsigned int buf[64];
    
    found=__svgalib_pci_find_vendor_vga_pos(VENDOR_ID,buf);
    
    if(!found) return 0;
    
    id=(buf[0]>>16)&0xffff;
    
    if((id==0x51a)||(id==0x51e)||(id==0x520)||(id==0x521)||(id==0x525)||(id==0x1000)||(id==0x1001)){
       init(0,0,0);
       return 1;
    };
    return 0;
}


/* Set display start address (not for 16 color modes) */

static void setdisplaystart(int address)
{ 
}


/* Set logical scanline length (usually multiple of 8) */

static void setlogicalwidth(int width)
{   
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

DriverSpecs __svgalib_g450c2_driverspecs =
{
    saveregs,
    setregs,
    unlock,
    lock,
    test,
    init,
    NULL,
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
    NULL,
};

/* Initialize chipset (called after detection) */

static int init(int force, int par1, int par2)
{
    unsigned int buf[64];
    int found=0;
    int pci_id;
    int max_mem = 8;

    if (force) {
	memory = par1;
    } else {
	memory = 0;
    };

    found=__svgalib_pci_find_vendor_vga_pos(VENDOR_ID,buf);
    
    if(!found) {
        fprintf(stderr,"Error: Must use Matrox driver, but no card found\n");
        exit(1);
    }

    pciposition=found;
    pci_id=(buf[0]>>16)&0xffff;
	
    switch(pci_id) {
        case 0x525:
            if((buf[11]&0xffff0000) != 0x07c00000) exit(1);
            break;
	default:
            exit(1);
    }

    linear_base = buf[4]&0xffffff00;
    mmio_base = buf[5]&0xffffff00;
        
    if(!memory) {
        memory=2048;
    }
    
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using Matrox G450 CRTC2 driver, %iKB RAM.\n", memory);
    };

	__svgalib_modeinfo_linearset |= LINEAR_CAN;

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = memory;
    cardspecs->maxPixelClock4bpp = 0;
    cardspecs->maxPixelClock8bpp = 0;
    cardspecs->maxPixelClock16bpp = 250000;
    cardspecs->maxPixelClock24bpp = 250000;
    cardspecs->maxPixelClock32bpp = 250000;
    cardspecs->flags = INTERLACE_DIVIDE_VERT | CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4095;
    cardspecs->nClocks =0;
    cardspecs->mapClock = map_clock;
    cardspecs->mapHorizontalCrtc = map_horizontal_crtc;
    cardspecs->matchProgrammableClock=match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_g450c2_driverspecs;
    __svgalib_linear_mem_base=linear_base;
    __svgalib_linear_mem_size=0;
    __svgalib_mmio_base=mmio_base;
    __svgalib_mmio_size=16384;
    __svgalib_novga=1;
    __svgalib_emulatepage=2;
    return 0;
}
