/*
*/

#include <stdlib.h>
#include <stdio.h>		/* for printf */
#include <string.h>		/* for memset */
#include <unistd.h>
#include <sys/mman.h>		/* for mmap */
#include <asm/vm86.h>
#include "lrmi.h"
#include "libvga.h"
#include "driver.h"

/* New style driver interface. */
#include "timing.h"
#include "vgaregs.h"
#include "interface.h"
#include "vbe.h"
#define VESAREG_SAVE(i) (VGA_TOTAL_REGS+i)
#define VESA_TOTAL_REGS (VGA_TOTAL_REGS + 4024)

/* #define VESA_savebitmap 0x0e */
int __svgalib_VESA_savebitmap=0x0e;
int __svgalib_VESA_textmode=3;

static int vesa_init(int, int, int);
static void vesa_unlock(void);

static int vesa_memory,vesa_chiptype;
static int vesa_is_linear, vesa_logical_width, vesa_bpp, vesa_granularity;
static int vesa_regs_size, vesa_linear_base, vesa_last_mode_set;
static struct LRMI_regs vesa_r;
static int vesa_read_write, vesa_read_window, vesa_write_window;
static void * LRMI_mem1, * LRMI_mem2;

static CardSpecs *cardspecs;
static struct
	{
	struct vbe_info_block *info;
	struct vbe_mode_info_block *mode;
	} vesa_data;

static int SVGALIB_VESA[__GLASTMODE+1];

static struct vbe_crtc_info_block *vesa_crtc;

static void vesa_setpage(int page)
{
vesa_r.eax=0x4f05;
vesa_r.ebx=0;
vesa_r.edx=page*64/vesa_granularity;
__svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);

if(vesa_read_write){
   vesa_r.eax=0x4f05;
   vesa_r.ebx=1;
   vesa_r.edx=page*64/vesa_granularity;
   __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
};
}

/* Fill in chipset specific mode information */

static void vesa_getmodeinfo(int mode, vga_modeinfo *modeinfo)
{
    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.getmodeinfo(mode, modeinfo);

    if(modeinfo->colors==16)return;

    modeinfo->maxpixels = vesa_memory*1024/modeinfo->bytesperpixel;
    modeinfo->maxlogicalwidth = 4088; /* just a guess, */
    modeinfo->startaddressrange = vesa_memory * 1024 - 1;
    modeinfo->haveblit = 0;
    modeinfo->flags &= ~HAVE_RWPAGE;
    modeinfo->flags |= vesa_read_write;  /* sets HAVE_RWPAGE bit */

    /* for linear need VBE2 */
    if((vesa_chiptype>=1) && (modeinfo->bytesperpixel >= 1)) {
		modeinfo->flags |= CAPABLE_LINEAR;
	}
      
    /* to get the logical scanline width */
    memset(&vesa_r, 0, sizeof(vesa_r));

    vesa_r.eax = 0x4f01;
    vesa_r.ecx = SVGALIB_VESA[mode];
    vesa_r.es = (unsigned int)vesa_data.mode >> 4;
    vesa_r.edi = (unsigned int)vesa_data.mode & 0xf;

    if (!__svgalib_LRMI_callbacks->rm_int(0x10, &vesa_r)) {
       fprintf(stderr, "Can't get mode info (vm86 failure)\n");
       return;
    }
    modeinfo->linewidth = vesa_data.mode->bytes_per_scanline;
}

/* Read and save chipset-specific registers */
static int vesa_saveregs(uint8_t regs[])
{ 
  void * buf;
  buf=LRMI_mem1;
  vesa_r.eax=0x4f04;
  vesa_r.ebx=0;
  vesa_r.es=((long)buf)>>4;
  vesa_r.edx=1;
  vesa_r.ecx=__svgalib_VESA_savebitmap;
  __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
  memcpy(&regs[VGA_TOTAL_REGS],buf,vesa_regs_size);  
  
  return vesa_regs_size;
}

/* Set chipset-specific registers */

static void vesa_setregs(const uint8_t regs[], int mode)
{   

  void * buf;
  buf=LRMI_mem1;
  memcpy(buf,&regs[VGA_TOTAL_REGS],vesa_regs_size);  
  vesa_r.eax=0x4f04;
  vesa_r.ebx=0;
  vesa_r.es=((long)buf)>>4;
  vesa_r.edx=2;
  vesa_r.ecx=__svgalib_VESA_savebitmap;
  __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
}


