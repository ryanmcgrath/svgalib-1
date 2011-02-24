/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

/* 21 January 1995 - added vga_readscanline(), added support for   */
/* non 8-pixel aligned scanlines in 16 color mode. billr@rastergr.com */
#include <stdio.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"

/* used to decompose color value into bits (for fast scanline drawing) */
union bits {
    struct {
		unsigned char bit3;
		unsigned char bit2;
		unsigned char bit1;
		unsigned char bit0;
    } b;
    unsigned int i;
};

/* color decompositions */
static union bits color16[16] =
{
    {{0, 0, 0, 0}},
    {{0, 0, 0, 1}},
    {{0, 0, 1, 0}},
    {{0, 0, 1, 1}},
    {{0, 1, 0, 0}},
    {{0, 1, 0, 1}},
    {{0, 1, 1, 0}},
    {{0, 1, 1, 1}},
    {{1, 0, 0, 0}},
    {{1, 0, 0, 1}},
    {{1, 0, 1, 0}},
    {{1, 0, 1, 1}},
    {{1, 1, 0, 0}},
    {{1, 1, 0, 1}},
    {{1, 1, 1, 0}},
    {{1, 1, 1, 1}}
};

/* mask for end points in plane buffer mode */
static unsigned char mask[8] =
{
    0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};

/* display plane buffers (for fast scanline drawing) */
/* 256 bytes -> max 2048 pixel per line (2/16 colors) */
static unsigned char plane0[256];
static unsigned char plane1[256];
static unsigned char plane2[256];
static unsigned char plane3[256];

static inline void shifted_memcpy(void *dest_in, void *source_in, int len)
{
    int *dest = dest_in;
    int *source = source_in;

    len >>= 2;

    while (len--)
	*dest++ = (*source++ << 8);
}

/* RGB_swapped_memcopy returns the amount of bytes unhandled */
static inline int RGB_swapped_memcpy(uint8_t *dest, uint8_t *source, int len)
{
    int rest, tmp;

    tmp = len / 3;
    rest = len - 3 * tmp;
    len = tmp;

    while (len--) {
	*dest++ = source[2];
	*dest++ = source[1];
	*dest++ = source[0];
	source += 3;
    }

    return rest;
}

int vga_drawscanline(int line, unsigned char *colors)
{
    if ((CI.colors == 2) || (CI.colors > 256))
		return vga_drawscansegment(colors, 0, line, CI.xbytes);
    else
		return vga_drawscansegment(colors, 0, line, CI.xdim);
}

#ifdef LIBC_MEMCPY
#define MEMCPY memcpy
#else
void MEMCPY(unsigned char *dst, unsigned char *src, size_t n) {
    unsigned char *e;
    e=src+n;
    while(src<e) *(dst++)=*(src++);
}
#endif

