#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "fb_lnx.h"
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "timing.h"
#include "interface.h"

/* #define DEBUG */

static int fbdev_fd = -1;
static int fbdev_vgamode = 0;
static int fbdev_banked_pointer_emulated = 1;
static size_t fbdev_memory;
static size_t fbdev_startaddressrange;
static CardSpecs *cardspecs;
static struct console_font_op fbdev_font;
/* Card Specs */
static int fbdev_setpalette(int index, int red, int green, int blue);
static void fbdev_set_virtual_height(struct fb_var_screeninfo *info);
static int dacmode;

static int fbdev_match_programmable_clock(int clock)
{
	return clock;
}

static int fbdev_map_clock(int bpp, int clock)
{
	return clock;
}

static int fbdev_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
	return htiming;
}

/* Driver Specs */

static int fbdev_saveregs(unsigned char *regs)
{
	ioctl(fbdev_fd, FBIOGET_VSCREENINFO, regs+EXT);
	return sizeof(struct fb_var_screeninfo);
}

static void fbdev_setregs(const unsigned char *regs, int mode)
{
       ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, regs+EXT);
       fbdev_vgamode = 0;
}

static void fbdev_unlock(void)
{
}

static void fbdev_lock(void)
{
}

static int fbdev_init(int force, int par1, int par2)
{
	struct fb_fix_screeninfo info;
	int fd;

	if ((fd = open("/dev/fb0", O_RDWR)) < 0)
	{
		return -1;
	}

	if (ioctl(fd, FBIOGET_FSCREENINFO, &info))
	{
		close(fd);
		return -1;
	}

	/*  Ensure this file is closed if we ever exec something else...  */
	if (fcntl(fd, F_SETFD, 1) == -1) {
		perror("fcntl 007");
		exit(-1);
	}

	fbdev_memory = info.smem_len;
	fbdev_fd = fd;

	fbdev_startaddressrange = 65536;
	while (fbdev_startaddressrange < fbdev_memory)
	{
		fbdev_startaddressrange <<= 1;
	}
	fbdev_startaddressrange -= 1;
    
	__svgalib_modeinfo_linearset |= LINEAR_CAN;
	
	cardspecs = malloc(sizeof(CardSpecs));
	cardspecs->videoMemory = fbdev_memory/1024;

/* FIXME: autodetect these */
	cardspecs->maxPixelClock4bpp = 200000;
	cardspecs->maxPixelClock8bpp = 200000;
	cardspecs->maxPixelClock16bpp = 200000;
	cardspecs->maxPixelClock24bpp = 200000;
	cardspecs->maxPixelClock32bpp = 200000;
	cardspecs->maxHorizontalCrtc = 8192;

	cardspecs->flags = CLOCK_PROGRAMMABLE;

	cardspecs->nClocks = 0;
	cardspecs->clocks = 0;

	cardspecs->mapClock = fbdev_map_clock;
	cardspecs->matchProgrammableClock = fbdev_match_programmable_clock;
	cardspecs->mapHorizontalCrtc = fbdev_map_horizontal_crtc;

	__svgalib_driverspecs = &__svgalib_fbdev_driverspecs;
	__svgalib_banked_mem_base = (unsigned long)info.smem_start;
	__svgalib_banked_mem_size = 0x10000;
	__svgalib_linear_mem_base = 0;
	__svgalib_linear_mem_phys_addr = (unsigned long)info.smem_start;
	__svgalib_linear_mem_size = fbdev_memory;
	__svgalib_linear_mem_fd = fbdev_fd;

        dacmode = 6;
        
        fprintf(stderr,"Using fbdev, %iKB at %08x\n",(int)fbdev_memory/1024, (unsigned int)info.smem_start);

	return 0;
}

static int fbdev_test(void)
{
	return !fbdev_init(0, 0, 0);
}

static void fbdev_setpage(int page)
{
	if (fbdev_vgamode)
	{
		__svgalib_vga_driverspecs.__svgalib_setpage(page);
	}
	else
	{
		mmap(BANKED_POINTER,
			     __svgalib_banked_mem_size,
			     PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_FIXED,
			     fbdev_fd, page << 16);
		fbdev_banked_pointer_emulated = 1;
	}
}

