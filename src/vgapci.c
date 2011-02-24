#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "endianess.h"
#include "libvga.h"
#include "svgalib_helper.h"
#include "vgapci.h"

int __svgalib_pci_helper_idev=0;
int __svgalib_pci_nohelper_idev=0;
int __svgalib_pci_card_found_at=0;

static void proc_pci_read_config(int device, unsigned int *buf, int size)
{
   int i;

   for(i=0;i<size;i++) {
       buf[i]=__svgalib_pci_read_config_dword(device, i*4);
   }
}

/* 
   find a vga device of the specified vendor, and return
   its configuration (64 dwords) in conf 
   return device pci pos of found.
*/ 
int __svgalib_pci_find_vendor_vga_pos(unsigned int vendor, unsigned int *conf)
{
  	int device;
  	int s, f, step;
	
	if(__svgalib_nohelper) { 
		step = 8; /* only one function per device */
		if(__svgalib_pci_nohelper_idev) {
			s=__svgalib_pci_nohelper_idev;
			f=__svgalib_pci_nohelper_idev+1;
		} else {
			s=0;
			f=1024;
		}
  	}
  	else
  	{
		step=1;
		if(__svgalib_pci_helper_idev) {
			s=__svgalib_pci_helper_idev;
			f=__svgalib_pci_helper_idev+1;
		} else {
			s=1;
			f=17;
		}
  	}

	for(device=s; device<f; device+=step) {
		proc_pci_read_config(device,conf,3);
		if(((conf[0]&0xffff)==vendor)&&
				(((conf[2]>>16)&0xffff)==0x0300)) { /* VGA Class */
			proc_pci_read_config(device,conf,16);
                __svgalib_pci_card_found_at = device;
			return device;
		}
	}

	return 0;
}

unsigned int __svgalib_pci_read_config_dword(int pos, int address)
{
    if(__svgalib_nohelper) {
		int f;
		unsigned int n, d;
		int bus, device, fn;
		char filename[256];
		
		bus=(pos&0xff00)>>8;
		device=(pos&0xf8)>>3;
		fn=pos&0x07;
		sprintf(filename,"/proc/bus/pci/%02i/%02x.%i",bus,device,fn);
		f=open(filename,O_RDONLY);
		lseek(f, address, SEEK_SET);
		read(f, &n, 4);
		close(f);
		d=LE32(n);
		return d;
	} else {
		pcic_t p;
 		
		p.pcipos = pos;
		p.address = address;
		
		if(ioctl( __svgalib_mem_fd, SVGAHELPER_PCIINL, &p)) return -1;
		
		return p.val;
	}
}

unsigned long __svgalib_pci_read_aperture_len(int pos, int address)
{
    if(__svgalib_nohelper) {
		FILE *f;
		char buf[512];

		f=fopen("/proc/bus/pci/devices", "r");
		while (fgets(buf, sizeof(buf)-1, f)) {
			int cnt;
			unsigned int dev, di;
			unsigned long lens[6], dl;
			cnt = sscanf(buf, "%x %x %x %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx %lx",
					&dev, &di, &di, 
					&dl, &dl, &dl, &dl, &dl, &dl, &dl,
					&lens[0], &lens[1], &lens[2], &lens[3], &lens[4], &lens[5],
					&dl);
			if(dev==pos)
				return lens[address];				
		}
	} else {
		pcic_t p;
		   
        p.pcipos = pos;
        p.address = address;
        
        if(ioctl( __svgalib_mem_fd, SVGAHELPER_PCIAPLEN, &p)) return -1;
        
        return p.val;
    }

	return -1;
}

void __svgalib_pci_write_config_dword(int pos, int address, unsigned int data)
{
    if(__svgalib_nohelper)
    {
	int f;
	unsigned int d;
	int bus, device, fn;
	char filename[256];
	
	d=LE32(data);
	bus=(pos&0xff00)>>8;
	device=(pos&0xf8)>>3;
	fn=pos&0x07;
	sprintf(filename,"/proc/bus/pci/%02i/%02x.%i",bus,device,fn);
	f=open(filename,O_WRONLY);
	lseek(f, address, SEEK_SET);
	write(f, &d, 4);
	close(f);
    }
    else
    {
        pcic_t p;
       
        p.pcipos = pos;
        p.address = address;
        p.val = data;
        
        ioctl( __svgalib_mem_fd, SVGAHELPER_PCIOUTL, &p);
    }
}

int memorytest(uint32_t *m, int max_mem) {
    unsigned char sav[1024];
    int i, j;

    max_mem*=4;
    for(i=0;i<max_mem;i++) {
        sav[i]=*(m+64*1024*i);
    }
    for(i=max_mem-1;i>=0;i--) {
        *(m+64*1024*i)=i;
    }
    for(i=0;i<max_mem;i++) {
//        printf("%i %i\n",i*256, *(m+64*1024*i));
        if(*(m+64*1024*i)!=i) break;
    }
    for(j=0;j<i;j++) {
        *(m+64*1024*j)=sav[j];
    }
    return i*256;
}
