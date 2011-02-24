/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */
/* HH: Added 4bpp support, and use bytesperpixel. */

#include <stdio.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

static inline void read_write(int off)
{
    *(GM+off)=*(GM+off)+1;
}

int vga_drawpixel(int x, int y)
{
    int c, offset;

    if (MODEX) {
	     /* select plane */
             __svgalib_outseq(0x02,1 << (x & 3));
	     /* write color to pixel */
             *(GM+y * CI.xbytes + (x >> 2))=COL;
	return 0;
    }
    switch (__svgalib_cur_info.bytesperpixel) {
    case 0:			/* Must be 2 or 16 color mode. */
	/* set up video page */
	offset = y * CI.xbytes + (x >> 3);
	vga_setpage(offset >> 16);
	offset &= 0xffff;

	/* select bit */
        __svgalib_outgra(0x08, 0x80 >> (x & 7));

	/* read into latch and write dummy back */
	read_write(offset);
	break;
    case 1:
	offset = y * CI.xbytes + x;

	if ( CAN_USE_LINEAR ) {
            *(LINEAR_POINTER+offset)=COL;
        } else {
	    /* select segment */
	    vga_setpage(offset >> 16);
    
	    /* write color to pixel */
            *(GM+(offset&0xffff))=COL;
        }
	break;
    case 2:
	offset = y * CI.xbytes + x * 2;
	if ( CAN_USE_LINEAR ) {
            *(uint16_t *)(LINEAR_POINTER+offset)=COL;
        } else {
	    vga_setpage(offset >> 16);
            *(uint16_t *)(GM+(offset&0xffff))=COL;
        }
	break;
    case 3:
	c = COL;
	offset = y * CI.xbytes + x * 3;
	if ( CAN_USE_LINEAR ) {
            /* Only on little endian */
            *(LINEAR_POINTER+offset)=COL&0xff;
            *(LINEAR_POINTER+offset+1)=(COL&0xff00)>>8;
            *(LINEAR_POINTER+offset+2)=(COL&0xff0000)>>16;
        } else {
	    vga_setpage(offset >> 16);
	    switch (offset & 0xffff) {
	    case 0xfffe:
                *(GM+0xfffe)=COL&0xff;
		*(GM+0xffff)=(COL&0xff00)>>8;
	        vga_setpage((offset >> 16) + 1);
		*(GM)=(COL&0xff0000)>>16;
	        break;
	    case 0xffff:
                *(GM+0xffff)=COL&0xff;
	        vga_setpage((offset >> 16) + 1);
		*(GM)=(COL&0xff00)>>8;
		*(GM+1)=(COL&0xff0000)>>16;
	        break;
	    default:
	        offset &= 0xffff;
                *(GM+offset)=COL&0xff;
		*(GM+offset+1)=(COL&0xff00)>>8;
		*(GM+offset+2)=(COL&0xff0000)>>16;
	        break;
	    }
        }
	break;
    case 4:
	offset = y * __svgalib_cur_info.xbytes + x * 4;
	if ( CAN_USE_LINEAR ) {
            *(uint32_t *)(LINEAR_POINTER+offset)=COL;
        } else {
	    vga_setpage(offset >> 16);
            *(uint32_t *)(GM+(offset&0xffff))=COL;
        }
	break;
    }
    return 0;
}

int vga_getpixel(int x, int y)
{
    unsigned char mask;
    int offset, pix = 0;

    if (MODEX) {
	     /* select plane */
	     __svgalib_outseq(0x02, 1 << (x & 3) );	    
	return gr_readb(y * CI.xbytes + (x >> 2));
    }
    switch (__svgalib_cur_info.bytesperpixel) {
    case 0:			/* Must be 2 or 16 color mode. */
	     /* set up video page */
	     offset = y * CI.xbytes + (x >> 3);
	     vga_setpage(offset >> 16);
	     offset &= 0xffff;

	     /* select bit */
	     mask = 0x80 >> (x & 7);
	     pix = 0;
             __svgalib_outgra(0x04,0);
	     if (gr_readb(offset) & mask)
	        pix |= 0x01;
             __svgalib_outgra(0x04,1);
	     if (gr_readb(offset) & mask)
	        pix |= 0x02;
             __svgalib_outgra(0x04,2);
	     if (gr_readb(offset) & mask)
	        pix |= 0x04;
             __svgalib_outgra(0x04,3);
	     if (gr_readb(offset) & mask)
	        pix |= 0x08;
	return pix;
    case 1:
	offset = y * CI.xbytes + x;
	if ( CAN_USE_LINEAR ) {
            return *(LINEAR_POINTER+offset);
        }
	/* select segment */
	vga_setpage(offset >> 16);
	return gr_readb(offset & 0xffff);
    case 2:
	offset = y * CI.xbytes + x * 2;
	if ( CAN_USE_LINEAR ) {
            return *(uint16_t *)(LINEAR_POINTER+offset);
        }
	vga_setpage(offset >> 16);
	return gr_readw(offset & 0xffff);
    case 3:
	offset = y * CI.xbytes + x * 3;
	if ( CAN_USE_LINEAR ) {
            return (*(LINEAR_POINTER+offset)) + (*(LINEAR_POINTER+offset+1)<<8) + 
                   (*(LINEAR_POINTER+offset+2)<<16);
        }
	vga_setpage(offset >> 16);
	switch (offset & 0xffff) {
	case 0xfffe:
	    pix = gr_readw(0xfffe);
	    vga_setpage((offset >> 16) + 1);
	    return pix + (gr_readb(0) << 16);
	case 0xffff:
	    pix = gr_readb(0xffff);
	    vga_setpage((offset >> 16) + 1);
	    return pix + (gr_readw(0) << 8);
	default:
	    offset &= 0xffff;
	    return gr_readw(offset) + (gr_readb(offset + 2) << 16);
	}
	break; 
    case 4:
	offset = y * __svgalib_cur_info.xbytes + x * 4;
	if ( CAN_USE_LINEAR ) {
            return *(uint32_t *)(LINEAR_POINTER+offset);
        }
	vga_setpage(offset >> 16);
	return gr_readl(offset & 0xffff);
    }
    return 0;
}