int vga_drawscansegment(unsigned char *colors, int x, int y, int length)
{
    /* both length and x must divide with 8 */
    /*  no longer true (at least for 16 & 256 colors) */
    if (MODEX)
		goto modeX;

    switch (CI.colors) {
	    case 16: {
			int i, j, k, first, last, page, l1, l2;
			unsigned int offset, eoffs, soffs, ioffs;
			union bits bytes;
			unsigned char *address;
			
			k = 0;
			soffs = ioffs = (x & 0x7);	/* starting offset into first byte */
			eoffs = (x + length) & 0x7;		/* ending offset into last byte */
			for (i = 0; i < length;) {
				bytes.i = 0;
				first = i;
				last = i + 8 - ioffs;
				if (last > length)
					last = length;
				for (j = first; j < last; j++, i++)
					bytes.i = (bytes.i << 1) | color16[colors[j]&15].i;
				plane0[k] = bytes.b.bit0;
				plane1[k] = bytes.b.bit1;
				plane2[k] = bytes.b.bit2;
				plane3[k++] = bytes.b.bit3;
				ioffs = 0;
			}
			if (eoffs) {
				/* fixup last byte */
				k--;
				bytes.i <<= (8 - eoffs);
				plane0[k] = bytes.b.bit0;
				plane1[k] = bytes.b.bit1;
				plane2[k] = bytes.b.bit2;
				plane3[k++] = bytes.b.bit3;
			}
			offset = (y * CI.xdim + x) / 8;
			vga_setpage((page = offset >> 16));
			l1 = 0x10000 - (offset &= 0xffff);
			/* k currently contains number of bytes to write */
			if (l1 > k)
				l1 = k;
			l2 = k - l1;
			/* make k the index of the last byte to write */
			k--;
			
			address = GM + offset;
			
			/* disable Set/Reset Register */
			__svgalib_outgra(0x01,0x00);
			
			/* write to all bits */
			__svgalib_outgra(0x08,0xff);
			
			/* select write map mask register */
			__svgalib_outseq(0x02,0x01);
			
			/* select read map mask register */
			__svgalib_outgra(0x04,0x00);
			if (soffs)
				plane0[0] |= *address & ~mask[soffs];
			if (eoffs && l2 == 0)
				plane0[k] |= *(address + l1 - 1) & mask[eoffs];
			MEMCPY(address, plane0, l1);
			
			/* write plane 1 */
			__svgalib_outseq(0x02,0x02);
			/* read plane 1 */
			__svgalib_outgra(0x04,0x01);
			if (soffs)
				plane1[0] |= *address & ~mask[soffs];
			if (eoffs && l2 == 0)
				plane1[k] |= *(address + l1 - 1) & mask[eoffs];
			MEMCPY(address, plane1, l1);
			
			/* write plane 2 */
			__svgalib_outseq(0x02,0x04);
			/* read plane 2 */
			__svgalib_outgra(0x04,0x02);
			if (soffs)
				plane2[0] |= *address & ~mask[soffs];
			if (eoffs && l2 == 0)
				plane2[k] |= *(address + l1 - 1) & mask[eoffs];
			MEMCPY(address, plane2, l1);
			
			/* write plane 3 */
			__svgalib_outseq(0x02,0x08);
			/* read plane 3 */
			__svgalib_outgra(0x04,0x03);
			if (soffs)
				plane3[0] |= *address & ~mask[soffs];
			if (eoffs && l2 == 0)
				plane3[k] |= *(address + l1 - 1) & mask[eoffs];
			MEMCPY(address, plane3, l1);
			
			if (l2 > 0) {
				vga_setpage(page + 1);
				
				/* write plane 0 */
				__svgalib_outseq(0x02,0x01);
				if (eoffs) {
					/* read plane 0 */
					__svgalib_outgra(0x04,0x00);
					plane0[k] |= *(GM + l2 - 1) & mask[eoffs];
				}
				MEMCPY(GM, &plane0[l1], l2);
				
				/* write plane 1 */
				__svgalib_outseq(0x02,0x02);
				if (eoffs) {
					/* read plane 1 */
					__svgalib_outgra(0x04,0x01);
					plane1[k] |= *(GM + l2 - 1) & mask[eoffs];
				}
				MEMCPY(GM, &plane1[l1], l2);
				
				/* write plane 2 */
				__svgalib_outseq(0x02,0x04);
				if (eoffs) {
					/* read plane 2 */
					__svgalib_outgra(0x04,0x02);
					plane2[k] |= *(GM + l2 - 1) & mask[eoffs];
				}
				MEMCPY(GM, &plane2[l1], l2);
				
				/* write plane 3 */
				__svgalib_outseq(0x02,0x08);
				if (eoffs) {
					/* read plane 3 */
					__svgalib_outgra(0x04,0x03);
					plane3[k] |= *(GM + l2 - 1) & mask[eoffs];
				}
				MEMCPY(GM, &plane3[l1], l2);
			}
			/* restore map mask register */
			__svgalib_outseq(0x02,0x0f);
			
			/* enable Set/Reset Register */
			__svgalib_outgra(0x01,0x0f);
			}
			break;
		case 2:	{
			/* disable Set/Reset Register */
			__svgalib_outgra(0x01,0x00);
			
			/* write to all bits */
			__svgalib_outgra(0x08,0xff);
			
			/* write to all planes */
			__svgalib_outseq(0x02,0x0f);
			
			MEMCPY(GM + (y * CI.xdim + x) / 8, colors, length);
			
			/* restore map mask register */
			__svgalib_outseq(0x02,0x0f);
			
			/* enable Set/Reset Register */
			__svgalib_outgra(0x01,0x0f);
			}
			break;
		case 256: {
			switch (CM) {
				case G320x200x256:	/* linear addressing - easy and fast */
					MEMCPY(GM + (y * CI.xdim + x), colors, length);
					return 0;
				case G320x240x256:
				case G320x400x256:
				case G360x480x256:
				case G400x300x256X:
modeX:
					{
						int first, offset, pixel, plane;
						
						for (plane = 0; plane < 4; plane++) {
							/* select plane */
							__svgalib_outseq(0x02,1 << plane );
							
							pixel = ((4 - (x & 3) + plane) & 3);
							first = (y * CI.xdim + x) / 4;
							if((x & 3) + pixel > 3)
								first++;
							for (offset = first; pixel < length; offset++) {
								*(GM+offset) = colors[pixel];
								pixel += 4;
							}
						}
					}
					return 0;
			}
			{
				unsigned int offset;
				int segment, free;
				
SegmentedCopy:
				offset = y * CI.xbytes + x;
				if ( CAN_USE_LINEAR ) {
					MEMCPY(LINEAR_POINTER+offset, colors, length);
				} else {
					segment = offset >> 16;
					free = ((segment + 1) << 16) - offset;
					offset &= 0xFFFF;
					
					if (free < length) {
						vga_setpage(segment);
						MEMCPY(GM + offset, colors, free);
						vga_setpage(segment + 1);
						MEMCPY(GM, colors + free, length - free);
					} else {
						vga_setpage(segment);
						MEMCPY(GM + offset, colors, length);
					}
				}
			}
			}
			break;
		case 32768:
		case 65536:
			x *= 2;
			goto SegmentedCopy;
		case 1 << 24:
			if (__svgalib_cur_info.bytesperpixel == 4) {
				x <<= 2;
				if (MODEFLAGS & RGB_MISORDERED) {
					unsigned int offset;
					int segment, free;
					
					offset = y * CI.xbytes + x;
					if ( CAN_USE_LINEAR ) {
						shifted_memcpy(LINEAR_POINTER+offset, colors, length);
					} else {
						segment = offset >> 16;
						free = ((segment + 1) << 16) - offset;
						offset &= 0xFFFF;
						
						if (free < length) {
							vga_setpage(segment);
							shifted_memcpy(GM + offset, colors, free);
							vga_setpage(segment + 1);
							shifted_memcpy(GM, colors + free, length - free);
						} else {
							vga_setpage(segment);
							shifted_memcpy(GM + offset, colors, length);
						}
					}
				} else {
					goto SegmentedCopy;
				}
				break;
			}
			x *= 3;
			if (MODEFLAGS & RGB_MISORDERED) {
				unsigned int offset;
				int segment, free;
				
				offset = y * CI.xbytes + x;
				if ( CAN_USE_LINEAR ) {
					RGB_swapped_memcpy(LINEAR_POINTER+offset, colors, length);
				} else {
					segment = offset >> 16;
					free = ((segment + 1) << 16) - offset;
					offset &= 0xFFFF;
					
					if (free < length) {
						int i;
						
						vga_setpage(segment);
						i = RGB_swapped_memcpy(GM + offset, colors, free);
						colors += (free - i);
						
						switch (i) {
							case 2:
								*(GM+0xfffe) = colors[2];
								*(GM+0xffff) = colors[1];
								break;
							case 1:
								*(GM+0xffff) = colors[2];
								break;
						}
						
						vga_setpage(segment + 1);
						
						switch (i) {
							case 1:
								*(GM+1) = colors[0];
								*(GM)   = colors[1];
								i = 3 - i;
								free += i;
								colors += 3;
								break;
							case 2:
								*(GM) = colors[0];
								i = 3 - i;
								free += i;
								colors += 3;
								break;
						}
						RGB_swapped_memcpy(GM + i, colors, length - free);
					} else {
						vga_setpage(segment);
						RGB_swapped_memcpy(GM + offset, colors, length);
					}
				}
			} else {
				goto SegmentedCopy;
			}
	}
    return 0;
}

