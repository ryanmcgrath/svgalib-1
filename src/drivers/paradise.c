/* Western Digital Paradise WD90C31 driver for VGAlib                 */
/* (c) 1998 Petr Kulhavy   <brain@artax.karlin.mff.cuni.cz>           */
/*                                                                    */
/* This driver is absolutely free software. You can redistribute      */
/* and/or it modify without any restrictions. But it's WITHOUT ANY    */
/* WARRANTY.                                                          */

#include <stdio.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

#include "regs/paradise.regs"

/* static int paradise_chiptype; */
static int paradise_memory;	/* amount of video memory in K */

static int paradise_init(int, int, int);
static void paradise_unlock(void);

/* Mode table */
static ModeTable paradise_modes_512[] =
{				/* 512k, non-interlaced modes */
/* *INDENT-OFF* */
    OneModeEntry(640x480x256),
    OneModeEntry(800x600x16),
    OneModeEntry(800x600x256),
    OneModeEntry(1024x768x16),
    END_OF_MODE_TABLE
/* *INDENT-ON* */
};

/* Interlaced modes suck, I hate them */

static ModeTable *paradise_modes = NULL;

static void nothing(void)
{
}

static void inline _outb(unsigned port,unsigned value)
{
#ifdef DEBUG
 fprintf (stderr,"0x%x, 0x%x\n",port,value);
#endif
port_out_r(port,value);
}

/* Fill in chipset specific mode information */

static void paradise_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
#ifdef DEBUG
 fprintf(stderr,"paradise_getmodeinfo\n");
#endif
    switch(modeinfo->colors)
     {
     case 16:			/* 16 colors, 4 planes */
     modeinfo->maxpixels=65536*8;
     modeinfo->startaddressrange = 0x7ffff;
     break;
     default:
     if (modeinfo->bytesperpixel > 0)
 	modeinfo->maxpixels = paradise_memory * 1024 /
 	    modeinfo->bytesperpixel;
     else
 	modeinfo->maxpixels = paradise_memory * 1024;
     modeinfo->startaddressrange = 0x3ffff;
     break;
     }
    modeinfo->maxlogicalwidth = 2040;
    modeinfo->haveblit = 0;
    modeinfo->flags |= HAVE_RWPAGE;
}


/* select the correct register table */
static void setup_registers(void)
{
#ifdef DEBUG
 fprintf(stderr,"setup_registers\n");
#endif
    if (paradise_modes == NULL) {
	    paradise_modes = paradise_modes_512;
    }
}


/* Read and store chipset-specific registers */

static int paradise_saveregs(unsigned char regs[])
{
unsigned a,b;

#ifdef DEBUG
 fprintf(stderr,"paradise_saveregs\n");
#endif

paradise_unlock();
b=0;
for (a=0x03;a<=0x15;)
 { 
 port_out_r(SEQ_I,a);
 regs[EXT+b]=port_in(SEQ_D);
 b++;
 if (a==0x03)a=0x06;
 else if (a==0x07)a=0x10;
 else if (a==0x12)a=0x14;
 else a++;
 }
for (a=0x09;a<=0x0f;a++)
 {
 port_out_r(GRA_I,a);
 regs[EXT+b]=port_in(GRA_D);
 b++;
 }  
for (a=0x29;a<=0x3e;)
 {
 port_out_r(CRT_IC,a);
 regs[EXT+b]=port_in(CRT_DC);
 b++;
 if (a==0x30)a=0x32;
 else if (a==0x32)a=0x34;
 else if (a==0x34)a=0x3e;
 else a++;
 }  
/* VGA registers, not set by shitty svgalib */
port_out_r(CRT_IC,0x18);
regs[EXT+b+1]=port_in(CRT_DC);
port_out_r(ATT_IW,0x20);
__svgalib_delay();
regs[EXT+b+2]=port_in(ATT_IW);
return 28;			/* Paradise requires 28 additional registers */
}