static int fbdev_screeninfo(struct fb_var_screeninfo *info, int mode)
{
	ModeTiming modetiming;
	ModeInfo *modeinfo;

	if (ioctl(fbdev_fd, FBIOGET_VSCREENINFO, info))
		return 1;

	modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

	if (__svgalib_getmodetiming(&modetiming, modeinfo, cardspecs))
	{
		free(modeinfo);
		return 1;
	}

	info->xres = modeinfo->width;
	info->yres = modeinfo->height;
	info->xres_virtual = modeinfo->width;
	info->yres_virtual = modeinfo->height;
	info->xoffset = 0;
	info->yoffset = 0;
	info->bits_per_pixel = modeinfo->bitsPerPixel;
	info->grayscale = 0;
	info->red.offset = modeinfo->redOffset;
	info->red.length = modeinfo->redWeight;
	info->red.msb_right = 0;
	info->green.offset = modeinfo->greenOffset;
	info->green.length = modeinfo->greenWeight;
	info->green.msb_right = 0;
	info->blue.offset = modeinfo->blueOffset;
	info->blue.length = modeinfo->blueWeight;
	info->blue.msb_right = 0;
	info->nonstd = 0;
        
	free(modeinfo);

	info->vmode &= FB_VMODE_MASK;

	if (modetiming.flags & INTERLACED)
		info->vmode |= FB_VMODE_INTERLACED;
	if (modetiming.flags & DOUBLESCAN)
	{
		info->vmode |= FB_VMODE_DOUBLE;
		modetiming.VDisplay >>= 1;
		modetiming.VSyncStart >>= 1;
		modetiming.VSyncEnd >>= 1;
		modetiming.VTotal >>= 1;
	}

	info->pixclock = 1000000000 / modetiming.pixelClock;
	info->left_margin = modetiming.HTotal - modetiming.HSyncEnd;
	info->right_margin = modetiming.HSyncStart - modetiming.HDisplay;
	info->upper_margin = modetiming.VTotal - modetiming.VSyncEnd;
	info->lower_margin = modetiming.VSyncStart - modetiming.VDisplay;
	info->hsync_len = modetiming.HSyncEnd - modetiming.HSyncStart;
	info->vsync_len = modetiming.VSyncEnd - modetiming.VSyncStart;

	info->sync &= ~(FB_SYNC_HOR_HIGH_ACT|FB_SYNC_VERT_HIGH_ACT);

	if (modetiming.flags & PHSYNC)
		info->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (modetiming.flags & PVSYNC)
		info->sync |= FB_SYNC_VERT_HIGH_ACT;

	fbdev_set_virtual_height(info);

	return 0;
}

static void fbdev_set_virtual_height(struct fb_var_screeninfo *info)
{
	int maxpixels = fbdev_memory;
	int bytesperpixel = info->bits_per_pixel / 8;
 
	if (bytesperpixel)
		maxpixels /= bytesperpixel;

	info->yres_virtual = maxpixels / info->xres_virtual;
	if (info->yres_virtual > 8192)
	        info->yres_virtual = 8192;
}

static int fbdev_put_vscreeninfo(struct fb_var_screeninfo *info)
{
	if (!ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, info))
		return 0;

	/* try again with an yres virtual of 2x yres */
	info->yres_virtual = 2 * info->yres;
	if (!ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, info))
		return 0;

	/* last try, with an yres_virtual of yres */
	info->yres_virtual = info->yres;
	if (!ioctl(fbdev_fd, FBIOPUT_VSCREENINFO, info))
		return 0;

	/* we failed */
	return 1;
}

