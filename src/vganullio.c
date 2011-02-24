#include "libvga.h"
#include <stdio.h>

int __svgalib_vganull_inmisc(void)
{
   return 0;
}

void __svgalib_vganull_outmisc(int i)
{
}

int __svgalib_vganull_incrtc(int i)
{
   return 0;
}

void __svgalib_vganull_outcrtc(int i, int d)
{
}

int __svgalib_vganull_inseq(int index)
{
    return 0;
}

void __svgalib_vganull_outseq(int index, int val)
{
}

int __svgalib_vganull_ingra(int index)
{
    return 0;
}

void __svgalib_vganull_outgra(int index, int val)
{
}

int __svgalib_vganull_inis1(void)
{
   return 0;
}

int __svgalib_vganull_inatt(int index)
{
    return 0;
}

void __svgalib_vganull_outatt(int index, int val)
{
}

void __svgalib_vganull_attscreen(int i)
{
}

void __svgalib_vganull_inpal(int i, int *r, int *g, int *b)
{
}

void __svgalib_vganull_outpal(int i, int r, int g, int b)
{
}
