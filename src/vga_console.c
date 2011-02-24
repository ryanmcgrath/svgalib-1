#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/mman.h>
#include <sys/kd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>
#include "vga.h"
#include "libvga.h"
#include "driver.h"
#include "vgapci.h"
#include "vgaio.h"
#include "mouse/vgamouse.h"
#include "keyboard/vgakeyboard.h"

extern struct termios __svgalib_text_termio;	/* text mode termio parameters     */
extern struct termios __svgalib_graph_termio;	/* graphics mode termio parameters */
int __svgalib_vc=-1;
extern int __svgalib_startup_vc;

void vga_waitvtactive() {

	if(__svgalib_novccontrol)return;
	return __svgalib_waitvtactive();
}

void __svgalib_set_graphtermio(void)
{
    /* Leave keyboard alone when rawkeyboard is enabled! */
    if (__svgalib_kbd_fd < 0) {
	/* set graphics mode termio parameters */
	ioctl(0, TCSETSW, &__svgalib_graph_termio);
    }
}


void __svgalib_set_texttermio(void)
{

    DTP((stderr,"set_texttermio\n"));
    /* Leave keyboard alone when rawkeyboard is enabled! */
    if (__svgalib_kbd_fd < 0) {
	/* restore text mode termio parameters */
	ioctl(0, TCSETSW, &__svgalib_text_termio);
    }
}


void __svgalib_disable_interrupt(void)
{
    struct termios cur_termio;

    /* Well, one could argue that sigint is not enabled at all when in __svgalib_nosigint
       but sometimes they *still* are enabled b4 graph_termio is set.. */
    ioctl(0, TCGETS, &cur_termio);
    cur_termio.c_lflag &= ~ISIG;
    ioctl(0, TCSETSW, &cur_termio);
}


void __svgalib_enable_interrupt(void)
{
    struct termios cur_termio;

    if (__svgalib_nosigint) /* do not reenable, they are often reenabled by text_termio */
	return; 
    ioctl(0, TCGETS, &cur_termio);
    cur_termio.c_lflag |= ISIG;
    ioctl(0, TCSETSW, &cur_termio);
}

/* The following is rather messy and inelegant. The only solution I can */
/* see is getting a extra free VT for graphics like XFree86 does. */

void __svgalib_waitvtactive(void)
{
    if (__svgalib_tty_fd < 0)
	return; /* Not yet initialized */

    while (ioctl(__svgalib_tty_fd, VT_WAITACTIVE, __svgalib_vc) < 0) {
	if ((errno != EAGAIN) && (errno != EINTR)) {
	    perror("ioctl(VT_WAITACTIVE)");
	    exit(1);
	}
//	usleep(150000);
    }
}

static int check_owner(int vc)
{
    struct stat sbuf;
    char fname[30];

#ifdef ROOT_VC_SHORTCUT
    if (!getuid())
        return 1;               /* root can do it always */
#endif
    sprintf(fname, "/dev/tty%d", vc);
    if ((stat(fname, &sbuf) >= 0) && (getuid() == sbuf.st_uid)) {
        return 1;
    }
    fprintf(stderr,"You must be the owner of the current console to use svgalib.\n");
    return 0;
}

void __svgalib_open_devconsole(void)
{
    struct vt_mode vtm;
    struct vt_stat vts;
    struct stat sbuf;
    char fname[30];

	if(__svgalib_novccontrol) {
        return;
    }

    if (__svgalib_tty_fd >= 0)
        return;

    /*  The code below assumes file descriptors 0, 1, and 2
     *  are already open; make sure that's true.  */
    if ((fcntl(0,F_GETFD) == -1) && (open("/dev/null", O_RDONLY) == -1)){
        perror("/dev/null");
        exit(1);
    }
    if ((fcntl(1,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)){
        perror("/dev/null");
        exit(1);
    }
    if ((fcntl(2,F_GETFD) == -1) && (open("/dev/null", O_WRONLY) == -1)){
        perror("/dev/null");
        exit(1);
    }

    /*
     * Now, it would be great if we could use /dev/tty and see what it is connected to.
     * Alas, we cannot find out reliably what VC /dev/tty is bound to. Thus we parse
     * stdin through stderr for a reliable VC
     */
    for (__svgalib_tty_fd = 0; __svgalib_tty_fd < 3; __svgalib_tty_fd++) {
        if (fstat(__svgalib_tty_fd, &sbuf) < 0)
            continue;
        if (ioctl(__svgalib_tty_fd, VT_GETMODE, &vtm) < 0)
            continue;
        if ((sbuf.st_rdev & 0xff00) != 0x400)
            continue;
        if (!(sbuf.st_rdev & 0xff))
            continue;
        __svgalib_vc = sbuf.st_rdev & 0xff;
        return;                 /* perfect */
    }

    if ((__svgalib_tty_fd = open("/dev/console", O_RDWR)) < 0) {
        fprintf(stderr,"svgalib: can't open /dev/console \n");
        exit(1);
    }
    if (ioctl(__svgalib_tty_fd, VT_OPENQRY, &__svgalib_vc) < 0)
        goto error;
    if (__svgalib_vc <= 0)
        goto error;
    sprintf(fname, "/dev/tty%d", __svgalib_vc);
    close(__svgalib_tty_fd);
    /* change our control terminal: */
    setpgid(0,getppid());
    setsid();
    /* We must use RDWR to allow for output... */
    if (((__svgalib_tty_fd = open(fname, O_RDWR)) >= 0) &&
        (ioctl(__svgalib_tty_fd, VT_GETSTATE, &vts) >= 0)) {
        if (!check_owner(vts.v_active)) {
            
            goto error;
        }
        /* success, redirect all stdios */
        if (DREP)
            fprintf(stderr,"[svgalib: allocated virtual console #%d]\n", __svgalib_vc);
        fflush(stdin);
        fflush(stdout);
        fflush(stderr);
        close(0);
        close(1);
        close(2);
        dup(__svgalib_tty_fd);
        dup(__svgalib_tty_fd);
        dup(__svgalib_tty_fd);
        /* clear screen and switch to it */
        fwrite("\e[H\e[J", 6, 1, stderr);
        fflush(stderr);
        if (__svgalib_vc != vts.v_active) {
            __svgalib_startup_vc = vts.v_active;
	    ioctl(__svgalib_tty_fd, VT_ACTIVATE, __svgalib_vc);
            __svgalib_waitvtactive();
	}
    } else {
error:
    if (__svgalib_tty_fd > 2)
	close(__svgalib_tty_fd);
    __svgalib_tty_fd = - 1;
    fprintf(stderr,"Not running in a graphics capable console,\n"
	 "and unable to find one.\n");
    }
}

