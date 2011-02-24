/* Program to test the svgalib keyboard functions. */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <vga.h>
#include <vgagl.h>
#include <vgakeyboard.h>

void usage(void)
{
    puts("Usage: svidtune [mode]\n"
    );
    exit(2);
}

int main(int argc, char **argv)
{
    int x=-1,xmax,ymax;
    char buffer[2];
    int vgamode;
    int key, retval = 0;
    int pixelClock;
    int HDisplay;
    int HSyncStart;
    int HSyncEnd;
    int HTotal;
    int VDisplay;
    int VSyncStart;
    int VSyncEnd;
    int VTotal;
    int flags;
    float hsf,vsf;
    char flagstring[256];

    if (argc > 2)
	usage();
    if(argc==2){
       	if(!sscanf(argv[1], "%d", &x))
	  usage();
    };

    vga_init();
    if(x==-1)vgamode = vga_getdefaultmode(); else vgamode=x;
    if (vgamode == -1)
	vgamode = G640x480x256;

    if (!vga_hasmode(vgamode)) {
	printf("Mode not available.\n");
	exit(1);
    }

    vga_setmode(vgamode);
    gl_setcontextvga(vgamode);
    gl_enableclipping();
    gl_setfont(8, 8, gl_font8x8);
    gl_setwritemode(FONT_COMPRESSED + WRITEMODE_OVERWRITE);
    gl_setfontcolors(0, vga_white());

    buffer[1] = 0;
    for(;!buffer[1];) {
        vga_getcurrenttiming(&pixelClock, 
           		     &HDisplay,		
           		     &HSyncStart,
                             &HSyncEnd,
                             &HTotal,
                             &VDisplay,
                             &VSyncStart,
                             &VSyncEnd,
                             &VTotal,
                             &flags);
        
        sprintf(flagstring,"%s%s%s%s%s%s",
                flags&1?"+hsync ":"",
                flags&2?"-hsync ":"",
                flags&4?"+vsync ":"",
                flags&8?"-vsync ":"",
                flags&16?"interlaced ":"",
                flags&32?"doublescan":"");

        hsf=pixelClock*1000.0/HTotal;
        vsf=hsf/VTotal;
        if(flags&32)vsf=vsf/2;

	gl_printf(500,500,"מתן זיו-אב");
        gl_printf(10,5, "Clock:%i", pixelClock);
        gl_printf(10,25, "HDisplay:%i", HDisplay);
        gl_printf(10,45, "HSyncStart:%i", HSyncStart);
        gl_printf(10,65, "HSyncEnd:%i", HSyncEnd);
        gl_printf(10,85, "HTotal:%i", HTotal);
        gl_printf(10,105, "VDisplay:%i", VDisplay);
        gl_printf(10,125, "VSyncStart:%i", VSyncStart);
        gl_printf(10,145, "VSyncEnd:%i", VSyncEnd);
        gl_printf(10,165, "VTotal:%i", VTotal);
        gl_printf(10,185, "flags:%i = %s", flags,flagstring);
        gl_printf(10,205, "Horz freq:%.3f kHz", hsf/1000);
        gl_printf(10,215, "Vert freq:%.2f Hz", vsf);

	if(VDisplay>270){
	        gl_printf(10,235, "Up, Down, Left, Right, Wider, Narrower, lOnger, Shorter");
        	gl_printf(10,258, "'p' - prints current modeline to stdout,  'P' - to config file ");
        };
        xmax = vga_getxdim() - 1;
        ymax = vga_getydim() - 1;
    
        vga_setcolor(vga_white());
        vga_drawline(0, 0, xmax, 0);
        vga_drawline(xmax, 0, xmax, ymax);
        vga_drawline(xmax, ymax, 0, ymax);
        vga_drawline(0, ymax, 0, 0);
        
	key = vga_getch();
	switch(key) {
           case 4:
           case 'q':
              buffer[1]=1;
              break;
           case 'l':
              vga_changetiming(0,0,8,8,0,0,0,0,0,0);
              break;
           case 'r':
              vga_changetiming(0,0,-8,-8,0,0,0,0,0,0);
              break;
           case 'u':
              vga_changetiming(0,0,0,0,0,0,1,1,0,0);
              break;
           case 'd':
              vga_changetiming(0,0,0,0,0,0,-1,-1,0,0);
              break;
           case 'w':
              vga_changetiming(0,0,-4,-4,-8,0,0,0,0,0);
              break;
           case 'n':
              vga_changetiming(0,0,4,4,8,0,0,0,0,0);
              break;
           case 's':
              vga_changetiming(0,0,0,0,0,0,1,1,2,0);
              break;
           case 'o':
              vga_changetiming(0,0,0,0,0,0,-1,-1,-2,0);
              break;
           case 'p':
              fprintf(stderr,"Modeline %c%ix%i@%.0f%c %.3f %i %i %i %i %i %i %i %i %s\n",'"',xmax+1,ymax+1,vsf,'"',
                 	     pixelClock/1000.0, 
           		     HDisplay,		
           		     HSyncStart,
                             HSyncEnd,
                             HTotal,
                             VDisplay,
                             VSyncStart,
                             VSyncEnd,
                             VTotal,
                             flagstring);
              break;
           case 'P':{
              FILE *f;
              f=fopen("/etc/vga/libvga.config","a");
              fprintf(f,"Modeline %c%ix%i@%.0f%c %.3f %i %i %i %i %i %i %i %i %s\n",'"',xmax+1,ymax+1,vsf,'"',
                 	     pixelClock/1000.0, 
           		     HDisplay,		
           		     HSyncStart,
                             HSyncEnd,
                             HTotal,
                             VDisplay,
                             VSyncStart,
                             VSyncEnd,
                             VTotal,
                             flagstring);
              fclose(f);
              };
              break;

        };
    }

    vga_setmode(TEXT);

    exit(retval);
}
