/*
**  tseng3.c - get ET4000 graphics register values
**  Copyright (C) 1993  Tommy Frandsen, Harm Hanemaayer, Hartmut Schirmer
**
**  This program is free software;
**
**  Permission is granted to any individual or institution to use, copy,
**  or redistribute this executable so long as it is not modified and
**  that it is not sold for profit.
**
**  LIKE ANYTHING ELSE THAT'S FREE, TSENG3 IS PROVIDED AS IS AND
**  COMEs WITH NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED.
**  IN NO EVENT WILL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DAMAGES
**  RESULTING FROM THE USE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <stdarg.h>

/* VGA index register ports */
#define CRT_I   0x3D4   /* CRT Controller Index (mono: 0x3B4) */
#define ATT_IW  0x3C0   /* Attribute Controller Index & Data Write Register */
#define GRA_I   0x3CE   /* Graphics Controller Index */
#define SEQ_I   0x3C4   /* Sequencer Index */
#define PEL_IW  0x3C8   /* PEL Write Index */

/* VGA data register ports */
#define CRT_D   0x3D5   /* CRT Controller Data Register (mono: 0x3B5) */
#define ATT_R   0x3C1   /* Attribute Controller Data Read Register */
#define GRA_D   0x3CF   /* Graphics Controller Data Register */
#define SEQ_D   0x3C5   /* Sequencer Data Register */
#define MIS_R   0x3CC   /* Misc Output Read Register */
#define MIS_W   0x3C2   /* Misc Output Write Register */
#define IS1_R   0x3DA   /* Input Status Register 1 (mono: 0x3BA) */
#define PEL_D   0x3C9   /* PEL Data Register */

/* VGA indexes max counts */
#define CRT_C   24              /* 24 CRT Controller Registers */
#define ATT_C   21              /* 21 Attribute Controller Registers */
#define GRA_C   9               /* 9  Graphics Controller Registers */
#define SEQ_C   5               /* 5  Sequencer Registers */
#define MIS_C   1               /* 1  Misc Output Register */
#define EXT_C	17		/* 11 SVGA Extended Registers */

/* VGA registers saving indexes */
#define CRT     0               /* CRT Controller Registers start */
#define ATT     CRT+CRT_C       /* Attribute Controller Registers start */
#define GRA     ATT+ATT_C       /* Graphics Controller Registers start */
#define SEQ     GRA+GRA_C       /* Sequencer Registers */
#define MIS     SEQ+SEQ_C       /* General Registers */
#define EXT     MIS+MIS_C       /* SVGA Extended Registers */
#define TOTAL	EXT+EXT_C	/* # or registers values */

unsigned char vga_regs[TOTAL];
static int do_all = 0;
FILE *output = NULL;

static int save_extregs;

void port_out(unsigned char value, unsigned short port)
{
   asm mov dx,port
   asm mov al,value
   asm out dx,al
}


unsigned char port_in(unsigned short port)
{
    asm mov dx,port
    asm in al,dx
    return (_AL);
}

int dactype(void)
{
	union REGS cpu_regs;

	cpu_regs.x.ax=0x10F1;
	cpu_regs.x.bx=0x2EFF;

	int86(0x10, &cpu_regs, &cpu_regs);
	if (cpu_regs.x.ax != 0x0010)
		return (-1);
	else
		return (cpu_regs.h.bl);
}

int read_dac(int addr)
{
	inp(0x3c8);
	inp(0x3c6);
        inp(0x3c6);
        inp(0x3c6);
        inp(0x3c6);
        while (addr)
                --addr,inp(0x3c6);
        addr = inp(0x3c6);
        return addr;
}

int write_dac(int addr,int value)
{
	inp(0x3c8);
	inp(0x3c6);
        inp(0x3c6);
        inp(0x3c6);
        inp(0x3c6);
	while (addr)
		--addr,inp(0x3c6);
        outp(0x3c6,value);
	return read_dac(addr);
}

