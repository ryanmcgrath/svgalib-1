/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

#include <stdio.h>
#include "vga.h"
#include "libvga.h"

#define ABS(a) (((a)<0) ? -(a) : (a))

int vga_drawline(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    int ax = ABS(dx) << 1;
    int ay = ABS(dy) << 1;
    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;

    int x = x1;
    int y = y1;

    if (ax > ay) {
	int d = ay - (ax >> 1);
	while (x != x2) {
	    vga_drawpixel(x, y);

	    if (d > 0 || (d == 0 && sx == 1)) {
		y += sy;
		d -= ax;
	    }
	    x += sx;
	    d += ay;
	}
    } else {
	int d = ax - (ay >> 1);
	while (y != y2) {
	    vga_drawpixel(x, y);

	    if (d > 0 || (d == 0 && sy == 1)) {
		x += sx;
		d -= ay;
	    }
	    y += sy;
	    d += ax;
	}
    }
    vga_drawpixel(x, y);

    return 0;
}

