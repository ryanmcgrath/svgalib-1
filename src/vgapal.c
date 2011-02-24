/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* Converted to especially ugly code and seriously hacked for Mach32: */
/* M. Weller in 1994                                                  */
#include <stdlib.h>

#include "vga.h"
#include "vgaio.h"
#include "libvga.h"
#include "driver.h"

/*
 * In grayscale mode, we convert RGB colors to a Y component on the
 * green-channel (the Y component is used in grayscale TV sets for the
 * same purpose and corresponds to the "brightness" of the color as
 * perceived by the human eye.  In order to be able to return to the
 * user the original green-component, we save a backup copy of the
 * green channel in __svgalib_green_backup:
 */
int __svgalib_green_backup[256];


static int set_lut(int index, int red, int green, int blue)
{
    if (__svgalib_novga) return 1;
    
    /* prevents lockups */
    if ((__svgalib_chipset == MACH64)) {
        port_out_r(0x02ec+0x5c00,index);
        port_out_r(0x02ec+0x5c01,red);
        port_out_r(0x02ec+0x5c01,green);
        port_out_r(0x02ec+0x5c01,blue);
        return 0;
    }

    __svgalib_outpal(index,red,green,blue);
     
    return 0;
}


static int get_lut(int index, int *red, int *green, int *blue)
{
    if (__svgalib_novga) return 0;

    /* prevents lockups on mach64 */
    if ((__svgalib_chipset == MACH64)) {
        port_out_r(0x02ec+0x5c00,index);
        *red=port_in(0x02ec+0x5c01);
        *green=port_in(0x02ec+0x5c01);
        *blue=port_in(0x02ec+0x5c01);
        return 0;
    }

    __svgalib_inpal(index,red,green,blue);

    return 0;
}

int vga_setpalette(int index, int red, int green, int blue)
{

    DTP((stderr,"setpalette %i %i %i %i\n",index,red,green,blue));

    if (__svgalib_grayscale) {
	if ((unsigned) index >= sizeof(__svgalib_green_backup) / sizeof(__svgalib_green_backup[0])) {
	    fprintf(stderr,"vga_setpalette: color index %d out of range\n", index);
	}
	__svgalib_green_backup[index] = green;

	green = 0.299 * red + 0.587 * green + 0.114 * blue;
	if (green < 0)
	    green = 0;
	if (green > 255)
	    green = 255;
    }

    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->setpalette) {
        return __svgalib_driverspecs->emul->setpalette(index, red, green, blue);
    } else {
	return set_lut(index, red, green, blue);
    }
}

int vga_getpalette(int index, int *red, int *green, int *blue)
{
    if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->getpalette) 
        __svgalib_driverspecs->emul->getpalette(index, red, green, blue);
    else get_lut(index, red, green, blue);
    if (__svgalib_grayscale) {
	if ((unsigned) index >= sizeof(__svgalib_green_backup) / sizeof(__svgalib_green_backup[0])) {
	    fprintf(stderr,"vga_getpalette: color index %d out of range\n", index);
	}
	*green = __svgalib_green_backup[index];
    }
    return 0;
}


int vga_setpalvec(int start, int num, int *pal)
{
    int i;

    DTP((stderr,"setpalvec %i %i %x\n",start,num,pal));

    if ((__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->setpalette) ||
	(__svgalib_outpal!=__svgalib_vga_outpal)) {
        for (i = start; i < start + num; ++i) {
	    vga_setpalette(i, pal[0], pal[1], pal[2]);
	    pal += 3;
        }
    } else {
        unsigned char string[768];

        if ( num > 256 )
            return 0;

        for (i = 0; i < num * 3; i++)
            string[i] = pal[i];

        port_out ( start, 0x3c8 );
#if 0
        port_rep_outb( string, num * 3, 0x3c9 );
#else
        for(i=0;i<num*3;i++)port_out(string[i],0x3c9);
#endif
    }

    return num;
}


int vga_getpalvec(int start, int num, int *pal)
{
    int i;

    for (i = start; i < start + num; ++i) {
	vga_getpalette(i, pal + 0, pal + 1, pal + 2);
	pal += 3;
    }
    return num;
}
