/*
   Note: Previously this program did not unlock the extended registers,
   so it only worked if the registers happened to be unlocked.

   This program sets the 'memory clock' of Cirrus 5424/26/28 cards.
   The first three values could be set by utility programs that
   came with my card (AVGA3), but somewhat higher values seem to work (on my
   card at least). It may be that better and more recent Cirrus cards use a
   higher value as boot-up default. It should depend on DRAM speed, but it
   seems to be more dependant on the card logic.

   I have the impression that many Cirrus 542x cards suffer from horrible
   BIOS version/DRAM timing misconfigurations. Perhaps even some versions of
   MS-Windows drivers change the MCLK register. In any case, the boot-up BIOS
   default (0x1c) may be inappropriately low for the type of DRAM timing most
   cards use.

   Using a higher memory clock gives a very significant performance improvement;
   with high dot clock modes (like 640x480x16M or 1150x900x256) performance can
   be more than twice that of the standard 50 MHz clock. This goes for both
   (VLB) framebuffer access and accelerated features (bitblt). This also helps
   XFree86 server performance, but only if the XFree86 Cirrus driver doesn't
   set the memory clock register (it should work for XFree86 1.3 and 2.0).
   Use at your own risk!

   Note that the 'dot clock' is something entirely different. There does not
   seem to be much correlation between the two (i.e. if a high dot clock gives
   screen problems, using a high memory clock is not likely to fix it, other
   than improving speed).

 */


#define NEW_MCLK 0x1C		/* Default value */
														 /* #define NEW_MCLK 0x26 *//* High value to test XFree86 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <vga.h>
#include <sys/io.h>	/* For port I/O macros. */
#define OUTB(a,d) outb(d,a)

int
main (void)
{
  vga_init ();
  if (vga_getcurrentchipset () != CIRRUS)
    {
      printf ("Not a Cirrus.\n");
      printf ("Continue anyway (y/n)?\n");
      if (getchar () != 'y')
	return -1;
    }

  /* Unlock extended registers. */
  OUTB (0x3c4, 0x06);
  OUTB (0x3c5, 0x12);


  /*
     Tested on VLB AVGA3 CL-GD5426 on 40MHz VLB
     0x1c         50 MHz (boot-up default)
     0x21         59
     0x22         62
     0x23 OK
     0x24 OK
     0x25 OK
     0x26 OK              [This corresponds to the highest value
     mentioned in the databook (which does
     not mean that it is supposed to work
     on all cards); the book also says the
     parts are not specified for more than
     50 MHz].
     0x27 occasional loose pixels
     0x28 occasional problems
     0x29 some problems
     0x2A 16M/textmode problems
     0x2c 256/32k color problems
   */
  OUTB (0x3c4, 0x1f);
  printf ("Old MCLK value: %02x\n", inb (0x3c5));
  OUTB (0x3c4, 0x1f);
  OUTB (0x3c5, NEW_MCLK);
  printf ("New MCLK value: %02x\n", NEW_MCLK);

  return 0;
}
