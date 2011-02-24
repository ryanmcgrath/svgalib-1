/*
   This file assumes 132 column textmode :-).

   This program tries to fix problems with extended textmodes on some cards. The problem is that for 132x43 textmode, some
   BIOSes set the vertical display end register to 349 (350), instead of 343 (344 = 3 * 8 scanlines). Because in Linux textmode
   video memory is usually filled with old text that has already scrolled away (this includes the area below the 43rd textmode
   line, which changes when the console scrolls), the top half of a constantly changing irrelevant text line is visible
   at the bottom of the screen, which is very annoying.

   This program sets the VGA Vertical Display End register to the proper value.

   The problem is at least present in the BIOS of most Cirrus Logic 542x based cards, and some WD90C03x based cards.


   You can also change the number scanlines and number of lines per character of 132x43 textmode to improve readability.


   VGA CRTC 0x12        bits 0-7        Vertical Display End Register bits 0-7
   VGA CRTC 0x07        bit 1           Vertical Display End Register bit 8

   Cirrus 542x BIOS v1.10 132x43:       0x15d (349)     (this is the wrong value that the BIOS sets)
   Correct value for 132x43:    0x157 (343)
   Correct value for 132x44:    0x15f (351)


   VGA CRTC 0x09        bits 0-4        Number of scanlines in a textmode font character.
   VGA MiscOut  bits 6-7        Indicates number of scanlines: 1 = 400, 2 = 350, 3 = 480.
   VGA CRTC 0x06   bits 0-7     Vertical Total register bits 0-7 (should be #scanlines - 2)
   Bit 8 is in VGA CRTC 0x07, bit 0
   Bit 9 is in VGA CRTC 0x07, bit 5
   VGA CRTC 0x10        bits 0-7        Vertical Retrace Start
   Bit 8 is in VGA CRTC 0x07, bit 2
   Bit 9 is in VGA CRTC 0x07, bit 7
   VGA CRTC 0x11        bits 0-3        Vertical Retrace End (last 4 bits)
   VGA CRTC 0x15        bits 0-7        Vertical Blank Start
   Bit 8 is in VGA CRTC 0x07 bit 3

 */


/* Comment this out if setting graphics modes is undesirable. You can load proper fonts with restorefont. */
#define FIX_FONT


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <vga.h>
#include "sys/io.h"	/* For port I/O macros. */
#define OUTB(a,d) outb(d,a)

static void fixfont (int);