static void paradise_unlock(void)
{
unsigned char b;
#ifdef DEBUG
 fprintf(stderr,"paradise_unlock\n");
#endif
port_outw_r(GRA_I,0x050f);
port_outw_r(CRT_IC,0x8529);
port_outw_r(SEQ_I,0x4806);
/* let's unlock sync, timing ... */
port_out_r(GRA_I,0xd);
b=port_in (GRA_D);
b&=30;
b|=2;
port_out_r(GRA_I,0xd);
port_out_r(GRA_D,b);
port_out_r(GRA_I,0xe);
b=port_in(GRA_D);
b&=251;
port_out_r(GRA_I,0xe);
port_out_r(GRA_D,b);
port_out_r(CRT_IC,0x2a);
b=port_in(CRT_DC);
b&=248;
port_out_r(CRT_IC,0x2a);
port_out_r(CRT_DC,b);
}

static void paradise_lock(void)
{
#ifdef DEBUG
 fprintf(stderr,"paradise_lock\n");
#endif
port_outw_r(GRA_I,0x000f);
port_outw_r(CRT_IC,0x0029);
port_outw_r(SEQ_I,0x0006);
}

/* Set chipset-specific registers */

static void paradise_setregs(const unsigned char regs[], int mode)
{
unsigned a,b;
#ifdef DEBUG
fprintf(stderr,"paradise_setregs\n");
#endif

paradise_unlock();

b=0;
for (a=0x03;a<=0x15;)
 { 
 _outb(SEQ_I,a);
 _outb(SEQ_D,regs[EXT+b]);
 b++;
 if (a==0x03)a=0x06;
 else if (a==0x07)a=0x10;
 else if (a==0x12)a=0x14;
 else a++;
 }
for (a=0x09;a<=0x0f;a++)
 {
 _outb(GRA_I,a);
 _outb(GRA_D,regs[EXT+b]);
 b++;
 }  
for (a=0x29;a<=0x3e;)
 {
 _outb(CRT_IC,a);
 _outb(CRT_DC,regs[EXT+b]);
 b++;
 if (a==0x30)a=0x32;
 else if (a==0x32)a=0x34;
 else if (a==0x34)a=0x3e;
 else a++;
 }  
/* VGA registers, not set by shitty svgalib */
_outb(CRT_IC,0x18);
_outb(CRT_DC,regs[EXT+b+1]);
_outb(0x3da,0x100);
_outb(ATT_IW,0x20);
__svgalib_delay();
_outb(ATT_IW,regs[EXT+b+2]);
}


/* Return nonzero if mode is available */

static int paradise_modeavailable(int mode)
{
    const unsigned char *regs;
    struct vgainfo *info;
#ifdef DEBUG
 fprintf(stderr,"paradise_modeavailable\n");
#endif

    regs = LOOKUPMODE(paradise_modes, mode);
    if (regs == NULL || mode == GPLANE16)
	return __svgalib_vga_driverspecs.modeavailable(mode);
    if (regs == DISABLE_MODE || mode <= TEXT || mode > GLASTMODE)
	return 0;

    info = &__svgalib_infotable[mode];
    if (paradise_memory * 1024 < info->ydim * info->xbytes)
	return 0;

    return SVGADRV;
}

/* Set a mode */

static int paradise_setmode(int mode, int prv_mode)
{
    const unsigned char *regs;

#ifdef DEBUG
 fprintf(stderr,"paradise_setmode\n");
#endif

    regs = LOOKUPMODE(paradise_modes, mode);
    if (regs == NULL)
	return (int) (__svgalib_vga_driverspecs.setmode(mode, prv_mode));
    if (!paradise_modeavailable(mode))
	return 1;
    paradise_unlock();
    __svgalib_setregs(regs);
    paradise_setregs(regs, mode);
    return 0;
}


/* Indentify chipset; return non-zero if detected */

