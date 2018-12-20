#include <vga.h>
#include <stdlib.h>


int main(void)
{ vga_init();
/*  vga_setmode(G320x200x256); */
  vga_setmode(146);
  while(vga_getkey()==0) vga_setcolor(rand()&255),
                         vga_drawpixel(rand()%320,rand()%200);
  vga_setmode(TEXT);
  exit(0);
}
