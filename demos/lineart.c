/*  A program to test any mode for linear.  Default mode is G640x480x256 = 10
**  or parameters may be used giving modes as integers.
** linp [mode mode ...]
**                          Don Secrest Oct. 1998
*/

#include <vga.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

static void screen(int mode)
{
  int bpp,bott,endp,linlen,i,j,bii,col,p;
  vga_modeinfo *minf;
  unsigned char *vbuf;
  int mem;
  
  if(mode == 0)
    {
      printf("Usage:lineart [mode mode ...]\n\nwhere mode is an integer.\n");
      return;
    }
  if(! vga_hasmode(mode)) {
    printf("Invalid mode %d\n",mode);
    return;
  }
  vga_setmode(mode);
  minf = vga_getmodeinfo(mode);
  if(! (minf->flags & CAPABLE_LINEAR)){
    vga_setmode(TEXT);
    printf("The mode %d is not capable of linear\n",mode);
    return;
  }
  vga_setpage(0);
  if(vga_setlinearaddressing() == -1) {
    vga_setmode(TEXT);
    printf("Could not set linear addressing for mode %d\n",mode);
    return;
  }
  bpp = minf->bytesperpixel;
  linlen = minf->width*bpp;
  bott = linlen*17;            /* pointer 17 pixels wide. */
  endp = linlen*minf->height;
  mem = minf->linewidth*minf->height;

  /* Do random pixels */
  vbuf = vga_getgraphmem();
  printf("Memory mapped to %08x. Mode = %d.\n",(int) vbuf,mode);
  memset(vbuf,0,mem);    /* Clear out 2 megabytes of memory, */
  for(i = 0;i < 100000;i++)
    {
      p = rand() % mem-2;
      *(vbuf + p) = rand() & 0xff;
      if(bpp > 1)
	*(vbuf + p + 1) = rand() & 0xff;
      if(bpp == 3) 
	*(vbuf + p + 2) = rand() & 0xff;
    }

  /* Place marker at top left and bottem right. */
  for(i = 0;i < 44;i += bpp) {
    *(vbuf + i) = 0x60;
    *(vbuf + bott + i) = 0x60;
    *(vbuf + endp - i) = 0x60;
    bii = endp -1 -bott;
    *(vbuf + bii -i) = 0x60;
    if(bpp > 1) {
      *(vbuf + i + 1) = 0x60;
      *(vbuf + i + 1 + bott) = 0x60;
      *(vbuf - i - 1 + endp) = 0x60;
      *(vbuf - i - 1 + bii) = 0x60;
    }
    if(bpp == 3) {
            *(vbuf + i + 2) = 0x60;
	    *(vbuf + i + 2 + bott) = 0x60;
	    *(vbuf - i - 2 + endp) = 0x60;
	    *(vbuf - i - 2 + bii) = 0x60;
    }
    col = (i == 0 || i >= 42)? 0x60:0;
    for(j = 1;j < 17;j++) {
      *(vbuf + i + linlen*j) = col;
      *(vbuf - i + endp -1 - linlen*j) = col;
      if(bpp > 1) {
	*(vbuf + i + 1 + linlen*j) = col;
	*(vbuf - i - 2 + endp - linlen*j) = col;
      }
      if(bpp == 3) {
	*(vbuf + i + 2 + linlen*j) = col;
	*(vbuf - i - 3 + endp - linlen*j) = col;
      }
    }
  }
  for(i = 5;i < 12;i += bpp)
    for(j = 4;j < 12;j++) {
            *(vbuf + i + linlen*j) = 0x3f;
	    *(vbuf + endp - i - bpp - linlen*j) = 0x3f;
    }
  getchar();   /* Wait for a key punch */
}

int
main(int argc,char *argv[])
{
  int mode,c;

  vga_init();
  c=1;
  if(argc == 1)
    screen(10);     /* G640x480x256 */
  else
    c = 0;
  while(argc > 1)
    {
      argc--;
      c++;
      if(isdigit(*argv[c]))
	mode = atoi(argv[c]);
      else if(*argv[c] == 'G')
	mode = vga_getmodenumber(argv[c]);
      else
	{
	  printf("Unknown mode %s\n",argv[c]);
	  continue;
	}
      screen(mode);
    }
  vga_setmode(TEXT);
  return 0;
}