/* Return nonzero if mode is available */

static int vesa_modeavailable(int mode)
{
    struct vgainfo *info;
    ModeTiming *modetiming;
    ModeInfo *modeinfo;


    if (IS_IN_STANDARD_VGA_DRIVER(mode))
	return __svgalib_vga_driverspecs.modeavailable(mode);

    info = &__svgalib_infotable[mode];

    modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);

    modetiming = malloc(sizeof(ModeTiming));
    if (__svgalib_getmodetiming(modetiming, modeinfo, cardspecs)) {
		free(modetiming);
		free(modeinfo);
		return 0;
    }
    free(modetiming);
    free(modeinfo);

return SVGALIB_VESA[mode];
}

/* Set CRTC info related registers when setting video mode */
static void vesa_set_crtc_info_regs( void )
{
	if (vesa_chiptype >= 2)
	{
		vesa_r.ebx |= 0x0800;
		vesa_r.es = (unsigned int)vesa_crtc >> 4;
		vesa_r.edi = (unsigned int)vesa_crtc & 0xf;
	}
}



static int vesa_setmode(int mode, int prv_mode)
{
    vesa_bpp=1;
    vesa_granularity=1;
    if (IS_IN_STANDARD_VGA_DRIVER(mode)) {

        if(__svgalib_vesatext){
            vesa_r.eax=0x4f02; /* make sure we are in a regular VGA mode before we start */
            vesa_r.ebx=__svgalib_VESA_textmode;    /* without this, if we start in SVGA mode the result might */
            __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r); /* be something weird */
        };
	return __svgalib_vga_driverspecs.setmode(mode, prv_mode);
    }
    if (!vesa_modeavailable(mode)) return 1;
    vesa_r.eax=0x4f02;
    vesa_r.ebx=SVGALIB_VESA[mode]|0x8000|(vesa_is_linear*0x4000);

	/* Set timing information */
	if (vesa_chiptype >= 2)
	{
		ModeTiming *modetiming;
		ModeInfo *modeinfo;

		/* Get mode timing */
    	modeinfo = __svgalib_createModeInfoStructureForSvgalibMode(mode);
		modetiming = malloc(sizeof(ModeTiming));
		if (!__svgalib_getmodetiming(modetiming, modeinfo, cardspecs))
		{
			/* Fill VBE Timing structure */
			memset(vesa_crtc, 0, sizeof(*vesa_crtc));
			vesa_crtc->horiz_start = modetiming->HSyncStart;
			vesa_crtc->horiz_end = modetiming->HSyncEnd;
			vesa_crtc->horiz_total = modetiming->HTotal;
			vesa_crtc->vert_start = modetiming->VSyncStart;
			vesa_crtc->vert_end = modetiming->VSyncEnd;
			vesa_crtc->vert_total = modetiming->VTotal;
			vesa_crtc->pixel_clock = modetiming->pixelClock * 1000;
			vesa_crtc->refresh_rate = (unsigned short)
				(100 * (vesa_crtc->pixel_clock / 
				(vesa_crtc->vert_total * vesa_crtc->horiz_total)));
		}
		free(modetiming);
		free(modeinfo);
	}
	vesa_set_crtc_info_regs();
    vesa_last_mode_set=vesa_r.ebx;
    __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);

    vesa_data.info = LRMI_mem2 ;
    vesa_data.mode = (struct vbe_mode_info_block *)(vesa_data.info + 1);
    vesa_r.eax = 0x4f01;
    vesa_r.ecx=SVGALIB_VESA[mode];
    vesa_r.es = (unsigned int)vesa_data.mode >> 4;
    vesa_r.edi = (unsigned int)vesa_data.mode&0xf;    
    __svgalib_LRMI_callbacks->rm_int(0x10, &vesa_r);
    vesa_logical_width=vesa_data.mode->bytes_per_scanline;
    vesa_bpp=(vesa_data.mode->bits_per_pixel+7)/8;
    if(vesa_logical_width==0) vesa_logical_width=vesa_bpp*vesa_data.mode->x_resolution;
                                        	/* if not reported then guess */
    vesa_granularity=vesa_data.mode->win_granularity;
    if(vesa_granularity==0)vesa_granularity=64; /* if not reported then guess */
    if(vesa_chiptype>=1)vesa_linear_base=vesa_data.mode->phys_base_ptr;
    vesa_read_write=0;
    vesa_read_window=0;
    vesa_write_window=0;
    if((vesa_data.mode->win_a_attributes&6)!=6){ 
        vesa_read_write=1;
        if ((vesa_data.mode->win_b_attributes&2) == 2) vesa_read_window=1;
        if ((vesa_data.mode->win_b_attributes&4) == 4) vesa_write_window=1;
    }
    return 0;
}