int vga_getscansegment(unsigned char *colors, int x, int y, int length)
{

    if (MODEX)
		goto modeX2;
	switch (CI.colors) {
		case 16: {
			int i, k, page, l1, l2;
			int offset, eoffs, soffs, nbytes, bit;
			unsigned char *address;
			unsigned char color;
			
			k = 0;
			soffs = (x & 0x7);	/* starting offset into first byte */
			eoffs = (x + length) & 0x7;		/* ending offset into last byte */
			offset = (y * CI.xdim + x) / 8;
			vga_setpage((page = offset >> 16));
			l1 = 0x10000 - (offset &= 0xffff);
			if (soffs)
				nbytes = (length - (8 - soffs)) / 8 + 1;
			else
				nbytes = length / 8;
			if (eoffs)
				nbytes++;
			if (l1 > nbytes)
				l1 = nbytes;
			l2 = nbytes - l1;
			address = GM + offset;
			/* disable Set/Reset Register */
			__svgalib_outgra(0x01,0x00);
			/* read plane 0 */
			__svgalib_outgra(0x04,0x00);
			memcpy(plane0, address, l1);
			/* read plane 1 */
			__svgalib_outgra(0x04,0x01);
			memcpy(plane1, address, l1);
			/* read plane 2 */
			__svgalib_outgra(0x04,0x02);
			memcpy(plane2, address, l1);
			/* read plane 3 */
			__svgalib_outgra(0x04,0x03);
			memcpy(plane3, address, l1);
			if (l2 > 0) {
				vga_setpage(page + 1);
				/* read plane 0 */
  				__svgalib_outgra(0x04,0x00);
				memcpy(&plane0[l1], GM, l2);
				/* read plane 1 */
  				__svgalib_outgra(0x04,0x01);
				memcpy(&plane1[l1], GM, l2);
				/* read plane 2 */
  				__svgalib_outgra(0x04,0x02);
				memcpy(&plane2[l1], GM, l2);
				/* read plane 3 */
  				__svgalib_outgra(0x04,0x03);
				memcpy(&plane3[l1], GM, l2);
			}
			/* enable Set/Reset Register */
			__svgalib_outgra(0x01,0x0f);
			k = 0;
			for (i = 0; i < length;) {
				for (bit = 7 - soffs; bit >= 0 && i < length; bit--, i++) {
					color = (plane0[k] & (1 << bit) ? 1 : 0);
					color |= (plane1[k] & (1 << bit) ? 1 : 0) << 1;
					color |= (plane2[k] & (1 << bit) ? 1 : 0) << 2;
					color |= (plane3[k] & (1 << bit) ? 1 : 0) << 3;
					colors[i] = color;
				}
				k++;
				soffs = 0;
			}
			}
			break;
		case 2:
			/* disable Set/Reset Register */
			__svgalib_outgra(0x01,0x00);
			/* read from plane 0 */
			__svgalib_outseq(0x04,0x00);
			memcpy(colors, GM + (y * CI.xdim + x) / 8, length);
			/* enable Set/Reset Register */
			__svgalib_outgra(0x01,0x0f);
			break;
		case 256: 
			switch (CM) {
				case G320x200x256:	/* linear addressing - easy and fast */
					memcpy(colors, GM + y * CI.xdim + x, length);
					return 0;
				case G320x240x256:
				case G320x400x256:
				case G360x480x256:
				case G400x300x256X:
modeX2:
					{
						int first, offset, pixel, plane;
						for (plane = 0; plane < 4; plane++) {
							/* select plane */
							__svgalib_outgra(0x04, plane);
							pixel = ((4 - (x & 3) + plane) & 3);
							first = (y * CI.xdim + x) / 4;
							if((x & 3) + pixel > 3)
								first++;
							for (offset = first; pixel < length; offset++) {
								colors[pixel] = gr_readb(offset);
								pixel += 4;
							}
						}
					}
					return 0;
			}
			{
				unsigned int offset;
				int segment, free;
SegmentedCopy2:
				offset = y * CI.xbytes + x;
				if ( CAN_USE_LINEAR ) {
					memcpy(colors, LINEAR_POINTER+offset, length);
				} else {
					segment = offset >> 16;
					free = ((segment + 1) << 16) - offset;
					offset &= 0xFFFF;
					if (free < length) {
						vga_setpage(segment);
						memcpy(colors, GM + offset, free);
						vga_setpage(segment + 1);
						memcpy(colors + free, GM, length - free);
					} else {
						vga_setpage(segment);
						memcpy(colors, GM + offset, length);
					}
				}
			}
			break;
		case 32768:
		case 65536:
			x *= 2;
			goto SegmentedCopy2;
		case 1 << 24:
			if (__svgalib_cur_info.bytesperpixel == 4) {
				x<<=2;
			} else {
				x *= 3;
			}
			goto SegmentedCopy2;
	}
	return 0;
}

