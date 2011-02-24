
#include <stdlib.h>
#include <stdio.h>
#include <vga.h>
#include "../src/driver.h"
#include <errno.h>


/*
 * Note: Observe that when writing the font to a file, the file to write is
 * opened after vga_init has been called (so that root permissions have been
 * given up). This means that there is no major security hole lurking here.
 */

unsigned char regs[MAX_REGS];

int
main (int argc, char *argv[])
{
  vga_init ();
  if (argc == 1)
    {
      printf ("Save/restore textmode registers.\n");
      printf ("Syntax: restoretextmode option filename\n");
      printf ("	-r filename	Restore registers from file.\n");
      printf ("	-w filename	Write registers to file.\n");
      return 0;
    }
  if (argv[1][0] != '-')
    {
      printf ("Must specify -r or -w.\n");
      return 1;
    }
  switch (argv[1][1])
    {
    case 'r':
    case 'w':
      if (argc != 3)
	{
	  printf ("Must specify filename.\n");
	  return 1;
	}
      break;
    default:
      printf ("Invalid option. Must specify -r or -w.\n");
      return 1;
    }
  if (argv[1][1] == 'r')
    {
      FILE *f;
      f = fopen (argv[2], "rb");
      if (f == NULL)
	{
	error:
	  perror ("restoretextmode");
	  return 1;
	}
      if (1 != fread (regs, MAX_REGS, 1, f))
	{
	  if (errno)
	    goto error;
	  puts ("restoretextmode: input file corrupted.");
	  return 1;
	}
      fclose (f);
    }
  vga_setmode (G640x350x16);
  switch (argv[1][1])
    {
    case 'r':
      vga_settextmoderegs (regs);
      break;
    case 'w':
      vga_gettextmoderegs (regs);
      break;
    }
  vga_setmode (TEXT);
  if (argv[1][1] == 'w')
    {
      FILE *f;
      f = fopen (argv[2], "wb");
      if (f == NULL)
	goto error;
      if (1 != fwrite (regs, MAX_REGS, 1, f))
	goto error;
      if (fclose (f))
	goto error;
    }
  return 0;
}