/* Unlock chipset-specific registers */

static void vesa_unlock(void)
{
}


/* Relock chipset-specific registers */
/* (currently not used) */

static void vesa_lock(void)
{
}


/* Indentify chipset, initialize and return non-zero if detected */

static int vesa_test(void)
{
        __svgalib_LRMI_callbacks->rm_init();
        LRMI_mem2 = __svgalib_LRMI_callbacks->rm_alloc_real(sizeof(struct vbe_info_block)
                                        + sizeof(struct vbe_mode_info_block));
	vesa_data.info = LRMI_mem2;
	vesa_data.mode = (struct vbe_mode_info_block *)(vesa_data.info + 1);
	vesa_r.eax = 0x4f00;
	vesa_r.es = (unsigned int)vesa_data.info >> 4;
	vesa_r.edi = 0;

        __svgalib_LRMI_callbacks->rm_free_real(LRMI_mem2);

        __svgalib_LRMI_callbacks->rm_int(0x10, &vesa_r);
        if (vesa_r.eax!=0x4f) return 0;
        return !vesa_init(0,0,0);
}


/* No r/w paging */
static void vesa_setrdpage(int page)
{
vesa_r.eax=0x4f05;
vesa_r.ebx=vesa_read_window;
vesa_r.edx=page*64/vesa_granularity;
__svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
}
static void vesa_setwrpage(int page)
{
vesa_r.eax=0x4f05;
vesa_r.ebx=vesa_write_window;
vesa_r.edx=page*64/vesa_granularity;
__svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
}


/* Set display start address (not for 16 color modes) */

static void vesa_setdisplaystart(int address)
{
  vesa_r.eax=0x4f07;
  vesa_r.ebx=0;
  vesa_r.ecx=address % vesa_logical_width;
  vesa_r.edx=address / vesa_logical_width;

  __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);

}

/* Set logical scanline length (usually multiple of 8) */

static void vesa_setlogicalwidth(int width)
{
  vesa_r.eax=0x4f06;
  vesa_r.ebx=0;
  vesa_r.ecx=width / vesa_bpp ;
  __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
  vesa_logical_width=vesa_r.ebx;

}

static int vesa_linear(int op, int param)
{
if (op==LINEAR_ENABLE) {  
  vesa_r.eax=0x4f02;
  vesa_r.ebx=vesa_last_mode_set|0x4000;
  vesa_set_crtc_info_regs();
  __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
  vesa_is_linear=1;
};

if (op==LINEAR_DISABLE){ 
  vesa_r.eax=0x4f02;
  vesa_r.ebx=vesa_last_mode_set;
  vesa_set_crtc_info_regs();
  __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
  vesa_is_linear=0;
};
if (op==LINEAR_QUERY_BASE) {return vesa_linear_base ;}
if (op == LINEAR_QUERY_RANGE || op == LINEAR_QUERY_GRANULARITY) return 0;		/* No granularity or range. */
    else return -1;		/* Unknown function. */
}

static int vesa_match_programmable_clock(int clock)
{
return clock ;
}
static int vesa_map_clock(int bpp, int clock)
{
return clock ;
}
static int vesa_map_horizontal_crtc(int bpp, int pixelclock, int htiming)
{
return htiming;
}
/* Function table (exported) */

DriverSpecs __svgalib_vesa_driverspecs =
{
    vesa_saveregs,
    vesa_setregs,
    vesa_unlock,
    vesa_lock,
    vesa_test,
    vesa_init,
    vesa_setpage,
    vesa_setrdpage,
    vesa_setwrpage,
    vesa_setmode,
    vesa_modeavailable,
    vesa_setdisplaystart,
    vesa_setlogicalwidth,
    vesa_getmodeinfo,
    0,				/* old blit funcs */
    0,
    0,
    0,
    0,
    0,				/* ext_set */
    0,				/* accel */
    vesa_linear,
    0,				/* accelspecs, filled in during init. */
    NULL,                       /* Emulation */
};