int read_ext_dac(int addr)
{
	write_dac(1,addr);
	write_dac(2,0);
	return read_dac(3);
}

void puts2(char *s) {
  if (output != NULL)
    fprintf(output, "%s\n", s);
  while (*s != '\0') {
    if (*s == '\n') putch('\r');
    putch (*(s++));
  }
  putch('\r'); putch('\n');
}

int printf2(char *fmt, ...)
{
  va_list  argptr;
  char str[140], *s;
  int cnt;

  va_start( argptr, fmt );

  if (output != NULL)
    vfprintf(output, fmt, argptr);
  cnt = vsprintf( str, fmt, argptr );
  s = str;
  while (*s) {
    switch (*s) {
      case '\n' : putch('\r');
		  break;
      case '\t' : cprintf("%*s", 8-((wherex()-1)&7), "");
		  ++s;
		  continue;
    }
    putch(*(s++));
  }
  va_end( argptr );
  return( cnt );
}

void get_dac(void)
{
    int dac;

    dac = dactype();
    switch (dac)
    {
	case -1:
		printf2 ("/* Dac detection BIOS call returned an error */\n");
		break;
	case 0:
		printf2 ("/* Standard VGA dac detected */\n");
		printf2 ("#define DAC_TYPE 0\n");
		break;
	case 1:
		printf2 ("/* Sierra SC1148x HiColor dac detected */\n");
		printf2 ("#define DAC_TYPE 1\n");
		break;
	case 2:
		printf2 ("/* Diamond Speedstar 24 24bit dac or Sierra Mark2/Mark3 dac detected */\n");
		break;
	case 3:
		printf2 ("/* AT&T ATT20c491/2 15/16/24 bit dac detected */\n");
		printf2 ("#define DAC_TYPE 9\n");
		break;
	case 4:
		printf2 ("/* AcuMos ADAC1 15/16/24 bit dac detected */\n");
		break;
	case 8:
		printf2 ("/* Music 15/16/24 bit dac (AT&T compatible) detected */\n");
		printf2 ("#define DAC_TYPE 9\n");
		break;
	default: {	/* use alternate method */
		int cmd = read_dac(0);
		write_dac(0,cmd | 0x10);
		if (read_ext_dac(0) == 0x44) {
			printf2 ("/* SGS-Thomson STG170x 15/16/24 dac detected */\n");
			save_extregs = 1;
		}
		else
			printf2 ("/* Unknown HiColor dac (%d) detected */\n", dac);
		break;
	}
    }
}

#define TickTime (1.0/1193182.0)

#define init_timer() do { \
  asm mov al, 034h;       \
  asm out 043h, al;       \
  asm xor al, al;         \
  asm out 040h, al;       \
  asm out 040h, al;       \
} while(0)

#define ReadTimer(dst) do { \
       asm { mov   al, 4;   \
	     out   43h, al; \
	     in    al, 40h; \
	     mov   bl, al;  \
	     in    al, 40h; \
	     mov   ah, al;  \
	     mov   al, bl;  \
	     not   ax     } \
      (dst) = _AX;	    \
    } while (0);

#define _wait_(ID,VAL)  \
w_##ID##1:              \
  asm in    al, dx;     \
  asm test  al, VAL;    \
  asm jne   w_##ID##1;  \
w_##ID##2:              \
  asm in    al, dx;     \
  asm test  al, VAL;    \
  asm je    w_##ID##2

#define wait_horizontal(ID,port)  do { _DX=(port); _wait_(ID##ch,8);} while(0)
#define wait_vertical(ID,port)    do { _DX=(port); _wait_(ID##cv,1);} while(0)
#define __loop__(ID,port,loops,msk)     do { \
		register int cnt = (loops);  \
		_DX=(port);         	     \
		do {                         \
		  _wait_(ID,msk);            \
		} while (--cnt > 0);         \
	} while(0)
#define loop_vertical(ID,port,loops)    __loop__(ID##cv,(port),(loops),1)


int interlaced(void) {
  return (vga_regs[EXT+5] & 0x80) ? 1 : 0;
}