#ifdef DEBUG
static void print_vinfo(struct fb_var_screeninfo *vinfo)
{
        fprintf(stderr, "\txres: %d\n", vinfo->xres);
        fprintf(stderr, "\tyres: %d\n", vinfo->yres);
        fprintf(stderr, "\txres_virtual: %d\n", vinfo->xres_virtual);
        fprintf(stderr, "\tyres_virtual: %d\n", vinfo->yres_virtual);
        fprintf(stderr, "\txoffset: %d\n", vinfo->xoffset);
        fprintf(stderr, "\tyoffset: %d\n", vinfo->yoffset);
        fprintf(stderr, "\tbits_per_pixel: %d\n", vinfo->bits_per_pixel);
        fprintf(stderr, "\tgrayscale: %d\n", vinfo->grayscale);
        fprintf(stderr, "\tnonstd: %d\n", vinfo->nonstd);
        fprintf(stderr, "\tactivate: %d\n", vinfo->activate);
        fprintf(stderr, "\theight: %d\n", vinfo->height);
        fprintf(stderr, "\twidth: %d\n", vinfo->width);
        fprintf(stderr, "\taccel_flags: %d\n", vinfo->accel_flags);
        fprintf(stderr, "\tpixclock: %d\n", vinfo->pixclock);
        fprintf(stderr, "\tleft_margin: %d\n", vinfo->left_margin);  
        fprintf(stderr, "\tright_margin: %d\n", vinfo->right_margin);
        fprintf(stderr, "\tupper_margin: %d\n", vinfo->upper_margin);
        fprintf(stderr, "\tlower_margin: %d\n", vinfo->lower_margin);
        fprintf(stderr, "\thsync_len: %d\n", vinfo->hsync_len);
        fprintf(stderr, "\tvsync_len: %d\n", vinfo->vsync_len);
        fprintf(stderr, "\tsync: %d\n", vinfo->sync);
        fprintf(stderr, "\tvmode: %d\n", vinfo->vmode);
        fprintf(stderr, "\tred: %d/%d\n", vinfo->red.length, vinfo->red.offset);
        fprintf(stderr, "\tgreen: %d/%d\n", vinfo->green.length, vinfo->green.offset);
        fprintf(stderr, "\tblue: %d/%d\n", vinfo->blue.length, vinfo->blue.offset);
        fprintf(stderr, "\talpha: %d/%d\n", vinfo->transp.length, vinfo->transp.offset);
	fflush(stderr);
}
#endif

static int fbdev_setmode(int mode, int prv_mode)
{
	struct fb_var_screeninfo info;

	if (IS_IN_STANDARD_VGA_DRIVER(mode))
	{
		if (__svgalib_fbdev_novga)
			return 1;
		if (__svgalib_vga_driverspecs.setmode(mode, prv_mode))
			return 1;
		if (fbdev_banked_pointer_emulated)
		{
			__svgalib_banked_mem_base = 0xA0000;
			map_banked(MAP_FIXED);
			fbdev_banked_pointer_emulated=0;
		}
		fbdev_vgamode = 1;
		return 0;
	}
	
	if (!fbdev_banked_pointer_emulated)
	{
		__svgalib_banked_mem_base = __svgalib_linear_mem_phys_addr;
		map_banked(MAP_FIXED);
		fbdev_banked_pointer_emulated=1;
	}

	if (fbdev_screeninfo(&info, mode))
		return 1;

	info.activate = FB_ACTIVATE_NOW;
#ifdef DEBUG
        fprintf(stderr, "Vinfo: before putvinfo\n");
	print_vinfo(&info);
#endif
	if (fbdev_put_vscreeninfo(&info))
		return 1;
#ifdef DEBUG
        fprintf(stderr, "Vinfo: after putvinfo\n");
	print_vinfo(&info);
#endif

	fbdev_vgamode = 0;
        if(info.bits_per_pixel>16) {
           int i;
           dacmode = 8;
           for(i=0;i<256;i++)fbdev_setpalette(i,i,i,i);
        } else if(info.bits_per_pixel>8) {
           int i;
           dacmode = 8;
           for(i=0;i<256;i++)fbdev_setpalette(i,0,0,0);
           for(i=0;i<16;i++)fbdev_setpalette(i,i<<3,i<<3,i<<3);
        } else {
           dacmode = 6;
        }
	return 0;
}

