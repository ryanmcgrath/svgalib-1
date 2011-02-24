/*
Set mode to VESA mode 3 (80x25 text mode)
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <sys/stat.h>

#include "lrmi.h"

void set_vesa_mode(int mode){
	struct LRMI_regs r;
	memset(&r, 0, sizeof(r));
	r.eax = 0x4f02;
        r.ebx = mode;
	if (!LRMI_int(0x10, &r))
		{
		fprintf(stderr, "Can't set video mode (vm86 failure)\n");
		}
	}

int main(int argc, char *argv[]){
   	int i;
        

        if((argc>1)&&(!strcmp(argv[1],"-h"))){
           	printf("Usage: mode3 [ modenum [ font ] ]\n"
                       "\n"
                       "uses VESA bios to set video mode to modenum (3 by default)\n"
                       "if font is given, uses setfont to change the screen font\n\n");   	
                return 0;
        };
	if (!LRMI_init())
		return 1;
	ioperm(0, 0x400, 1);
	iopl(3);
        i=3;
        if(argc>1){
           	sscanf(argv[1],"%i",&i);
                if((i<=0))i=3;
        };
	set_vesa_mode(i);
        if(argc>2){
           	execlp("setfont",argv[2],NULL);
                return 1;
        };
	return 0;
}
