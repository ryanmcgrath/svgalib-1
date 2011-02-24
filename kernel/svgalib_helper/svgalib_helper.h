#ifndef SVGALIB_HELPER_H
#define SVGALIB_HELPER_H

#ifdef __KERNEL__

#define MAX_NR_DEVICES 15

#define address_t unsigned long

struct sh_pci_device {
   unsigned short vendor;
   unsigned short id;
   unsigned char revision;
   struct pci_dev *dev;
   address_t mem[6];
   address_t len[6];
   address_t mask[6];
   int flags[6];
   unsigned long iobase;
   u8 * membase;
   int (*test_vsync)(struct sh_pci_device *);
   void (*ack_vsync)(struct sh_pci_device *);
   void (*enable_vsync)(struct sh_pci_device *);
   void (*disable_vsync)(struct sh_pci_device *);
   int opencount;
   int vsync;
   wait_queue_head_t vsync_wait;
#ifndef NO_TASK
   wait_queue_head_t startad_wait;
   long startad;
   struct tq_struct tq_startad;
#endif
};

extern int debug;

#endif

typedef struct {
  int port;
  int length;
  unsigned char* string;
} io_string_t;

typedef struct {
   int port;
   unsigned int val;
} io_t;

typedef struct {
   int pcipos;
   unsigned int address;
   unsigned long val;
} pcic_t;

typedef struct {
   void *win;
   void *lfb;
} windowing_t;

typedef struct dhahelper_vmi_s
{
    void *	virtaddr;
    unsigned long length;
    unsigned long *realaddr;
}dhahelper_vmi_t;

typedef struct dhahelper_mem_s
{
    void *	addr;
    unsigned long length;
}dhahelper_mem_t;

#define SVGAHELPER_MAGIC 0xB3

#define SVGAHELPER_OUTB		_IOR(SVGAHELPER_MAGIC,1,io_t)
#define SVGAHELPER_OUTW		_IOR(SVGAHELPER_MAGIC,2,io_t)
#define SVGAHELPER_OUTL		_IOR(SVGAHELPER_MAGIC,3,io_t)
#define SVGAHELPER_INB		_IOW(SVGAHELPER_MAGIC,4,io_t)
#define SVGAHELPER_INW		_IOW(SVGAHELPER_MAGIC,5,io_t)
#define SVGAHELPER_INL		_IOW(SVGAHELPER_MAGIC,6,io_t)

#define SVGAHELPER_PCIOUTB	_IOR(SVGAHELPER_MAGIC,11,pcic_t)
#define SVGAHELPER_PCIOUTW	_IOR(SVGAHELPER_MAGIC,12,pcic_t)
#define SVGAHELPER_PCIOUTL	_IOR(SVGAHELPER_MAGIC,13,pcic_t)
#define SVGAHELPER_PCIINB	_IOW(SVGAHELPER_MAGIC,14,pcic_t)
#define SVGAHELPER_PCIINW	_IOW(SVGAHELPER_MAGIC,15,pcic_t)
#define SVGAHELPER_PCIINL	_IOW(SVGAHELPER_MAGIC,16,pcic_t)
#define SVGAHELPER_PCIAPLEN	_IOW(SVGAHELPER_MAGIC,17,pcic_t)

#define SVGAHELPER_DVMA		_IO(SVGAHELPER_MAGIC,7)
#define SVGAHELPER_WIND		_IOR(SVGAHELPER_MAGIC,8,windowing_t)

#define SVGAHELPER_IOPERM	_IO(SVGAHELPER_MAGIC,9)
#define SVGAHELPER_REPOUTB	_IOR(SVGAHELPER_MAGIC,10,io_t)

#define SVGAHELPER_I810GTT	_IOW(SVGAHELPER_MAGIC,128,unsigned int *)
#define SVGAHELPER_I810GTTE	_IOW(SVGAHELPER_MAGIC,129,unsigned int *)

#define SVGAHELPER_WRITEB	_IOR(SVGAHELPER_MAGIC,21,io_t)
#define SVGAHELPER_WRITEW	_IOR(SVGAHELPER_MAGIC,22,io_t)
#define SVGAHELPER_WRITEL	_IOR(SVGAHELPER_MAGIC,23,io_t)
#define SVGAHELPER_READB	_IOW(SVGAHELPER_MAGIC,24,io_t)
#define SVGAHELPER_READW	_IOW(SVGAHELPER_MAGIC,25,io_t)
#define SVGAHELPER_READL	_IOW(SVGAHELPER_MAGIC,26,io_t)

#define SVGAHELPER_WAITRETRACE	_IO(SVGAHELPER_MAGIC,31)
#define SVGAHELPER_SETDISPLAYSTART _IOR(SVGAHELPER_MAGIC,32, int)

#define SVGAHELPER_VIRT_TO_PHYS  _IOWR(SVGAHELPER_MAGIC, 40, dhahelper_vmi_t)
#define SVGAHELPER_VIRT_TO_BUS   _IOWR(SVGAHELPER_MAGIC, 41, dhahelper_vmi_t)
#define SVGAHELPER_ALLOC_PA      _IOWR(SVGAHELPER_MAGIC, 42, dhahelper_mem_t)
#define SVGAHELPER_FREE_PA       _IOWR(SVGAHELPER_MAGIC, 43, dhahelper_mem_t)
#define SVGAHELPER_LOCK_MEM      _IOWR(SVGAHELPER_MAGIC, 44, dhahelper_mem_t)
#define SVGAHELPER_UNLOCK_MEM    _IOWR(SVGAHELPER_MAGIC, 45, dhahelper_mem_t)

#endif /* SVGALIB_HELPER_H */