int
main (int argc, char *argv[])
{
  int vgaIOBase;
  unsigned char val;
  int lines;

  vga_disabledriverreport ();
  vga_setchipset (VGA);
  vga_init ();

  if (argc == 1)
    {
      printf ("Fiddle with 132x43 textmode VGA registers.\n");
      printf ("Syntax: fix132x43 option (one at a time).\n");
      printf ("	-f	Fix problem of annoying changing line of text at bottom of screen.\n");
      printf ("	-v	Switch to 9 line characters (400 line frame, 70 Hz).\n");
#ifdef INCLUDE_480LINEMODE
      printf ("	-w	Switch to 11 line characters (480 line frame, 60 Hz).\n");
#endif
      printf ("	-r	Switch to 8 line characters again (350 line frame, 70 Hz).\n");
      printf ("LINES environment variable is used to detect 43 or 44 line console.\n");
      return 0;
    }

  if (argv[1][0] != '-')
    {
      printf ("Must specify -f, -v or -r.\n");
      return 1;
    }

  if (argv[1][1] != 'f' && argv[1][1] != 'v' && argv[1][1] != 'w' && argv[1][1] != 'r')
    {
      printf ("Must specify -f, -v, or -r.\n");
      return 1;
    }

  lines = atoi (getenv ("LINES"));
  printf ("Lines: %d\n", lines);

  /* Deprotect CRT registers 0-7. */
  vgaIOBase = (inb (0x3cc) & 0x01) ? 0x3D0 : 0x3B0;
  OUTB (vgaIOBase + 4, 0x11);
  val = inb (vgaIOBase + 5);
  OUTB (vgaIOBase + 5, val & 0x7f);

  if (argv[1][1] == 'f')
    {
      /* Fix stupid bottom line. */
      OUTB (vgaIOBase + 4, 0x12);
      OUTB (vgaIOBase + 5, 0x57);	/* Value for 43 lines (343). */
    }

  if (argv[1][1] == 'r')
    {
      /* Set 8 line characters, 350 line frame (344 used). */

      OUTB (vgaIOBase + 4, 0x09);
      val = inb (vgaIOBase + 5);
      OUTB (vgaIOBase + 5, (val & 0xe0) | 7);	/* Set 8-line characters. */

      val = inb (0x3cc);
      OUTB (0x3c2, (val & 0x3f) | (2 << 6));	/* 350 scanlines. */

      OUTB (vgaIOBase + 4, 0x12);	/* Vertical Display End */
      if (lines == 44)
	OUTB (vgaIOBase + 5, 0x60);	/* Value for 44 lines (352). */
      else
	OUTB (vgaIOBase + 5, 0x57);	/* Value for 43 lines (343). */

      OUTB (vgaIOBase + 4, 0x10);	/* Retrace Start */
      OUTB (vgaIOBase + 5, 0x83);	/* Value for 350 line frame. */

      OUTB (vgaIOBase + 4, 0x11);	/* Retrace End */
      val = inb (vgaIOBase + 5);
      OUTB (vgaIOBase + 5, (val & 0xf0) | 0x05);

      OUTB (vgaIOBase + 4, 0x15);	/* Vertical Blank Start */
      OUTB (vgaIOBase + 5, 0x63);	/* Value for 350 line frame. */
    }

  if (argv[1][1] == 'v')
    {
      /* Set 9 line characters, 400 line frame (387 used). */

      OUTB (vgaIOBase + 4, 0x09);
      val = inb (vgaIOBase + 5);
      OUTB (vgaIOBase + 5, (val & 0xe0) | 8);	/* Set 9-line characters. */

      val = inb (0x3cc);
      OUTB (0x3c2, (val & 0x3f) | (1 << 6));	/* 400 scanlines. */

      OUTB (vgaIOBase + 4, 0x12);
      if (lines == 44)
	OUTB (vgaIOBase + 5, 0x8b);	/* End scanline is 44 * 9 - 1 = 395 */
      else
	OUTB (vgaIOBase + 5, 0x82);	/* End scanline is 43 * 9 - 1 = 386 */

      OUTB (vgaIOBase + 4, 0x10);	/* Retrace Start */
      OUTB (vgaIOBase + 5, 0x9c);	/* Value for 400 line frame. */

      OUTB (vgaIOBase + 4, 0x11);	/* Retrace End */
      val = inb (vgaIOBase + 5);
      OUTB (vgaIOBase + 5, (val & 0xf0) | 0x0e);

      OUTB (vgaIOBase + 4, 0x15);	/* Vertical Blank Start */
      OUTB (vgaIOBase + 5, 0x95);	/* Value for 400 line frame. */

#ifdef FIX_FONT
      fixfont (9);
#endif
    }

#ifdef INCLUDE_480LINEMODE
  if (argv[1][1] == 'w')
    {
      /* Set 11 line characters, 480 line frame (473 used). */

      OUTB (vgaIOBase + 4, 0x09);
      val = inb (vgaIOBase + 5);
      OUTB (vgaIOBase + 5, (val & 0xe0) | 10);	/* Set 11-line characters. */

      OUTB (0x3c2, 0xeb);

      OUTB (vgaIOBase + 4, 0x12);
      OUTB (vgaIOBase + 5, 0xd8);	/* End scanline is 43 * 11 - 1 = 472 */

      OUTB (vgaIOBase + 4, 0x10);	/* Retrace Start */
      OUTB (vgaIOBase + 5, 0xf4 /*0xea */ );	/* Value for 480 line frame. ?? */

      OUTB (vgaIOBase + 4, 0x11);	/* Retrace End */
      val = inb (vgaIOBase + 5);
      OUTB (vgaIOBase + 5, (val & 0xf0) | 0x0c);

      OUTB (vgaIOBase + 4, 0x15);	/* Vertical Blank Start */
      OUTB (vgaIOBase + 5, 0xf4 /*0xe7 */ );	/* Value for 480 line frame. */

#ifdef FIX_FONT
      fixfont (11);
#endif
    }
#endif

  return 0;
}



/* Clear offsets 8-31 of each character bitmap (the BIOS usually leaves some trash here from the 16 line font that was loaded */
/* prior to setting 132x43 (8 line font) textmode). */

static void
fixfont (int lines)
{
  unsigned char font[8192];
  int i;
  vga_setmode (G640x480x16);
  vga_gettextfont (font);

  for (i = 0; i < 256; i++)
    memset (font + i * 32 + 8, 0, 32 - 8);	/* Clear remaining part of character bitmap buffer. */

  vga_puttextfont (font);
  vga_setmode (TEXT);
}