static int fbdev_modeavailable(int mode)
{
	struct fb_var_screeninfo info;
	unsigned g, bpp;

	if (IS_IN_STANDARD_VGA_DRIVER(mode))
	{
		if (__svgalib_fbdev_novga)
			return 0;
		else
		return __svgalib_vga_driverspecs.modeavailable(mode);
	}

	if (fbdev_screeninfo(&info, mode))
		return 0;

	g = info.green.length;
        bpp = info.bits_per_pixel;

	info.activate = FB_ACTIVATE_TEST;
	if (fbdev_put_vscreeninfo(&info))
		return 0;

        if (info.bits_per_pixel != bpp )
                return 0;

	if (info.bits_per_pixel == 16 &&
	    info.green.length != g)
		return 0;

	/* We may need to add more checks here. */

	return SVGADRV;
}

static void fbdev_setdisplaystart(int address)
{
	struct fb_var_screeninfo info;

	if (fbdev_vgamode)
	{
		__svgalib_vga_driverspecs.setdisplaystart(address);
		return;
	}

	if (ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &info))
		return;

	info.xoffset = address % info.xres_virtual;
	info.yoffset = address / info.xres_virtual;

	ioctl(fbdev_fd, FBIOPAN_DISPLAY, &info);
}

static void fbdev_setlogicalwidth(int width)
{
	struct fb_var_screeninfo info;

	if (fbdev_vgamode)
	{
		__svgalib_vga_driverspecs.setlogicalwidth(width);
		return;
	}

	if (ioctl(fbdev_fd, FBIOGET_VSCREENINFO, &info))
		return;

	info.xres_virtual = width;
	fbdev_set_virtual_height(&info);
	fbdev_put_vscreeninfo(&info);
}

static void fbdev_getmodeinfo(int mode, vga_modeinfo * modeinfo)
{
	struct fb_var_screeninfo info;
	int maxpixels = fbdev_memory;

	if (IS_IN_STANDARD_VGA_DRIVER(mode))
		return __svgalib_vga_driverspecs.getmodeinfo(mode, modeinfo);

	if (modeinfo->bytesperpixel)
		maxpixels /= modeinfo->bytesperpixel;

	modeinfo->maxlogicalwidth = maxpixels / modeinfo->height;
	modeinfo->startaddressrange = fbdev_startaddressrange;
	modeinfo->maxpixels = maxpixels;
	modeinfo->haveblit = 0;
	modeinfo->flags |= (__svgalib_modeinfo_linearset | CAPABLE_LINEAR); 

	if (fbdev_screeninfo(&info, mode))
		return;

	info.activate = FB_ACTIVATE_TEST;
	if (fbdev_put_vscreeninfo(&info))
		return;

	modeinfo->linewidth = info.xres_virtual * info.bits_per_pixel / 8;
	modeinfo->maxpixels = info.xres_virtual * info.yres_virtual;
}

static int fbdev_linear(int op, int param)
{
	switch(op)
	{
	case LINEAR_QUERY_BASE:
		return __svgalib_linear_mem_phys_addr;
	case LINEAR_ENABLE:
	case LINEAR_DISABLE:
		return 0;
	}
	return -1;
}

/* Emulation */

static void fbdev_savepalette(unsigned char *red,
			unsigned char *green,
			unsigned char *blue)
{
	uint16_t r[256], g[256], b[256], t[256];
	struct fb_cmap cmap;
	unsigned i;

	if (fbdev_vgamode)
	{
		for (i = 0; i < 256; i++) {
			int r,g,b;
			__svgalib_inpal(i,&r,&g,&b);
			*(red++) = r;
			*(green++) = g;
			*(blue++) = b;
		}
		return;
	}

	cmap.start = 0;
	cmap.len = 256;
	cmap.red = r;
	cmap.green = g;
	cmap.blue = b;
	cmap.transp = t;

	if (ioctl(fbdev_fd, FBIOGETCMAP, &cmap))
		return;

	for (i = 0; i < 256; i++)
	{
		red[i] = r[i] >> 10;
		green[i] = g[i] >> 10;
		blue[i] = b[i] >> 10;
	}
}