int calc_vtotal(void) {
  int total;

  total = (vga_regs[EXT+5]&2) ? 1024 : 0;
  switch (vga_regs[CRT+7]&0x21) {
    case 0x01 : total += 256; break;
    case 0x20 : total += 512; break;
    case 0x21 : total += 768; break;
  }
  total += vga_regs[CRT+6];
  return total + 2;
}

double measure_horizontal(void)
{
  short start, stop;
  long  diff;

  disable();
  init_timer();
  wait_horizontal(mv0,0x3da);
  wait_vertical(mv0,0x3da);
  ReadTimer(start);
  loop_vertical(mv, 0x3da, 200);
  ReadTimer(stop);
  enable();
  diff = stop-start;
  if (diff < 0) diff += 65536L;
  return 200/(TickTime*((double)diff));
}

#define ASK_CONT 0
#define ASK_SKIP 1
#define ASK_ALL  2

int ask(char *expl)
{
    int ch;

    cprintf("\r\n\n"
	    "ษออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออป\r\n"
	    "บ WARNING: Improper use of this program may destroy your monitor บ\r\n"
	    "ศออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออออผ\r\n\n"
	    "%s\r\n\n"
	    "Type SPACE to skip, ESC to cancel program, RETURN to continue ... ",
	    expl);
    do {
      ch = getch();
      if (ch == 27) exit(1);
      switch (ch) {
	case 27  : exit(1);
	case ' ' : return ASK_SKIP;
	case 'A' : return ASK_ALL;
	case '\r': return ASK_CONT;
      }
    } while (1);
}


void get_clocks(void)
{
    union REGS cpu_regs;
    unsigned char temp;
    int i;
    double clock[8], horzbase, baseosc;

    if (!do_all)
      switch (ask("Measure clock frequencies")) {
	case ASK_SKIP : return;
	case ASK_ALL  : do_all = 1; break;
	default       : break;
      }

    cpu_regs.h.ah = 0x00;
    cpu_regs.h.al = 0x13;
    int86(0x10, &cpu_regs, &cpu_regs);
    port_out(0x03, 0x3BF);  /* Unprotect ET4000 registers */
    port_out(0xA0, 0x3D8);
    port_out(0x00, ATT_IW); /* disable video */
    port_out(7, SEQ_I);     /* devide by 4 */
    temp = port_in(SEQ_D);
    port_out(temp | 0x41, SEQ_D);

    baseosc = 25175;		/* Base frequency */
    switch (temp & 0x41) {
      case 0x41 : baseosc *= 4; break;
      case 0x40 : baseosc *= 2; break;
    }
    clock[0] = baseosc;
    clock[1] = baseosc * 28322.0 / 25175.0;
    horzbase = measure_horizontal();
    for (i=2; i < 8; ++i) {
      temp = (port_in(MIS_R) & 0xF3) | ((i&3)<<2);
      port_out(temp, MIS_W);
      port_out(0x34, CRT_I);
      temp = (port_in(CRT_D) & 0xFD) | ((i&4)>>1);
      port_out(temp, CRT_D);
      clock[i] = baseosc*measure_horizontal()/horzbase;
    }

    cpu_regs.h.ah = 0x00;
    cpu_regs.h.al = 0x03;
    int86(0x10, &cpu_regs, &cpu_regs);

    printf2("\n#define CLOCK_VALUES {\t\\\n");
    for (i = 0; i < 8; ++i)
      printf2("\t/* %d */ %5.0f%s\t\\\n", i, clock[i],(i==7?"":","));
    printf2("\t}\n");
}

