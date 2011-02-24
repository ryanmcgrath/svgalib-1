/* From VGAlib, changed for svgalib */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */


#include <stdio.h>
#include <unistd.h>		/* for usleep( long ) */
#include <string.h>
#include <stdlib.h>
#include "vga.h"

static unsigned char line[2048 * 3];

static void testmode(int mode)
{
    int xmax, ymax, i, x, y, yw, ys, c;
    vga_modeinfo *modeinfo;

    vga_setmode(mode);

    modeinfo = vga_getmodeinfo(mode);

    printf("Width: %d  Height: %d  Colors: %d\n",
	   modeinfo->width,
	   modeinfo->height,
	   modeinfo->colors);
    printf("DisplayStartRange: %xh  Maxpixels: %d  Blit: %s\n",
	   modeinfo->startaddressrange,
	   modeinfo->maxpixels,
	   modeinfo->haveblit ? "YES" : "NO");
    printf("Offset: %i  Bytes Per Pixel: %d\n",
	   modeinfo->linewidth,
	   modeinfo->bytesperpixel);

#ifdef TEST_MODEX
    if (modeinfo->colors == 256)
	printf("Switching to ModeX ... %s\n",
	       (vga_setmodeX()? "done" : "failed"));
#endif

    vga_screenoff();

    xmax = vga_getxdim() - 1;
    ymax = vga_getydim() - 1;

    vga_setcolor(vga_white());
    vga_drawline(0, 0, xmax, 0);
    vga_drawline(xmax, 0, xmax, ymax);
    vga_drawline(xmax, ymax, 0, ymax);
    vga_drawline(0, ymax, 0, 0);
    for (i = 0; i <= 15; i++) {
	vga_setegacolor(i);
	vga_drawline(10 + i * 5, 10, 90 + i * 5, 90);
    }
    for (i = 0; i <= 15; i++) {
	vga_setegacolor(i);
	vga_drawline(90 + i * 5, 10, 10 + i * 5, 90);
    }

    vga_screenon();

    ys = 100;
    yw = (ymax - 100) / 4;
    switch (vga_getcolors()) {
    case 256:
	for (i = 0; i < 60; ++i) {
	    c = (i * 64) / 60;
	    vga_setpalette(i + 16, c, c, c);
	    vga_setpalette(i + 16 + 60, c, 0, 0);
	    vga_setpalette(i + 16 + (2 * 60), 0, c, 0);
	    vga_setpalette(i + 16 + (3 * 60), 0, 0, c);
	}
	line[0] = line[xmax] = 15;
	line[1] = line[xmax - 1] = 0;
	for (x = 2; x < xmax - 1; ++x)
	    line[x] = (((x - 2) * 60) / (xmax - 3)) + 16;
	for (y = ys; y < ys + yw; ++y)	
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	
	    vga_drawscanline(y, line);
	break;

    case 1 << 15:
    case 1 << 16:
    case 1 << 24:
	for (x = 2; x < xmax - 1; ++x) {
	    c = ((x - 2) * 256) / (xmax - 3);
	    y = ys;
	    vga_setrgbcolor(c, c, c);
	    vga_drawline(x, y, x, y + yw - 1);
	    y += yw;
	    vga_setrgbcolor(c, 0, 0);
	    vga_drawline(x, y, x, y + yw - 1);
	    y += yw;
	    vga_setrgbcolor(0, c, 0);
	    vga_drawline(x, y, x, y + yw - 1);
	    y += yw;
	    vga_setrgbcolor(0, 0, c);
	    vga_drawline(x, y, x, y + yw - 1);
	}
	for (x = 0; x < 64; x++) {
	    for (y = 0; y < 64; y++) {
		vga_setrgbcolor(x * 4 + 3, y * 4 + 3, 0);
		vga_drawpixel(xmax / 2 - 160 + x, y + ymax / 2 - 80);
		vga_setrgbcolor(x * 4 + 3, 0, y * 4 + 3);
		vga_drawpixel(xmax / 2 - 32 + x, y + ymax / 2 - 80);
		vga_setrgbcolor(0, x * 4 + 3, y * 4 + 3);
		vga_drawpixel(xmax / 2 + 160 - 64 + x, y + ymax / 2 - 80);

		vga_setrgbcolor(x * 4 + 3, y * 4 + 3, 255);
		vga_drawpixel(xmax / 2 - 160 + x, y + ymax / 2 + 16);
		vga_setrgbcolor(x * 4 + 3, 255, y * 4 + 3);
		vga_drawpixel(xmax / 2 - 32 + x, y + ymax / 2 + 16);
		vga_setrgbcolor(255, x * 4 + 3, y * 4 + 3);
		vga_drawpixel(xmax / 2 + 160 - 64 + x, y + ymax / 2 + 16);
	    }
	}
	break;
    default:
	if (vga_getcolors() == 16) {
	    for (i = 0; i < xmax - 1; i++)
		line[i] = (i + 2) % 16;
	    line[0] = line[xmax] = 15;
	    line[1] = line[xmax - 1] = 0;
	}
	if (vga_getcolors() == 2) {
	    for (i = 0; i <= xmax; i++)
		line[i] = 0x11;
	    line[0] = 0x91;
	}
	for (i = 100; i < ymax - 1; i++)
	    vga_drawscanline(i, line);
	break;

    }
    if (getchar() == 'd')
	vga_dumpregs();

    vga_setmode(TEXT);
}

int main(int argc, char *argv[])
{
    int mode;
    int tests=0xffffffff;

    if(argc>1)tests=strtol(argv[1],NULL,0);

    vga_init();			/* Initialize. */


    if(tests&1){
      vga_addtiming(45000,720,776,880,936,540,570,576,600,0);
      vga_addtiming(36000,720,756,872,950,540,542,547,565,0);
      mode=vga_addmode(720,540,1<<24,720*4,4);
      printf("Mode=%i\n",mode);
      if (vga_hasmode(mode))
	  testmode(mode);
      else printf("Error: Video mode not supported by driver\n");
    };
    
    if(tests&2){
        vga_addtiming(45000,720,776,880,936,720,750,756,800,0);
        vga_addtiming(36000,720,756,872,950,720,723,729,750,0);
        mode=vga_addmode(720,720,1<<24,720*4,4);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };
    
    if(tests&4){
        vga_guesstiming(576,432,0,0);
        mode=vga_addmode(576,432,1<<16,576*2,2);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };
    
    if(tests&8){
        vga_guesstiming(576,431,1,0);
        mode=vga_addmode(576,431,1<<16,576*2,2);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };

    if(tests&16){
        vga_guesstiming(360,271,1,0);
        mode=vga_addmode(360,271,1<<16,360*2,2);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };

    if(tests&32){
        vga_guesstiming(360,270,1,0);
        mode=vga_addmode(360,270,1<<16,360*2,2);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };

    if(tests&64){
        vga_guesstiming(672,800,257,0);
        mode=vga_addmode(672,800,1<<16,672*2,2);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };
    if(tests&128){
        vga_guesstiming(712,800,257,0);
        mode=vga_addmode(712,800,1<<16,712*2,2);
        printf("Mode=%i\n",mode);
        if (vga_hasmode(mode))
	    testmode(mode);
        else printf("Error: Video mode not supported by driver\n");
    };

    return 0;
}