static void fbdev_restorepalette(const unsigned char *red,
			   const unsigned char *green,
			   const unsigned char *blue)
{
	uint16_t r[256], g[256], b[256], t[256];
	struct fb_cmap cmap;
	unsigned i;

	if (fbdev_vgamode)
	{
		for (i = 0; i < 256; i++) {
			__svgalib_outpal(i,*(red++),*(green++),*(blue++));
		}
		return;
	}

	for (i = 0; i < 256; i++)
	{
		r[i] = (red[i] << 10) | (red[i] << 4) | (red[i] >> 2);
		g[i] = (green[i] << 10) | (green[i] << 4) | (green[i] >> 2);
		b[i] = (blue[i] << 10) | (blue[i] << 4) | (blue[i] >> 2);
		t[i] = 0;
	}

	cmap.start = 0;
	cmap.len = 256;
	cmap.red = r;
	cmap.green = g;
	cmap.blue = b;
	cmap.transp = t;

	ioctl(fbdev_fd, FBIOPUTCMAP, &cmap);
}

static int fbdev_setpalette(int index, int red, int green, int blue)
{
	uint16_t r, g, b, t;
	struct fb_cmap cmap;

	if (fbdev_vgamode)
	{
		__svgalib_outpal(index,red,green,blue);
		return 0;
	}     

        if(dacmode==8) {
               r=red<<8;
               g=green<<8;
               b=blue<<8;
        } else {
	       r = (red << 10) | (red << 4) | (red >> 2);
               g = (green << 10) | (green << 4) | (green >> 2);
               b = (blue << 10) | (blue << 4) | (blue >> 2);
        }
	t = 0;

	cmap.start = index;
	cmap.len = 1;
	cmap.red = &r;
	cmap.green = &g;
	cmap.blue = &b;
	cmap.transp = &t;
	ioctl(fbdev_fd, FBIOPUTCMAP, &cmap);

	return 0;
}

static void fbdev_getpalette(int index, int *red, int *green, int *blue)
{
	uint16_t r, g, b, t;
	struct fb_cmap cmap;

	if (fbdev_vgamode)
	{
		__svgalib_inpal(index,red,green,blue);
		return;
	}

	cmap.start = 0;
	cmap.len = 1;
	cmap.red = &r;
	cmap.green = &g;
	cmap.blue = &b;
	cmap.transp = &t;

	if (ioctl(fbdev_fd, FBIOGETCMAP, &cmap))
		return;

	*red = r >> 10;
	*green = g >> 10;
	*blue = b >> 10;
}

static void fbdev_savefont(void)
{
	fbdev_font.op = KD_FONT_OP_GET;
	fbdev_font.flags = 0;
	fbdev_font.width = 32;
	fbdev_font.height = 32;
	fbdev_font.charcount = 512;
	fbdev_font.data = malloc(65536);

	ioctl(fbdev_fd, KDFONTOP, &fbdev_font);
}

static void fbdev_restorefont(void)
{
	fbdev_font.op = KD_FONT_OP_SET;

	ioctl(fbdev_fd, KDFONTOP, &fbdev_font);
}

/* Function tables */

static Emulation fbdev_vgaemul = 
{
	fbdev_savepalette,
	fbdev_restorepalette,
	fbdev_setpalette,
	fbdev_getpalette,
	fbdev_savefont,
	fbdev_restorefont,
	0,				/* screenoff */
	0,				/* screenon */
	0,				/* waitretrace */
};

DriverSpecs __svgalib_fbdev_driverspecs =
{
	fbdev_saveregs,
	fbdev_setregs,
	fbdev_unlock,
	fbdev_lock,
	fbdev_test,
	fbdev_init,
	fbdev_setpage,
	0,				/* setrdpage */
	0,				/* setwrpage */
	fbdev_setmode,
	fbdev_modeavailable,
	fbdev_setdisplaystart,
	fbdev_setlogicalwidth,
	fbdev_getmodeinfo,
	0,				/* bitblt */
	0,				/* imageblt */
	0,				/* fillblt */
	0,				/* hlinelistblt */
	0,				/* bltwait */
	0,				/* extset */
	0,				/* accel */
	fbdev_linear,
	0,				/* Accelspecs */
	&fbdev_vgaemul,
	0				/* cursor */
};

