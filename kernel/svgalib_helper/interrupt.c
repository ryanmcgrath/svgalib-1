#include <linux/pci.h>
#include <linux/mm.h>
#include <asm/io.h>
#include "kernel26compat.h"
#include "svgalib_helper.h"

int vga_test_vsync(struct sh_pci_device *dev) {
     return inb(0x3c2)&0x80;
}

void vga_ack_vsync(struct sh_pci_device *dev) {
    int pb;
    
/* clear pending */
    outb(0x11, 0x3d4);
    pb = inb(0x3d5);
    outb(0x11, 0x3d4);
    outb((pb&0xef) | 0x20, 0x3d5);

/* disable interrupts */
    outb(0x11, 0x3d4);
    pb = inb(0x3d5);
    outb(0x11, 0x3d4);
    outb(pb | 0x20 , 0x3d5);
}

void vga_enable_vsync(struct sh_pci_device *dev) {
    int pb;

/* enable interrupt, clear pending */
    outb(0x11, 0x3d4);
    pb = inb(0x3d5);
    outb(0x11, 0x3d4);
    outb((pb&0xcf) , 0x3d5);

/* Allow interrupts */
    outb(0x11, 0x3d4);
    pb = inb(0x3d5);
    outb(0x11, 0x3d4);
    outb(pb | 0x10 , 0x3d5);
}

int io_test_vsync(struct sh_pci_device *dev) {
     return inb(dev->iobase+0x3c2)&0x80;
}

void io_ack_vsync(struct sh_pci_device *dev) {
    int pb;
    
/* clear pending */
    outb(0x11, dev->iobase+0x3d4);
    pb = inb(dev->iobase+0x3d5);
    outb(0x11, dev->iobase+0x3d4);
    outb((pb&0xef) | 0x20, dev->iobase+0x3d5);

/* disable interrupts */
    outb(0x11, dev->iobase+0x3d4);
    pb = inb(dev->iobase+0x3d5);
    outb(0x11, dev->iobase+0x3d4);
    outb(pb | 0x20 , dev->iobase+0x3d5);
}

void io_enable_vsync(struct sh_pci_device *dev) {
    int pb;

/* enable interrupt, clear pending */
    outb(0x11, dev->iobase+0x3d4);
    pb = inb(dev->iobase+0x3d5);
    outb(0x11, dev->iobase+0x3d4);
    outb((pb&0xcf) , dev->iobase+0x3d5);

/* Allow interrupts */
    outb(0x11, dev->iobase+0x3d4);
    pb = inb(dev->iobase+0x3d5);
    outb(0x11, dev->iobase+0x3d4);
    outb(pb | 0x10 , dev->iobase+0x3d5);
}

int mm_test_vsync(struct sh_pci_device *dev) {
     return readb(dev->membase+0x3c2)&0x80;
}

void mm_ack_vsync(struct sh_pci_device *dev) {
    int pb;
    
/* clear pending */
    writeb(0x11, dev->membase+0x3d4);
    pb = readb(dev->membase+0x3d5);
    writeb(0x11, dev->membase+0x3d4);
    writeb((pb&0xef) | 0x20, dev->membase+0x3d5);

/* disable interrupts */
    writeb(0x11, dev->membase+0x3d4);
    pb = readb(dev->membase+0x3d5);
    writeb(0x11, dev->membase+0x3d4);
    writeb(pb | 0x20 , dev->membase+0x3d5);
}

void mm_enable_vsync(struct sh_pci_device *dev) {
    int pb;

/* enable interrupt, clear pending */
    writeb(0x11, dev->membase+0x3d4);
    pb = readb(dev->membase+0x3d5);
    writeb(0x11, dev->membase+0x3d4);
    writeb((pb&0xcf) , dev->membase+0x3d5);

/* Allow interrupts */
    writeb(0x11, dev->membase+0x3d4);
    pb = readb(dev->membase+0x3d5);
    writeb(0x11, dev->membase+0x3d4);
    writeb(pb | 0x10 , dev->membase+0x3d5);
}

int s3_test_vsync(struct sh_pci_device *dev) {
	return readl(dev->membase+0x8504) & 1;
}

void s3_enable_vsync(struct sh_pci_device *dev) {
	writel(0x101, dev->membase+0x8504);
}

void s3_ack_vsync(struct sh_pci_device *dev) {
	writel(0x001, dev->membase+0x8504);
}

static uint32_t saved_pmc=0xffffffff;

int nv3_test_vsync(struct sh_pci_device *dev) {
     return readl(dev->membase+0x400100)&0x100;
}

void nv3_ack_vsync(struct sh_pci_device *dev) {

/* disable interrupt, clear pending */
    writel(0xffffffff,  dev->membase + 0x000100);
    writel(0x100, 	dev->membase + 0x400100);
    writel(0, 		dev->membase + 0x000140);
    writel(0, 		dev->membase + 0x400140);
	if(saved_pmc!=0xffffffff)
		writel(saved_pmc,	dev->membase + 0x000200); 

}

void nv3_enable_vsync(struct sh_pci_device *dev) {
    saved_pmc = inl(dev->iobase + 0x200);
    writel(saved_pmc|0x1000, dev->membase+0x200);
    writel(0x1, 	dev->membase + 0x000140);
    writel(0x100, 	dev->membase + 0x400140);
    writel(0xffffffff, 	dev->membase + 0x000100);
    writel(0xffffffff, 	dev->membase + 0x400100);
}

int nv4_test_vsync(struct sh_pci_device *dev) {
     return readl(dev->membase+0x600100)&0x1;
}

