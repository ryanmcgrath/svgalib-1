/* Make cursor mkcur.c A program to build a cursor.  Started Jan 27, 2001 */
/* Don Secrest            */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <vga.h>
#include <vgagl.h>
#include <vgamouse.h>
#include "arrow.h"

unsigned char *fnt=0;
static int software = 0,psiz;
static unsigned int sprt[64] = {0};

void usage(void)
{
  puts("Usage:\n"
       "buildcsr -p<sprite> -b<sprite> -m<mode> -s\n"
       "\tDraw sprite by holding down the left mouse button.\n"
       "\tDraw color 2 type 2 and use left button.\n"
       "\tRight mouse button to erase pixels.\n"
       "\t-p print out the sprite as a header file to be compiled\n"
       "\t-b print out the sprite as a binary file to be read by a program\n"
       "\t-m to use any mode. Default is vga_default mode or G640x480x256.\n"
       "\t-s to use software cursor. Default is to use hardware cursor if it\n"
       "\t exits."
       );
    exit(2);
};

void setcursor(int *arrow,int cursor, int color0, int color1)
{
  static int init = 1;

  if(init)
    {
      init = 0;
      if(cursor != 0)
	vga_setcursorimage(cursor,0,color0,color1,(void *)arrow);
      else
	{
	  vga_setmousesupport(1);
	  vga_initcursor(software);
	}
    }
      vga_setcursorimage(cursor,0,color0,color1,(void *) arrow);
  /* if(vga_selectcursor(0) == -1)
    {
      vga_setmode(TEXT);
      printf("Cursor select failure.\n");
      exit(1);
      } */
  vga_selectcursor(cursor);
  mouse_setposition(0,0);
  mouse_setxrange(-5,psiz - 1);
  mouse_setyrange(0,psiz - 1);
  return;
};

int main(int argc,char **argv)
{
  int vgamode,opt,vmode=0,color1,color2,xmax,ymax,i,j,px,dx,colnum;
  int colors;
  char *nameb=0,*namep=0;
  FILE *binfile=0,*progfile=0;

  while(EOF !=(opt = getopt(argc,argv,"p:b:m:s")))
    switch(opt){
    case 'p':
      namep = optarg;
      break;
    case 'b':
      nameb = optarg;
      break;
    case 'm':
      vmode = atoi(optarg);
      break;
    case 's':
      software = 1;
      break;
    case ':':
      printf("Missingh argument.\n");
      usage();
    case '?':
      printf("Unknown argument, %c\n",optopt);
      usage();
    }

  vga_init();
  if(vmode)
    vgamode = vmode;
  else
    vgamode = vga_getdefaultmode();
  if(vgamode == -1)
    vgamode = G640x480x256;
  if(!vga_hasmode(vgamode)){
    printf("Mode %d not available\n",vgamode);
    exit(1);
  }
  vga_setmode(vgamode);
  gl_setcontextvga(vgamode);
  gl_enableclipping();
  fnt = gl_font8x8;
  gl_setfont(8,8,fnt);
  gl_setwritemode(FONT_COMPRESSED + WRITEMODE_OVERWRITE);
  gl_setfontcolors(0,vga_white());
  colors = color1 = gl_rgbcolor(0,200,0);
  color2 = gl_rgbcolor(100,0,100);
  xmax = vga_getxdim();
  ymax = vga_getydim();
  colnum = vga_getcolors();
  if(colnum == 256)
    vga_setcolor(vga_white());
  else
    vga_setrgbcolor(255,255,255);
  psiz = (xmax < ymax)?xmax:ymax;
  i = (psiz -8)/32;
  psiz = i*32 - 1;
  j = (i*5)/6;
  px = j;
  dx = i;
  setcursor(arrow,0,0xff0000,0x0000ff);
  vga_drawline(0,0,psiz+1,0);
  vga_drawline(psiz+1,0,psiz+1,psiz);
  vga_drawline(psiz+1,psiz,0,psiz);
  vga_drawline(0,psiz,0,0);
  gl_printf(1,psiz+1,"Type n: new sprite, o: old, q: quit");
  gl_fillbox(290,psiz+1,px,px,colors);
  do
    {
      int evt,mx,my,button,key;

      evt = vga_waitevent(VGA_MOUSEEVENT | VGA_KEYEVENT,NULL,NULL,NULL,NULL);
      if(evt & VGA_KEYEVENT)
	{
	  key = vga_getkey();
	  if(key == 'q' || key == 'Q')
	    break;
	  if(key == 'n')
	    {
	      setcursor(sprt,1,0xff0000,0x0000ff);
	    }
	  if(key == 'o')
	    {
	      setcursor(arrow,0,0xff0000,0x0000ff);
	    }
	  if(key == '2')
	    {
	      colors = color2;
	      gl_fillbox(290,psiz+1,px,px,colors);
	    }
	  if(key == '1')
	    {
	      colors = color1;
	      gl_fillbox(290,psiz+1,px,px,colors);
	    }
	}
      if(evt & VGA_MOUSEEVENT)
	{
	  int x,y,add;
	  unsigned int loc;

	  button = 0;
	  mouse_update();
	  mx = mouse_getx();
	  my = mouse_gety();
	  button = mouse_getbutton();
	  vga_setcursorposition(mx,my);
	  vga_showcursor(1);
	  if(button)
	    {
	      vga_showcursor(2);
	      x = 1 + (mx/dx)*dx;
	      y = 1 + (my/dx)*dx;
	      gl_fillbox(x,y,px,px,colors);
	      add = y/dx +32;
	      loc = 1 << (31 - x/dx);
	      if(button & 4)
		{
		  sprt[add] = sprt[add] | loc;
		  if(colors == color2)
		    sprt[add - 32] = sprt[add - 32] | loc;
		}
	      else if(button & 1)
		{
		  vga_showcursor(2);
		  x = 1 + (mx/dx)*dx;
		  y = 1 + (my/dx)*dx;
		  gl_fillbox(x,y,px,px,0);
		  sprt[add] = sprt[add] & (~loc);  /* erase it. */
		  sprt[add - 32] = sprt[add - 32] & (~loc);
		}
	    }
	}
    }while(1);
  vga_setmode(TEXT);
  if(namep)
    {
      if((progfile = fopen(namep,"w")))
	{
	  fprintf(progfile,"unsigned int %s[64] = {\n",namep);
	  for(i = 0;i < 63;i++)
	    {
	      fprintf(progfile,"0x%08x,  ",sprt[i]);
	      if((i+1)%4 == 0)
		fprintf(progfile,"\n");
	    }
	  fprintf(progfile,"0x%08x};\n",sprt[63]);
	}
      else
	printf("Unable to open file %s.\n",namep);
    }
  if(nameb)
    {
      if((binfile = fopen(nameb,"w")))
	fwrite(sprt,4,64,binfile);
      else
	printf("Unable to open file %s.\n",nameb);
    }
  printf("psiz = %d, px = %d, dx = %d\n",psiz,px,dx);
  return(0);
}