static int paradise_test(void)
{
    /* 
     * Check first that we have a Paradise card.
     */
    int a;
    unsigned char old_value;
    unsigned char txt[6];
#ifdef DEBUG
 fprintf(stderr,"paradise_test\n");
#endif
    txt[5]=0;
    old_value=port_in(CRT_IC);
    for (a=0;a<5;a++)
     {
     port_out_r(CRT_IC,0x31+a);
     txt[a]=port_in(CRT_DC);
     }
    if (strcmp((char *)txt,"WD90C"))
    {
	port_out_r(CRT_IC,old_value);
	return 0;
    }
    /* check version */
    port_out_r(CRT_IC, 0x36);
    switch (port_in(CRT_DC))
    {
    case 0x32:  /* WD90C2x */
    port_out_r(CRT_IC, 0x37);
    switch (port_in(CRT_DC))
     {
     case 0x34:  /* WD90C24 */
     case 0x36:  /* WD90C26 */
     break;
     default:
     return 0; 
     }
    break;
    case 0x33:  /* WD90C3x */
    port_out_r(CRT_IC, 0x37);
    port_out_r(CRT_IC, 0x37);
    switch (port_in(CRT_DC))
     {
     case 0x30:  /* WD90C30 */
     case 0x31:  /* WD90C31 */
     case 0x33:  /* WD90C31 */
     break;
     default:
     return 0; 
     }
    break;
    default:
    return 0;
    }

    paradise_init(0, 0, 0);
    return 1;
}


/* Bank switching function - set 64K bank number */

static void paradise_setpage(int page)
{
#ifdef DEBUG
 fprintf(stderr,"paradise_setpage\n");
#endif
paradise_unlock();
/* set read-write paging mode */
port_out_r(SEQ_I,0x11);
port_out_r(SEQ_D,port_in(SEQ_D)|0x80);
port_out_r(GRA_I,0x0b);
port_out_r(GRA_D,port_in(GRA_D)|0x80);
/* set bank number */
port_out_r(GRA_I,0x09);
port_out_r(GRA_D,page<<4);
port_out_r(GRA_I,0x0a);
port_out_r(GRA_D,page<<4);
}


/* Set display start address (not for 16 color modes) */

static void paradise_setdisplaystart(int address)
{unsigned char bits,orig;
#ifdef DEBUG
 fprintf(stderr,"paradise_setdisplaystart\n");
#endif
paradise_unlock();
port_out_r(CRT_IC,0x2f);
bits=address>>13;
orig=port_in(CRT_DC);
orig^=bits;
orig&=0xe7;
orig^=bits;
port_out_r(CRT_DC,orig);
}


/* Set logical scanline length (usually multiple of 8) */

static void paradise_setlogicalwidth(int width)
{
#ifdef DEBUG
 fprintf(stderr,"paradise_setlogicalwidth\n");
#endif
paradise_unlock();
    port_outw_r(CRT_IC, 0x13 + (width >> 3) * 256);
}


/* Function table */
DriverSpecs __svgalib_paradise_driverspecs =
{
    paradise_saveregs,
    paradise_setregs,
    paradise_unlock,
    paradise_lock,
    paradise_test,
    paradise_init,
    paradise_setpage,
    (void (*)(int)) nothing,	/* __svgalib_setrdpage */
    (void (*)(int)) nothing,	/* __svgalib_setwrpage */
    paradise_setmode,
    paradise_modeavailable,
    paradise_setdisplaystart,
    paradise_setlogicalwidth,
    paradise_getmodeinfo,  /*?*/
    0,				/* bitblt */
    0,				/* imageblt */
    0,				/* fillblt */
    0,				/* hlinelistblt */
    0,				/* bltwait */
    0,				/* extset */
    0,
    0,				/* linear */
    NULL			/* accelspecs */
};


/* Initialize chipset (called after detection) */

static int paradise_init(int force, int par1, int par2)
{
#ifdef DEBUG
 fprintf(stderr,"paradise_init\n");
#endif
    if (force) {
#ifdef DEBUG
 fprintf(stderr,"forcing memory to %dkB\n",par1);
#endif
	paradise_memory = par1;
    } else {
	port_out_r(GRA_I,0x0b);
	paradise_memory=(port_in(GRA_D)>>6);
	switch (paradise_memory)
	 {
	 case 0:
	 case 1:
	 paradise_memory=256;
	 break;
	 case 2:
	 paradise_memory=512;
	 break;
	 case 3:
	 paradise_memory=1024;
	 break;
	 }
    }

    if (__svgalib_driver_report) {
	fprintf(stderr,"Using WD90C31 Paradise driver (%dK non-interlaced).\n",
	       paradise_memory);
    }
    __svgalib_driverspecs = &__svgalib_paradise_driverspecs;
    setup_registers(); 
    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    return 0;
}
