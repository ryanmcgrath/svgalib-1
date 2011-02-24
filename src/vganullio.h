#include "libvga.h"

extern int __svgalib_vganull_inmisc(void);
extern void __svgalib_vganull_outmisc(int i);
extern int __svgalib_vganull_incrtc(int i);
extern void __svgalib_vganull_outcrtc(int i, int d);
extern int __svgalib_vganull_inseq(int i);
extern void __svgalib_vganull_outseq(int i, int d);
extern int __svgalib_vganull_ingra(int i);
extern void __svgalib_vganull_outgra(int i, int d);
extern int __svgalib_vganull_inis1(void);
extern int __svgalib_vganull_inatt(int i);
extern void __svgalib_vganull_outatt(int i, int d);
extern void __svgalib_vganull_attscreen(int i);
extern void __svgalib_vganull_inpal(int i, int *r, int *g, int *b);
extern void __svgalib_vganull_outpal(int i, int r, int g, int b);