/* Initialize chipset (called after detection) */

static int vesa_init(int force, int par1, int par2)
{  
    short int *mode_list;
    int i;
#if 0
    uint8_t *m;
    int address;

    m=mmap(0,0x502,PROT_READ,MAP_SHARED,__svgalib_mem_fd,0);
    
    address=((int)*(unsigned short *)(m+64))+(((int)*(unsigned short *)(m+66))<<4);
    if((address<0xa0000)||(address>0xfffff)) {
       	fprintf(stderr, "Real mode int 0x10 at 0x%08x handler not in ROM.\n",address);
        fprintf(stderr, "Try running vga_reset.\n");
       	return 1;
    };
#endif
    __svgalib_textprog|=1;
	vesa_bpp=1;
	vesa_granularity=1;

    /* Get I/O privilege */

    if (force) {
	vesa_memory = par1;
	vesa_chiptype = par2;
    } else {
        vesa_memory=4096; 
    };

    __svgalib_LRMI_callbacks->rm_init();
    for(i=0;i<__GLASTMODE;i++)SVGALIB_VESA[i]=IS_IN_STANDARD_VGA_DRIVER(i);

    vesa_data.info = __svgalib_LRMI_callbacks->rm_alloc_real(sizeof(struct vbe_info_block)
	 + sizeof(struct vbe_mode_info_block));
    vesa_data.mode = (struct vbe_mode_info_block *)(vesa_data.info + 1);
    vesa_r.eax = 0x4f00;
    vesa_r.es = (unsigned int)vesa_data.info >> 4;
    vesa_r.edi = 0;
    
    memcpy(vesa_data.info->vbe_signature, "VBE2", 4);

    __svgalib_LRMI_callbacks->rm_int(0x10, &vesa_r);
    
    if ((vesa_r.eax & 0xffff) != 0x4f || strncmp(vesa_data.info->vbe_signature, "VESA", 4) != 0) {
       	fprintf(stderr,"No VESA bios detected!\n");
        fprintf(stderr,"Try running vga_reset.\n");
       	return 1;
    }
    
    if(vesa_data.info->vbe_version>=0x0200)vesa_chiptype=1 ; else vesa_chiptype=0;
    if(vesa_data.info->vbe_version>=0x0300)vesa_chiptype=2 ; 

    vesa_memory = vesa_data.info->total_memory*64;

    mode_list = (short int *)(vesa_data.info->video_mode_list_seg * 16 + vesa_data.info->video_mode_list_off);

    while (*mode_list != -1) {
       memset(&vesa_r, 0, sizeof(vesa_r));

       vesa_r.eax = 0x4f01;
       vesa_r.ecx = *mode_list;
       vesa_r.es = (unsigned int)vesa_data.mode >> 4;
       vesa_r.edi = (unsigned int)vesa_data.mode & 0xf;

       if (!__svgalib_LRMI_callbacks->rm_int(0x10, &vesa_r)) {
          fprintf(stderr, "Can't get mode info (vm86 failure)\n");
          return 1;
       }
       if((vesa_chiptype>=1)&&(vesa_data.mode->mode_attributes&0x80))
          vesa_linear_base=vesa_data.mode->phys_base_ptr;
#if 1
       for(i=0;i<=__GLASTMODE;i++)
          if((infotable[i].xdim==vesa_data.mode->x_resolution)&&
             (infotable[i].ydim==vesa_data.mode->y_resolution)&&
             (((vesa_data.mode->rsvd_mask_size==8)&&(infotable[i].bytesperpixel==4))||
              ((vesa_data.mode->bits_per_pixel==32)&&(infotable[i].bytesperpixel==4))||
              ((vesa_data.mode->bits_per_pixel==24)&&(infotable[i].bytesperpixel==3))||
              ((vesa_data.mode->green_mask_size==5)&&(infotable[i].colors==32768))||
              ((vesa_data.mode->green_mask_size==6)&&(infotable[i].colors==65536))||
              ((vesa_data.mode->memory_model==VBE_MODEL_PLANAR)&&(infotable[i].colors==16))||
              ((vesa_data.mode->memory_model==VBE_MODEL_256)&&(infotable[i].colors==256))||
              ((vesa_data.mode->memory_model==VBE_MODEL_PACKED)&&
               (infotable[i].colors==256)&&(vesa_data.mode->bits_per_pixel==8)))){
                  	SVGALIB_VESA[i]=*mode_list;
                        i=__GLASTMODE+1;
          };

#else
       if (vesa_data.mode->memory_model == VBE_MODEL_RGB) {  
          if((vesa_data.mode->rsvd_mask_size==8)||(vesa_data.mode->bits_per_pixel==32)) {  
             switch(vesa_data.mode->y_resolution){
                case 200: if(vesa_data.mode->x_resolution==320)
                   SVGALIB_VESA[G320x200x16M32]=*mode_list; break;
                case 240: if(vesa_data.mode->x_resolution==320)
                   SVGALIB_VESA[G320x240x16M32]=*mode_list; break;
                case 300: if(vesa_data.mode->x_resolution==400)
                   SVGALIB_VESA[G400x300x16M32]=*mode_list; break;
                case 384: if(vesa_data.mode->x_resolution==512)
                   SVGALIB_VESA[G512x384x16M32]=*mode_list; break;
                case 480: if(vesa_data.mode->x_resolution==640)
                   SVGALIB_VESA[G640x480x16M32]=*mode_list; break;
                case 600: if(vesa_data.mode->x_resolution==800)
                   SVGALIB_VESA[G800x600x16M32]=*mode_list; break;
                case 720: if(vesa_data.mode->x_resolution==960)
                   SVGALIB_VESA[G960x720x16M32]=*mode_list; break;
                case 768: if(vesa_data.mode->x_resolution==1024)
                   SVGALIB_VESA[G1024x768x16M32]=*mode_list; break;
                case 864: if(vesa_data.mode->x_resolution==1152)
                    SVGALIB_VESA[G1152x864x16M32]=*mode_list; break;
                case 1024: if(vesa_data.mode->x_resolution==1280)
                    SVGALIB_VESA[G1280x1024x16M32]=*mode_list; break;
                case 1200: if(vesa_data.mode->x_resolution==1600)
                   SVGALIB_VESA[G1600x1200x16M32]=*mode_list; break;
                case 1440: if(vesa_data.mode->x_resolution==1920)
                   SVGALIB_VESA[G1920x1440x16M32]=*mode_list; break;
             }
          } else {  
              i=0;
              switch(vesa_data.mode->y_resolution){
                 case 200: if(vesa_data.mode->x_resolution==320)
                    i=G320x200x32K; break;
                 case 240: if(vesa_data.mode->x_resolution==320)
                    i=G320x240x32K; break;
                 case 300: if(vesa_data.mode->x_resolution==400)
                    i=G400x300x32K; break;
                 case 384: if(vesa_data.mode->x_resolution==512)
                    i=G512x384x32K; break;
                 case 480: if(vesa_data.mode->x_resolution==640)
                    i=G640x480x32K; break;
                 case 600: if(vesa_data.mode->x_resolution==800)
                    i=G800x600x32K; break;
                 case 720: if(vesa_data.mode->x_resolution==960)
                    i=G960x720x32K; break;
                 case 768: if(vesa_data.mode->x_resolution==1024)
                    i=G1024x768x32K; break;
                 case 864: if(vesa_data.mode->x_resolution==1152)
                    i=G1152x864x32K; break;
                 case 1024: if(vesa_data.mode->x_resolution==1280)
                    i=G1280x1024x32K; break;
                 case 1200: if(vesa_data.mode->x_resolution==1600)
                    i=G1600x1200x32K; break;
                 case 1440: if(vesa_data.mode->x_resolution==1920)
                    i=G1920x1440x32K; break;
              };
              if(i>0)switch(vesa_data.mode->green_mask_size){
                 case 5:SVGALIB_VESA[i]=*mode_list; break;
                 case 6:SVGALIB_VESA[i+1]=*mode_list; break;
                 case 8:if(vesa_data.mode->rsvd_mask_size==0)SVGALIB_VESA[i+2]=*mode_list; break;
              };
          };	
       } else if ((vesa_data.mode->memory_model == VBE_MODEL_256) ||
          	  (vesa_data.mode->memory_model == VBE_MODEL_PACKED)){
            switch(vesa_data.mode->y_resolution){
               case 200: if(vesa_data.mode->x_resolution==320)
                  SVGALIB_VESA[G320x200x256]=*mode_list; break;
               case 240: if(vesa_data.mode->x_resolution==320) {
                 if(vesa_data.mode->number_of_planes==8)
                    SVGALIB_VESA[G320x240x256]=*mode_list; 
                    else SVGALIB_VESA[G320x240x256V]=*mode_list; 
                 }; 
                 break;
               case 300: if(vesa_data.mode->x_resolution==400)
                  SVGALIB_VESA[G400x300x256]=*mode_list; break;
               case 384: if(vesa_data.mode->x_resolution==512)
                  SVGALIB_VESA[G512x384x256]=*mode_list; break;
               case 480: if(vesa_data.mode->x_resolution==640)
                  SVGALIB_VESA[G640x480x256]=*mode_list; break;
               case 600: if(vesa_data.mode->x_resolution==800)
                  SVGALIB_VESA[G800x600x256]=*mode_list; break;
               case 720: if(vesa_data.mode->x_resolution==960)
                  SVGALIB_VESA[G960x720x256]=*mode_list; break;
               case 768: if(vesa_data.mode->x_resolution==1024)
                  SVGALIB_VESA[G1024x768x256]=*mode_list; break;
               case 864: if(vesa_data.mode->x_resolution==1152)
                  SVGALIB_VESA[G1152x864x256]=*mode_list; break;
               case 1024: if(vesa_data.mode->x_resolution==1280)
                  SVGALIB_VESA[G1280x1024x256]=*mode_list; break;
               case 1200: if(vesa_data.mode->x_resolution==1600)
                  SVGALIB_VESA[G1600x1200x256]=*mode_list; break;
               case 1440: if(vesa_data.mode->x_resolution==1920)
                  SVGALIB_VESA[G1920x1440x256]=*mode_list; break;
            }
       }
#endif
       mode_list++;
    };
    vesa_r.eax=0x4f04;
    vesa_r.edx=0;
    vesa_r.ecx=__svgalib_VESA_savebitmap;
    vesa_r.ebx=0;
    __svgalib_LRMI_callbacks->rm_int(0x10,&vesa_r);
    vesa_regs_size=vesa_r.ebx*64;
    __svgalib_LRMI_callbacks->rm_free_real(vesa_data.info);

    SVGALIB_VESA[TEXT]=3;

    cardspecs = malloc(sizeof(CardSpecs));
    cardspecs->videoMemory = vesa_memory;
    cardspecs->maxPixelClock4bpp = 300000;	
    cardspecs->maxPixelClock8bpp = 300000;	
    cardspecs->maxPixelClock16bpp = 300000;	
    cardspecs->maxPixelClock24bpp = 300000;
    cardspecs->maxPixelClock32bpp = 300000;
    cardspecs->flags = CLOCK_PROGRAMMABLE;
    cardspecs->maxHorizontalCrtc = 4088;
    cardspecs->nClocks =1;
    cardspecs->clocks = NULL;
    cardspecs->mapClock = vesa_map_clock;
    cardspecs->mapHorizontalCrtc = vesa_map_horizontal_crtc;
    cardspecs->matchProgrammableClock=vesa_match_programmable_clock;
    __svgalib_driverspecs = &__svgalib_vesa_driverspecs;

    LRMI_mem1 = __svgalib_LRMI_callbacks->rm_alloc_real(vesa_regs_size);
    LRMI_mem2 = __svgalib_LRMI_callbacks->rm_alloc_real(sizeof(struct vbe_info_block)
                                    + sizeof(struct vbe_mode_info_block));

    __svgalib_banked_mem_base=0xa0000;
    __svgalib_banked_mem_size=0x10000;
    if(vesa_chiptype>=1) {
      __svgalib_linear_mem_base=vesa_linear_base;
      __svgalib_linear_mem_size=vesa_memory*0x400;
    };
    
    if (__svgalib_driver_report) {
	fprintf(stderr,"Using VESA driver, %iKB. %s\n",vesa_memory,
                (vesa_chiptype==2)?"VBE3":(vesa_chiptype?"VBE2.0":"VBE1.2"));
    }
    return 0;
}

