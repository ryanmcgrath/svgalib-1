#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "lrmi.h"

unsigned char * edid = NULL;

int read_edid() 
{
	int i;
        struct LRMI_regs regs;

   	if (!LRMI_init()) {
                return -1;
        }
             
        edid = LRMI_alloc_real(128);

        if ( edid == NULL )
        {
                return -1;
        }

	memset(edid, 0xed, 128);
        memset(&regs, 0, sizeof(regs));

        regs.es = (unsigned int)edid >> 4;
        regs.edi = 0;

        regs.eax = 0x4f15;
        regs.ebx = 0x01;

        ioperm(0,0x400,1);
        iopl(3);
        LRMI_int( 0x10, &regs );
        iopl(0);
        ioperm(0,0x400,0);

	if(*edid || *(edid+7)) return -2;
	for(i=1;i<=6;i++) if(*(edid+i)!=0xff) return -2;

        return regs.eax;
}


int main ( int argc, char *argv[])
{
read_edid();
fwrite(edid,128,1,stdout);
}
