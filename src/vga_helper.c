#include <sys/io.h>
#include <sys/ioctl.h>
#include "svgalib_helper.h"
#include "libvga.h"

void __svgalib_port_rep_outb(unsigned char* string, int length, int port)
{
  if(__svgalib_nohelper)
  {
    outsb(port, string, length);
  }
  else
  {
    io_string_t iostr;

    iostr.port = port;
    iostr.string = string;
    iostr.length = length;
    
    ioctl(__svgalib_mem_fd,SVGAHELPER_REPOUTB,&iostr); 
  }
}

void __svgalib_port_out(int value, int port)
{
  if(__svgalib_nohelper)
  {
    outb(value, port);
  }
  else
  {
    io_t iov;
    
    iov.val=value;
    iov.port=port;
    ioctl(__svgalib_mem_fd,SVGAHELPER_OUTB,&iov);
  }
}

void __svgalib_port_outw(int value, int port)
{
  if(__svgalib_nohelper)
  {
    outw(value, port);
  }
  else
  {
    io_t iov;
    
    iov.val=value;
    iov.port=port;
    ioctl(__svgalib_mem_fd,SVGAHELPER_OUTW,&iov);
  }
}

void __svgalib_port_outl(int value, int port)
{
  if(__svgalib_nohelper)
  {
    outl(value, port);
  }
  else
  {
    io_t iov;
    
    iov.val=value;
    iov.port=port;
    ioctl(__svgalib_mem_fd,SVGAHELPER_OUTL,&iov);
  }
}

int __svgalib_port_in(int port)
{
  if(__svgalib_nohelper)
  {
    return inb(port);
  }
  else
  {
    io_t iov;
    
    iov.port=port;
    ioctl(__svgalib_mem_fd,SVGAHELPER_INB,&iov);

    return iov.val;
  }
}

int __svgalib_port_inw(int port)
{
  if(__svgalib_nohelper)
  {
    return inw(port);
  }
  else
  {
    io_t iov;
    
    iov.port=port;
    ioctl(__svgalib_mem_fd,SVGAHELPER_INW,&iov);

    return iov.val;
  }
}

int __svgalib_port_inl(int port)
{
  if(__svgalib_nohelper)
  {
    return inl(port);
  }
  else
  {
    io_t iov;
    
    iov.port=port;
    ioctl(__svgalib_mem_fd,SVGAHELPER_INL,&iov);

    return iov.val;
  }
}
