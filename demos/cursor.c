#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <vga.h>
#include <vgamouse.h>
#include <sys/fcntl.h>
#include <stdint.h>

int mode = G1024x768x64K;

uint32_t mag_glass_bits[] = {
        0x3f8, 0x60c, 0x9e2, 0x1a13, 0x1401, 0x1401, 0x1001, 0x1001,
        0x1001, 0x1003, 0x1802, 0x340c, 0x6ff8, 0xd800, 0xb000, 0xe000, 
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x3f8, 0x60c, 0x9e2, 0x1a13, 0x1401, 0x1401, 0x1001, 0x1001,
        0x1001, 0x1003, 0x1802, 0x340c, 0x6ff8, 0xd800, 0xb000, 0xe000, 
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

uint32_t mag_glass_filled[] = {
        0x3f8, 0x60c, 0x9e2, 0x1a13, 0x1401, 0x1401, 0x1001, 0x1001,
        0x1001, 0x1003, 0x1802, 0x340c, 0x6ff8, 0xd800, 0xb000, 0xe000, 
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0x3f8, 0x7fc, 0xffe, 0x1fff, 0x1fff, 0x1fff, 0x1fff, 0x1fff,
        0x1fff, 0x1fff, 0x1ffe, 0x3ffc, 0x7ff8, 0xf800, 0xe000, 0xe000, 
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};

uint32_t *c2;
static unsigned char line[2048*4];
static void drawpattern(void)
{
    int xmax, ymax, i, x, y, yw, ys, c;
    vga_modeinfo *modeinfo;

    modeinfo = vga_getmodeinfo(mode);

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
	for (y = ys; y < ys + yw; ++y)	/* gray */
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	/* red */
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	/* green */
	    vga_drawscanline(y, line);
	for (x = 2; x < xmax - 1; ++x)
	    line[x] += 60;
	ys += yw;
	for (y = ys; y < ys + yw; ++y)	/* blue */
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
}

int main(int argc, char *argv[]) {
    	int i,s,j;
    	unsigned char *g;
	
        vga_init();
	vga_setmousesupport(1);
	printf("%i\n",vga_initcursor(argc-1));

	vga_setmode(mode);
        vga_setlinearaddressing();
	vga_setcursorposition(100,100);

        c2=malloc(256);
        j=1;
        for(i=0;i<32;i++) {
            *(c2+i)=j;
            *(c2+32+i)=j;
            j<<=1;
        }

        if(vga_getcolors()==256) 
	    for (i = 0; i < 60; ++i) {
	        j = (i * 64) / 60;
	        vga_setpalette(i + 16, j, j, j);
	        vga_setpalette(i + 16 + 60, j, 0, 0);
	        vga_setpalette(i + 16 + (2 * 60), 0, j, 0);
	        vga_setpalette(i + 16 + (3 * 60), 0, 0, j);
	    }
        vga_setcursorimage(0,0,0,0xff0000,(unsigned char *)c2);
        vga_setcursorimage(1,0,0,0xffffff,(unsigned char *)mag_glass_bits);
        vga_setcursorimage(2,0,0xff1080,0x0f0fff,(unsigned char *)mag_glass_filled);
        vga_selectcursor(1);
	vga_showcursor(1);
	vga_showcursor(2);
        g=vga_getgraphmem();

#if 1
    	drawpattern();
#else
        for(j=0;j<256*1024;j++) {
	    if((j&0x3ff)==0x3ff)usleep(1000);
            *(g+2*j)=0x00;
            *(g+2*j+1)=(j*83/69)&0xff;
        }

        for(j=256*1024;j<384*1024;j++) {
	    if((j&0x3ff)==0x3ff)usleep(1000);
            *(g+2*j)=0;
            *(g+2*j+1)=0;
        }
        for(j=384*1024;j<512*1024;j++) {
//	    if((j&0x3ff)==0x3ff)usleep(1000);
            *(g+2*j)=0xff;
            *(g+2*j+1)=0x7f;
        }
        for(j=512*1024;j<2048*1024;j++) {
//	    if((j&0x3ff)==0x3ff)usleep(1000);
            *(g+2*j)=((j>>9)&1)*(((~j)>>4)&0x1f);
            *(g+2*j+1)=(1-((j>>9)&1)) * ((j>>2)&0x7c);
        }
#endif

        i=1;
	
	vga_showcursor(3);
        s=3;
        while(!(mouse_getbutton()&4)) {
		mouse_waitforupdate();
		vga_setcursorposition(mouse_getx(),mouse_gety());
                if(mouse_getbutton()&1) {
                    vga_selectcursor(i=(i+1)%3);
		}
#if 1
                if(mouse_getbutton()&2) {
                    vga_showcursor(s=(s+1)%4);
		}
#else
                if(mouse_getbutton()&2) {
                    vga_dumpregs();
		}
#endif
	}
	vga_setmode(TEXT);
	return 0;
}
