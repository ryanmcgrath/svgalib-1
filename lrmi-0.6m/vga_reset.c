/*
Call real mode c0003
*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "lrmi.h"

int main(int argc, char *argv[]){
	struct LRMI_regs r;

	if (!LRMI_init())
		return 1;
	ioperm(0, 0x400, 1);
	iopl(3);
        memset(&r,0,sizeof(r));
	r.ip=3;
	r.cs=0xc000;
	LRMI_call(&r);
	return 0;
}