void one_mode(int vesa_mode, int mode, int colbits, int funny_card)
{
    union REGS cpu_regs;
    int i, skip = 0, modeok = 0;
    int x=0, y, col, size;
    char resol[100], colors[100], *desc;
    char   *verttxt;
    double vertical, horizontal;

    switch (mode) {
	case 0x13: x =  320; y =  200; break;
	case 0x25: x =  640; y =  480; break;
	case 0x29: x =  800; y =  600; break;
	case 0x2d: x =  640; y =  350; break;
	case 0x2e: x =  640; y =  480; break;
	case 0x2f: x =  640; y =  400; break;
	case 0x30: x =  800; y =  600; break;
	case 0x37: x = 1024; y =  768; break;
	case 0x38: x = 1024; y =  768; break;
	case 0x3d: x = 1280; y = 1024; break;
	default  : switch (vesa_mode) {
		     case 0x107:
		     case 0x119:
		     case 0x11a:
		     case 0x11b: x = 1280; y = 1024; break;
		     default   : return;
		   }
		   break;
    }
    col = (1<<colbits);
    size = (int)((((unsigned long) x) * ((unsigned long) y)) >> 10);
    if (col == 16) size >>= 2;

    switch (colbits) {
	case 15: size*=2; strcpy(colors, "32K"); desc = "HiColor ";   break;
	case 16: size*=2; strcpy(colors, "64K"); desc = "HiColor ";   break;
	case 24: size*=3; strcpy(colors, "16M"); desc = "TrueColor "; break;
	default: sprintf(colors, "%d", col); desc = ""; break;
    }
    sprintf(resol, "%dx%dx%s", x, y, colors);

    if (!do_all) {
      char expl[100];
      sprintf(expl, "BIOS mode 0x%2X : %s", mode, resol);
      switch(ask(expl)) {
	case ASK_SKIP : skip = 1; break;
	case ASK_ALL  : do_all = 1; break;
	default       : break;
      }
    }

    if (!skip) {
      /* First try VESA mode */
      if (vesa_mode != 0) {
	cpu_regs.x.ax = 0x4f02;
	cpu_regs.x.bx = vesa_mode;
	int86(0x10, &cpu_regs, &cpu_regs);
	modeok = (cpu_regs.x.ax == 0x004f);
      }
      /* Try std ET4000 mode numbers if VESA failed */
      if (!modeok && mode != 0) {
	switch (colbits)
	{
	    case 15:
	    case 16:
		cpu_regs.x.ax=0x10F0;
		cpu_regs.h.bl=mode;
		break;
	    case 24:
		switch (funny_card)
		{
		    case 1:
			cpu_regs.x.ax=0x10E0;
			cpu_regs.h.bl=0x2e;
			break;
		    case 2:
			cpu_regs.x.ax=0x10f0;
			cpu_regs.h.bl=0x3e;
			break;
		    case 0:
		    default:
			cpu_regs.x.ax=0x10F0;
			cpu_regs.h.bh=mode;
			cpu_regs.h.bl=0xFF;
			break;
		}
		break;
	    case 0:
	    default:
		cpu_regs.h.ah = 0x00;
		cpu_regs.h.al = mode;
		break;
	}

	int86(0x10, &cpu_regs, &cpu_regs);
	modeok = !(  (colbits > 8 && cpu_regs.x.ax != 0x0010)
		   ||(colbits <=8 && cpu_regs.x.ax == mode  ) );
	vesa_mode = 0;
      }

      if (modeok) {
	/* get VGA register values */
	for (i = 0; i < CRT_C; i++) {
	    port_out(i, CRT_I);
	    vga_regs[CRT+i] = port_in(CRT_D);
	}
	for (i = 0; i < ATT_C; i++) {
	    if (col == 16 && i < 16)
	      vga_regs[ATT+i] = i;
	    else {
	      port_in(IS1_R);
	      port_out(i, ATT_IW);
	      vga_regs[ATT+i] = port_in(ATT_R);
	    }
	}
	for (i = 0; i < GRA_C; i++) {
	    if (i==1 && col == 16)
	      vga_regs[GRA+i] = 0x0F;
	    else {
	      port_out(i, GRA_I);
	      vga_regs[GRA+i] = port_in(GRA_D);
	    }
	}
	for (i = 0; i < SEQ_C; i++) {
	    port_out(i, SEQ_I);
	    vga_regs[SEQ+i] = port_in(SEQ_D);
	}
	vga_regs[MIS] = port_in(MIS_R);

	/* get extended CRT registers */
	for (i = 0; i < 8; i++) {
	     port_out(0x30+i, CRT_I);
	     vga_regs[EXT+i] = port_in(CRT_D);
	}
	port_out(0x3f	, CRT_I);
	vga_regs[EXT+8] = port_in(CRT_D);

	/* get extended sequencer register */
	port_out(7, SEQ_I);
	vga_regs[EXT+9] = port_in(SEQ_D);

	/* get some other ET4000 specific registers */
	vga_regs[EXT+10] = port_in(0x3c3);
	vga_regs[EXT+11] = port_in(0x3cd);

	/* get extended attribute register */
	port_in(IS1_R);   /* reset flip flop */
	port_out(0x16, ATT_IW);
	vga_regs[EXT+12] = port_in(ATT_R);

	if (save_extregs) {	   /* copy stg170x extended info */
		vga_regs[EXT+13] = read_dac(0);
		write_dac(0,vga_regs[EXT+13] | 0x10);
		vga_regs[EXT+14] = read_ext_dac(3);
		vga_regs[EXT+15] = read_ext_dac(4);
		vga_regs[EXT+16] = read_ext_dac(5);
	}

	/* Measure video timing */
	horizontal = measure_horizontal();
	/* switch back to text mode */
	cpu_regs.h.ah = 0x00;
	cpu_regs.h.al = 0x03;
	int86(0x10, &cpu_regs, &cpu_regs);
      }
    }

    if (vesa_mode)
      printf2("\n/* VESA %smode 0x%03X", desc, vesa_mode);
    else
      printf2("\n/* ET4000 %sBIOS mode 0x%02X", desc, mode);
    printf2(" -- %s", resol);
    if ( skip || !modeok) {
	printf2(" : NOT SUPPORTED */\n");
	if (size <= 1024)
	  printf2("#define g%s_regs DISABLE_MODE\n", resol);
	return;
    }
    printf2(" */\n");

    /* can't always treat 15 and 16 bit modes the same! */
    if (colbits == 16 && !save_extregs) {
      printf2("#define g%s_regs g%dx%dx32K_regs\n", resol, x, y);
      return;
    }

    verttxt = interlaced() ? " (interlaced)" : "";
    vertical   = horizontal / calc_vtotal();
    printf2("/* Video timing:\tVertical frequency   : %4.1fHz%s\n",
						vertical, verttxt);
    printf2("\t\t\tHorizontal frequency : %4.1fKHz  */\n", horizontal/1000.0);
    printf2("static unsigned char g%s_regs[%d] = {\n  ", resol, TOTAL);
    for (i = 0; i < CRT_C; i++)
	printf2("%s0x%02X,", (i==12?"\n  ":""), vga_regs[CRT+i]);
    printf2("\n  ");
    for (i = 0; i < ATT_C; i++)
	printf2("%s0x%02X,", (i==12?"\n  ":""), vga_regs[ATT+i]);
    printf2("\n  ");
    for (i = 0; i < GRA_C; i++)
	printf2("0x%02X,",vga_regs[GRA+i]);
    printf2("\n  ");
    for (i = 0; i < SEQ_C; i++)
	printf2("0x%02X,",vga_regs[SEQ+i]);
    printf2("\n  0x%02X,",vga_regs[MIS]);
    printf2("\n  0x%02X",vga_regs[EXT]);
    for (i = 1; i < EXT_C; i++)
	printf2(",%s0x%02X", (i==12?"\n  ":""), vga_regs[i+EXT]);
    printf2("\n};\n");
}

