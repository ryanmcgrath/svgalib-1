#include <linux/pci.h>
#include <linux/mm.h>
#include <asm/io.h>
#include "kernel26compat.h"
#include "svgalib_helper.h"

#ifndef NO_TASK
void set_displaystart(struct sh_pci_device *dev) {
    int i, id;
	long ad;
	
    switch(dev->vendor) {
        case PCI_VENDOR_ID_MATROX:
			ad=dev->startad>>3;
			writeb(0x0c, dev->membase+0x3D4);
			writeb((ad & 0xFF00)>>8, dev->membase+0x3D5);
			writeb(0x0d, dev->membase+0x3D4);
			writeb(ad & 0xFF, dev->membase+0x3D5);
			writeb(0x00, dev->membase+0x3DE);
			i=readb(dev->membase+0x3DF) & 0xb0;
			i|=(ad & 0xf0000)>>16;
			i|=(ad & 0x100000)>>14;
			writeb(i, dev->membase+0x3DF);
            break;
#if 0
        case PCI_VENDOR_ID_SI: /* SiS */
            dev->iobase = dev->mem[2]-0x380;
            dev->test_vsync = io_test_vsync;
            dev->ack_vsync = io_ack_vsync;
            dev->enable_vsync = io_enable_vsync;
            break;
#endif
        case PCI_VENDOR_ID_NVIDIA_SGS:
            if(dev->id<0x20) {
				ad=dev->startad>>2;
				writeb(0x0c, dev->membase+0x6013D4);
				writeb((ad & 0xFF00)>>8, dev->membase+0x6013D5);
				writeb(0x0d, dev->membase+0x6013D4);
				writeb(ad & 0xFF, dev->membase+0x6013D5);
				
				writeb(0x19, dev->membase+0x6013D4);
				i=readb(dev->membase+0x6013D5) & 0xe0;
				i|=(ad & 0x1f0000)>>16;
				writeb(i, dev->membase+0x6013D5);
				
				writeb(0x2D, dev->membase+0x6013D4);
				i=readb(dev->membase+0x6013D5) & 0x9f;
				i|=(ad & 0x600000)>>16;
				writeb(i, dev->membase+0x6013D5);
            } else {
            }
            break;
#if 0
        case PCI_VENDOR_ID_NVIDIA:
            dev->iobase = (unsigned long)ioremap(dev->mem[0],0x800000);
            dev->test_vsync = nv4_test_vsync;
            dev->ack_vsync = nv4_ack_vsync;
            dev->enable_vsync = nv4_enable_vsync;
            break;
        case PCI_VENDOR_ID_ATI:
            id=dev->id;
    
            if( (id==0x4c45) ||
                (id==0x4c46) ||
                (id==0x4c57) ||
                (id==0x4c59) ||
                (id==0x4c5a) ||
                (id==0x4d46) ||
                (id==0x4d4c) ||
				(id==0x4242) ||
                ((id>>8)==0x50) ||
                ((id>>8)==0x51) || 
                ((id>>8)==0x52) ||
                ((id>>8)==0x53) ||
                ((id>>8)==0x54)) {                
                    dev->iobase = (unsigned long)ioremap(dev->mem[2], 16384);
                    dev->test_vsync = r128_test_vsync;
                    dev->ack_vsync = r128_ack_vsync;
                    dev->enable_vsync = r128_enable_vsync;
            } else {
                    dev->iobase = dev->mem[1];
                    dev->test_vsync = rage_test_vsync;
                    dev->ack_vsync = rage_ack_vsync;
                    dev->enable_vsync = rage_enable_vsync;
            }
            break;
        case PCI_VENDOR_ID_RENDITION:
            dev->iobase = dev->mem[1];
            dev->test_vsync = rendition_test_vsync;
            dev->ack_vsync = rendition_ack_vsync;
            dev->enable_vsync = rendition_enable_vsync;
            break;
		case PCI_VENDOR_ID_S3:
			dev->iobase = (unsigned long)ioremap(dev->mem[0]+0x1000000, 0x10000);
            dev->test_vsync = s3_test_vsync;
            dev->ack_vsync = s3_ack_vsync;
            dev->enable_vsync = s3_enable_vsync;
			break;
        default:
            dev->test_vsync = vga_test_vsync;
            dev->ack_vsync = vga_ack_vsync;
            dev->enable_vsync = vga_enable_vsync;
            dev->iobase = 0;
#endif
    }
    
}
#endif
