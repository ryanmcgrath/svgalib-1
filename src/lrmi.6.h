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

#define _SVGALIB_LRMI
#define LRMI_regs vm86_regs
#include "vga.h"

extern LRMI_callbacks __svgalib_default_LRMI_callbacks;
extern LRMI_callbacks * __svgalib_LRMI_callbacks;

#endif