void usage(void) {
  cputs("tseng3 <output_file> [SS24]");
  exit(1);
}

void main(int argc, char* argv[])
{
    int funny_card = 0;

    if (argc >= 2)
      output = fopen(argv[1], "w");

    if (output != NULL)
      fputs("/*\n   ( File generated by tseng3.exe )\n\n", output);
    puts2("tseng3 v1.2, Copyright (C) 1993  Tommy Frandsen, Harm Hanemaayer");
    puts2("and Hartmut Schirmer\n");
    puts2("Permission is granted to any individual or institution to use, copy, or");
    puts2("redistribute this executable so long as it is not modified and that it is");
    puts2("not sold for profit.\n");
    puts2("LIKE ANYTHING ELSE THAT'S FREE, TSENG3 IS PROVIDED AS IS AND COMES WITH");
    puts2("NO WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED. IN NO EVENT WILL");
    puts2("THE COPYRIGHT HOLDERS BE LIABLE FOR ANY DAMAGES RESULTING FROM THE USE OF");
    puts2("THIS SOFTWARE.");
    if (output != NULL)
      fputs("*/\n", output);
    puts2("");

    if (argc >= 2 && output == NULL)
	usage();

    if (argc>=3)
	funny_card = (strcmp("SS24", argv[2]) == 0) ? 1 : 2;

    get_dac();
    get_clocks();

    one_mode(0x10d, 0x13, 15, funny_card);  /* 320x200x32K */
    one_mode(0x10e, 0x13, 16, funny_card);  /* 320x200x64K */
    one_mode(0x10f, 0x13, 24, funny_card);  /* 320x200x16M */
    one_mode(0x101, 0x2e,  8, funny_card);  /* 640x480x256 */
    one_mode(0x110, 0x2e, 15, funny_card);  /* 640x480x32K */
    one_mode(0x111, 0x2e, 16, funny_card);  /* 640x480x64K */
    one_mode(0x112, 0x2e, 24, funny_card);  /* 640x480x16M */
    one_mode(0x102, 0x29,  4, funny_card);  /* 800x600x16 */
    one_mode(0x103, 0x30,  8, funny_card);  /* 800x600x256 */
    one_mode(0x113, 0x30, 15, funny_card);  /* 800x600x32K */
    one_mode(0x114, 0x30, 16, funny_card);  /* 800x600x64K */
    one_mode(0x115, 0x30, 24, funny_card);  /* 800x600x16M */
    one_mode(0x104, 0x37,  4, funny_card);  /* 1024x768x16 */
    one_mode(0x105, 0x38,  8, funny_card);  /* 1024x768x256 */
    one_mode(0x116, 0x38, 15, funny_card);  /* 1024x768x32K */
    one_mode(0x117, 0x38, 16, funny_card);  /* 1024x768x64K */
    one_mode(0x118, 0x38, 24, funny_card);  /* 1024x768x16M */
    one_mode(0x106, 0x3D,  4, funny_card);  /* 1280x1024x16 */
    one_mode(0x107, 0x00,  8, funny_card);  /* 1280x1024x256 */
    one_mode(0x119, 0x00, 15, funny_card);  /* 1280x1024x32K */
    one_mode(0x11a, 0x00, 16, funny_card);  /* 1280x1024x64K */
    one_mode(0x11b, 0x00, 24, funny_card);  /* 1280x1024x16M */
    puts2("\n/* --- ET4000 specific modes */\n#ifdef _DYNAMIC_ONLY_");
    one_mode(0x000, 0x2d,  8, funny_card);  /* 640x350x256 */
    one_mode(0x000, 0x2d, 15, funny_card);  /* 640x350x32K */
    one_mode(0x000, 0x2d, 16, funny_card);  /* 640x350x64K */
    one_mode(0x000, 0x2d, 24, funny_card);  /* 640x350x16M */
    one_mode(0x100, 0x2f,  8, funny_card);  /* 640x400x256 */
    one_mode(0x000, 0x2f, 15, funny_card);  /* 640x400x32K */
    one_mode(0x000, 0x2f, 16, funny_card);  /* 640x400x64K */
    one_mode(0x000, 0x2f, 24, funny_card);  /* 640x400x16M */
    puts2("\n#endif /* defined(_DYNAMIC_ONLY_ALL_) */\n");
}

