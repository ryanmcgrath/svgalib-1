/*
Linux Real Mode Interface - A library of DPMI-like functions for Linux.

Copyright (C) 1998 by Josh Vanderhoof

You are free to distribute and modify this file, as long as you
do not remove this copyright notice and clearly label modified
versions as being modified.

This software has NO WARRANTY.  Use it at your own risk.

*/

#ifndef LRMI_H
#define LRMI_H
#if defined __GLIBC__ && __GLIBC__ >= 2
#include <sys/io.h>
#endif

struct LRMI_regs
	{
	unsigned int edi;
	unsigned int esi;
	unsigned int ebp;
	unsigned int reserved;
	unsigned int ebx;
	unsigned int edx;
	unsigned int ecx;
	unsigned int eax;
	unsigned short int flags;
	unsigned short int es;
	unsigned short int ds;
	unsigned short int fs;
	unsigned short int gs;
	unsigned short int ip;
	unsigned short int cs;
	unsigned short int sp;
	unsigned short int ss;
	};

/*
 Initialize
 returns 1 if sucessful, 0 for failure
*/
int
LRMI_init(void);

/*
 Simulate a 16 bit far call
 returns 1 if sucessful, 0 for failure
*/
int
LRMI_call(struct LRMI_regs *r);

/*
 Simulate a 16 bit interrupt
 returns 1 if sucessful, 0 for failure
*/
int
LRMI_int(int interrupt, struct LRMI_regs *r);

/*
 Allocate real mode memory
 The returned block is paragraph (16 byte) aligned
*/
void *
LRMI_alloc_real(int size);

/*
 Free real mode memory
*/
void
LRMI_free_real(void *m);

#endif