void nv4_ack_vsync(struct sh_pci_device *dev) {

/* disable interrupt, clear pending */
    writel(0xffffffff,  dev->membase + 0x000100);
    writel(0x1, 	dev->membase + 0x600100);
    writel(0, 		dev->membase + 0x000140);
    writel(0, 		dev->membase + 0x600140);
    writel(saved_pmc,	dev->membase + 0x000200); 
}

void nv4_enable_vsync(struct sh_pci_device *dev) {
    saved_pmc = inl(dev->iobase + 0x200);
    writel(saved_pmc|(1<<24),dev->membase+0x200);
    writel(0x1, 	dev->membase + 0x000140);
    writel(0x1, 	dev->membase + 0x600140);
    writel(0xffffffff, 	dev->membase + 0x000100);
    writel(0xffffffff, 	dev->membase + 0x600100);
}

int r128_test_vsync(struct sh_pci_device *dev) {    
    return readl(dev->membase + 0x44) &1;
}

void r128_ack_vsync(struct sh_pci_device *dev) {
    writel(1, dev->membase + 0x44);
    writel(readl(dev->membase + 0x40) & 0xfffffffe, dev->membase + 0x40); 
}

void r128_enable_vsync(struct sh_pci_device *dev) {
    writel(1, dev->membase + 0x44);
    writel(readl(dev->membase + 0x40) | 1, dev->membase + 0x40); 
}

int rage_test_vsync(struct sh_pci_device *dev) {    
    return inl(dev->iobase + 0x18) &4;
}

void rage_ack_vsync(struct sh_pci_device *dev) {
    outl((inl(dev->iobase + 0x18) & 0xfffffff8) | 12, dev->iobase + 0x18); 
}

void rage_enable_vsync(struct sh_pci_device *dev) {
    outl((inl(dev->iobase + 0x18) & 0xfffffff8) | 14, dev->iobase + 0x18); 
}

int rendition_test_vsync(struct sh_pci_device *dev) {    
    return inw(dev->iobase + 0x44) & 1;
}

void rendition_ack_vsync(struct sh_pci_device *dev) {
    outw(1, dev->iobase + 0x44);
    outw(0, dev->iobase + 0x46); 
}

void rendition_enable_vsync(struct sh_pci_device *dev) {
    outw(1, dev->iobase + 0x44);
    outw(1, dev->iobase + 0x46); 
}

void vga_init_vsync(struct sh_pci_device *dev) {
    int i, id;
    switch(dev->vendor) {
        case PCI_VENDOR_ID_MATROX:
            i=0;
            if(dev->len[0]>=1048576)i=1;
            dev->membase = ioremap(dev->mem[i],0x2000) + 0x1c00;
            dev->test_vsync = mm_test_vsync;
            dev->ack_vsync = mm_ack_vsync;
            dev->enable_vsync = mm_enable_vsync;            
            dev->disable_vsync = mm_ack_vsync;
            break;
        case PCI_VENDOR_ID_SI: /* SiS */
            dev->iobase = dev->mem[2]-0x380;
            dev->test_vsync = io_test_vsync;
            dev->ack_vsync = io_ack_vsync;
            dev->enable_vsync = io_enable_vsync;
            dev->disable_vsync = io_ack_vsync;
            break;
        case PCI_VENDOR_ID_NVIDIA_SGS:
            dev->membase = ioremap(dev->mem[0],0x800000);
            if(dev->id<0x20) {
                dev->test_vsync = nv3_test_vsync;
                dev->ack_vsync = nv3_ack_vsync;
                dev->enable_vsync = nv3_enable_vsync;
                dev->disable_vsync = nv3_ack_vsync;
            } else {
                dev->test_vsync = nv4_test_vsync;
                dev->ack_vsync = nv4_ack_vsync;
                dev->enable_vsync = nv4_enable_vsync;
                dev->disable_vsync = nv4_ack_vsync;
            }
            break;
        case PCI_VENDOR_ID_NVIDIA:
            dev->membase = ioremap(dev->mem[0],0x800000);
            dev->test_vsync = nv4_test_vsync;
            dev->ack_vsync = nv4_ack_vsync;
            dev->enable_vsync = nv4_enable_vsync;
			dev->disable_vsync = nv4_ack_vsync;
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
                    dev->membase = ioremap(dev->mem[2], 16384);
                    dev->test_vsync = r128_test_vsync;
                    dev->ack_vsync = r128_ack_vsync;
                    dev->enable_vsync = r128_enable_vsync;
                    dev->disable_vsync = r128_ack_vsync;
            } else {
                    dev->iobase = dev->mem[1];
                    dev->test_vsync = rage_test_vsync;
                    dev->ack_vsync = rage_ack_vsync;
                    dev->enable_vsync = rage_enable_vsync;
                    dev->disable_vsync = rage_ack_vsync;
            }
            break;
        case PCI_VENDOR_ID_RENDITION:
            dev->iobase = dev->mem[1];
            dev->test_vsync = rendition_test_vsync;
            dev->ack_vsync = rendition_ack_vsync;
            dev->enable_vsync = rendition_enable_vsync;
            dev->disable_vsync = rendition_ack_vsync;
            break;
		case PCI_VENDOR_ID_S3:
			dev->membase = ioremap(dev->mem[0]+0x1000000, 0x10000);
            dev->test_vsync = s3_test_vsync;
            dev->ack_vsync = s3_ack_vsync;
            dev->enable_vsync = s3_enable_vsync;
            dev->disable_vsync = s3_ack_vsync;
			break;
        default:
            dev->test_vsync = vga_test_vsync;
            dev->ack_vsync = vga_ack_vsync;
            dev->enable_vsync = vga_enable_vsync;
            dev->disable_vsync = vga_ack_vsync;
            dev->iobase = 0;
    }
    
}

