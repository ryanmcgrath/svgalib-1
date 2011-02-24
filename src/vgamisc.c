/* VGAlib version 1.2 - (c) 1993 Tommy Frandsen                    */
/*                                                                 */
/* This library is free software; you can redistribute it and/or   */
/* modify it without any restrictions. This library is distributed */
/* in the hope that it will be useful, but without any warranty.   */

/* Multi-chipset support Copyright 1993 Harm Hanemaayer */
/* partially copyrighted (C) 1993 by Hartmut Schirmer */

#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "mouse/vgamouse.h"
#include "keyboard/vgakeyboard.h"
#include "svgalib_helper.h"

vga_cardinfo *vga_getcardinfo(void) {
    vga_cardinfo *vci;
    
    vci = malloc(sizeof(vga_cardinfo));
    if(vci==NULL) return vci;
    vci->version = 0x200;
    vci->size = sizeof(vga_cardinfo);
    vci->chipset = __svgalib_chipset;
    vci->physmem = __svgalib_linear_mem_phys_addr;
	vci->physmemsize = __svgalib_linear_mem_size;
	vci->linearmem = LINEAR_POINTER;
    
    return vci;
}

void vga_waitretrace(void)
{
#if 0
    if(!__svgalib_nohelper)
        ioctl(__svgalib_mem_fd, SVGAHELPER_WAITRETRACE, NULL);
    else
#endif
    {
        if (__svgalib_driverspecs->emul && __svgalib_driverspecs->emul->waitretrace) {
            __svgalib_driverspecs->emul->waitretrace();
        } else {
            while (!(__svgalib_inis1() & 8));
            while (__svgalib_inis1() & 8);
        }
    }
}

/*
 * The way IS_LINEAR gets indicated is rather convoluted; if the driver
 * is CAPABLE_LINEAR, setlinearaddressing will enable
 * the flag in __svgalib_linearset which gets set in the modeinfo by
 * vga_getmodeinfo(). The driver must turn off the flag in
 * __svgalib_linearset if linear addressing gets disabled (e.g. when
 * setting another mode).
 * 
 * For any driver, the chipset getmodeinfo flag can examine a hardware
 * register and set the IS_LINEAR flag if linear addressing is enabled.
 */

unsigned char *
 vga_getgraphmem(void)
{

    DTP((stderr,"getgraphmem\n"));

    if (__svgalib_modeinfo_linearset & IS_LINEAR )
		return LINEAR_POINTER;
    
	return GM;
}

/*
 * This function is to be called after a SVGA graphics mode set
 * in banked mode. Probing in VGA-compatible textmode is not a good
 * idea.
 */

/* cf. vga_waitretrace, M.Weller */
int vga_setlinearaddressing(void)
{
    int (*lfn) (int op, int param) = __svgalib_driverspecs->linear;
    vga_modeinfo *modeinfo;

    DTP((stderr,"setlinearaddressing\n"));

    modeinfo = vga_getmodeinfo(CM);
    if (!(modeinfo->flags&CAPABLE_LINEAR)) return -1;

    (*lfn) (LINEAR_ENABLE, 0);

    if (LINEAR_POINTER==NULL) {
		/* Shouldn't happen. */
		(*lfn) (LINEAR_DISABLE, 0);
    	__svgalib_modeinfo_linearset &= ~(IS_LINEAR);
		return -1;
    }
    
    __svgalib_modeinfo_linearset |= IS_LINEAR;

    graph_mem = LINEAR_POINTER;

    return __svgalib_linear_mem_size; /* Who cares? */
}

#if 1

/*
 * The other code doesn't work under Linux/Alpha (I think
 * it should).  For now, this is a quick work-around).
 */

int vga_getkey(void)
{
    struct timeval tv;
    fd_set fds;
    int fd = fileno(stdin);
    char c;

    tv.tv_sec = tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    if (select(fd + 1, &fds, 0, 0, &tv) > 0) {
	if (read(fileno(stdin), &c, 1) != 1) {
	    return 0;
	}
	return c;
    }
    return 0;
}

#else

int vga_getkey(void)
{
    struct termio zap, original;
    int e;
    char c;

    ioctl(fileno(stdin), TCGETA, &original);	/* Get termio */
    zap = original;
    zap.c_cc[VMIN] = 0;		/* Modify termio  */
    zap.c_cc[VTIME] = 0;
    zap.c_lflag = 0;
    ioctl(fileno(stdin), TCSETA, &zap);		/* Set new termio */
    e = read(fileno(stdin), &c, 1);	/* Read one char */
    ioctl(fileno(stdin), TCSETA, &original);	/* Restore termio */
    if (e != 1)
	return 0;		/* No key pressed. */
    return c;			/* Return key. */
}

#endif

int vga_waitevent(int which, fd_set * in, fd_set * out, fd_set * except,
		  struct timeval *timeout)
{
    fd_set infdset;
    int fd, retval;

    if (!in) {
	in = &infdset;
	FD_ZERO(in);
    }
    fd = __svgalib_mouse_fd;	/* __svgalib_mouse_fd might change on
				   vc switch!! */
    if ((which & VGA_MOUSEEVENT) && (fd >= 0))
	FD_SET(fd, in);
    if (which & VGA_KEYEVENT) {
        if(__svgalib_novccontrol) {
            fd=fileno(stdin);
            FD_SET(fd, in);
        } else  {
	    fd = __svgalib_kbd_fd;
	    if (fd >= 0) {		/* we are in raw mode */
	        FD_SET(fd, in);
	    } else {
	        FD_SET(__svgalib_tty_fd, in);
	    }
        }
    }
    if (select(FD_SETSIZE, in, out, except, timeout) < 0)
	return -1;
    retval = 0;
    fd = __svgalib_mouse_fd;
    if ((which & VGA_MOUSEEVENT) && (fd >= 0)) {
	if (FD_ISSET(fd, in)) {
	    retval |= VGA_MOUSEEVENT;
	    FD_CLR(fd, in);
	    mouse_update();
	}
    }
    if (which & VGA_KEYEVENT) {
        if(__svgalib_novccontrol) {
            fd=fileno(stdin);
            if(FD_ISSET(fd, in)) {
		FD_CLR(fd, in);
		retval |= VGA_KEYEVENT;
            }
        } else  {
	    fd = __svgalib_kbd_fd;
	    if (fd >= 0) {		/* we are in raw mode */
	        if (FD_ISSET(fd, in)) {
		    FD_CLR(fd, in);
		    retval |= VGA_KEYEVENT;
		    keyboard_update();
	        }
	    } else if (FD_ISSET(__svgalib_tty_fd, in)) {
	        FD_CLR(__svgalib_tty_fd, in);
	        retval |= VGA_KEYEVENT;
	    }
        }
    }
    return retval;
}

