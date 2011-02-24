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
    puts("Usage: printftest <x> <y>\n"
	"\tread text from keyboard and display it on screen\n"
	"\tat position (x, y).\n"
	"\t<ctrl>-D for quit\n"
	"\t<ctrl>-R for carriage return\n"
	"\t<Return> for line feed\n"
	"\t<Tab>    for tab\n"
	"\t<ctrl>-G for bell\n"
	"\t<ctrl>-H for backspace (non overwriting)\n"
    );
    exit(2);
}

int main(int argc, char **argv)
{
    int x, y;
    char buffer[2];
    int vgamode;
    int key, retval = 0;

    if (argc != 3)
	usage();
    if (1 != sscanf(argv[1], "%d%c", &x, buffer))
	usage();
    if (1 != sscanf(argv[2], "%d%c", &y, buffer))
	usage();

    vga_init();
    vgamode = vga_getdefaultmode();
    if (vgamode == -1)
	vgamode = G320x200x256;

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
    for(;;) {
	key = vga_getch();
	if (key == 4)
	    break;
	if (key == 18)
	    key = '\r';
	buffer[0] = key;
	gl_printf(x, y, "%s", buffer);
	x = y = -1;
    }

    vga_setmode(TEXT);

    exit(retval);
}
