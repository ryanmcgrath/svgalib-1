/* From VGAlib, changed for svgalib */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */


#include <stdio.h>
#include <unistd.h>		/* for usleep( long ) */
#include <string.h>
#include "vga.h"

static unsigned char line[2048 * 3];

static void testmode(int mode)
{
    int xmax, ymax, i, yw, ys;
    unsigned char buf[60];
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
    for (i = 0; i < xmax - 1; i++)
	line[i] = (i + 2) % 16;
    line[0] = line[xmax] = 15;
    line[1] = line[xmax - 1] = 0;
    for (i = 100; i < ymax - 1; i++)
	vga_drawscanline(i, line);

    if (getchar() == 'd')
	vga_dumpregs();

        
    vga_getcrtcregs(buf);

    buf[0]=0x4d;
    buf[1]=0x3f;
    buf[2]=0x3f;
    buf[3]=0x80;
    buf[4]=0x41;
    buf[5]=0x10;
        
    vga_setcrtcregs(buf);
    for(i=0;i<20;i++) {
        vga_setdisplaystart(i);
        usleep(200000);
        vga_waitretrace();
    }

    vga_getch();

    vga_setmode(TEXT);
}

int main(void)
{
    int mode;
    int i, high;

    vga_init();			/* Initialize. */

    mode = 4;

    if (mode == -1) {
	printf("Choose one of the following video modes: \n");

	high = 0;
	for (i = 1; i <= vga_lastmodenumber(); i++)
	    if (vga_hasmode(i)) {
		vga_modeinfo *info;
		char expl[100];
		char *cols = NULL;

		*expl = '\0';
		info = vga_getmodeinfo(i);
                cols = "16";
		strcpy(expl, "4 bitplanes");
		high = i;
		printf("%5d: %dx%d, ",
		       i, info->width, info->height);
		if (cols == NULL)
		    printf("%d", info->colors);
		else
		    printf("%s", cols);
		printf(" colors ");
		if (*expl != '\0')
		    printf("(%s)", expl);
		printf("\n");
	    }
    }
    if (vga_hasmode(mode))
	testmode(mode);
    else {
	printf("Error: Video mode not supported by driver\n");
	exit(-1);
    }
    
    return 0;
}
