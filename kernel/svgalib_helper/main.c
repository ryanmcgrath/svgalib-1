#include <linux/config.h>

#if defined (CONFIG_MODVERSIONS) && !defined (MODVERSIONS)
# define MODVERSIONS
#endif

#include <linux/kernel.h> /* printk() */
#include <linux/module.h>

#include <linux/slab.h> 
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/stddef.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/mm.h>
#include <linux/thread_info.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>

#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/syscalls.h>

#define __KERNEL_SYSCALLS__
#include <linux/unistd.h>
asmlinkage long (*s_ioperm)(unsigned long from, unsigned long num, int turn_on);

#include <asm/processor.h>
#include <asm/uaccess.h>
#include <asm/system.h>   /* cli(), *_flags */
#include <asm/segment.h>  /* memcpy and such */
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/irq.h>

#include "kernel26compat.h"

#include "svgalib_helper.h"

#include "vgaversion.h"

#include "i810.h"
#include "interrupt.h"
#include "displaystart.h"

int debug=0;
static int all_devices=0;
int num_devices=0;

static char *sdev_id="svgalib_helper";

static struct sh_pci_device *sh_pci_devs[MAX_NR_DEVICES];

static int irqs[MAX_NR_DEVICES];

#if (defined CONFIG_DEVFS_FS) && (!defined KERNEL_2_6)
static devfs_handle_t devfs_handle;
#endif

#if defined(KERNEL_2_6) && !defined(CONFIG_DEVFS_FS)
struct K_CLASS *svgalib_helper_class;
#endif

static int check_io_range(int port, int device) {
    return 1;
}

static struct pci_dev *get_pci_dev(int pcipos, int minor) {
    
    if(minor>=num_devices) return NULL;
    if(minor>0) {
        return sh_pci_devs[minor]->dev;
    } else {
        if(pcipos>0 && pcipos<num_devices)
            return sh_pci_devs[pcipos]->dev;
    }
    return NULL;

}

static int get_dev(int pcipos, int minor) {
    
    if(minor>=num_devices) return 0;
    if(minor>0) {
        return minor;
    } else {
		if(pcipos>=num_devices || pcipos<1) return 0;
		return pcipos;
    }
}

static volatile int vsync=0;
static wait_queue_head_t vsync_wait;

static irqreturn_t vsync_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    struct sh_pci_device *dev = (struct sh_pci_device *)dev_id;

    if((char *)dev==sdev_id) {
        if(!vga_test_vsync(dev)) {
			return IRQ_NONE;
		}
        vga_ack_vsync(dev);
    	vsync=0;
    	wake_up_interruptible(&vsync_wait);
    } else {
        if(!dev->test_vsync(dev)) {
			return IRQ_NONE;
		}
        dev->ack_vsync(dev);
    	dev->vsync=0;
    	wake_up_interruptible(&dev->vsync_wait);

#ifndef NO_TASK
		wake_up_interruptible(&dev->startad_wait);
#endif
    }

	return IRQ_HANDLED;
}

#ifndef NO_TASK
static void task_startad(void *data) {
	struct sh_pci_device *dev = (struct sh_pci_device *)data;
	int i;
	
	i=dev->dev->irq;
	dev->enable_vsync(dev);
	interruptible_sleep_on(&dev->vsync_wait);
	dev->ack_vsync(dev);
	set_displaystart(dev);
	dev->startad=-1;
}
#endif

#define GET_IOSTR \
	get_user(iostr.port, &user_iostr->port); \
	get_user(iostr.length, &user_iostr->length); \
	get_user(iostr.string, &user_iostr->string); 
#define GET_IOV \
	get_user(iov.port, &user_iov->port); \
	get_user(iov.val, &user_iov->val); 
#define PUT_IOV \
	put_user(iov.val, &user_iov->val); 
#define GET_PCIV \
	get_user(pciv.pcipos, &user_pciv->pcipos); \
	get_user(pciv.address, &user_pciv->address); \
	get_user(pciv.val, &user_pciv->val); 
#define PUT_PCIV \
	put_user(pciv.val, &user_pciv->val); 
static int svgalib_helper_ioctl( struct inode *inode, struct file *filp, 
                          unsigned int cmd, unsigned long arg) {

    io_t iov, *user_iov=(io_t *)arg;
    pcic_t pciv, *user_pciv=(pcic_t *)arg;
    int minor = my_minor(inode->i_rdev);
    struct pci_dev *pdev;
    io_string_t iostr, *user_iostr=(io_string_t *)arg;
    int i=0, ret;
    u8 pb;
    u16 pw;
    u32 pl;
    unsigned char *outb_str;

    if(_IOC_TYPE(cmd)!=SVGAHELPER_MAGIC) {
        return -EINVAL;
    }
    
    ret=0;

    switch(_IOC_NR(cmd)) {

        case _IOC_NR(SVGAHELPER_REPOUTB):
			GET_IOSTR;
            if (iostr.length>4096) return -EINVAL;
	    	if ( (outb_str = kmalloc(iostr.length, GFP_KERNEL )) == NULL ) return -ENOMEM;
	    	if(copy_from_user(outb_str,iostr.string,iostr.length)) return -EPERM;
	    	if(check_io_range(iostr.port,minor)) {
	        	for(i=0; i<iostr.length; i++) outb(outb_str[i], iostr.port);
			} else ret = -EPERM;
            kfree (outb_str);
            break;

        case _IOC_NR(SVGAHELPER_OUTB):
			GET_IOV;
 			if(check_io_range(iov.port,minor))
 				outb(iov.val,iov.port);
				else ret = -EPERM;
			break;

        case _IOC_NR(SVGAHELPER_OUTW):
			GET_IOV;
           if(check_io_range(iov.port,minor))
               outw(iov.val,iov.port);
               else ret = -EPERM;
        break;

        case _IOC_NR(SVGAHELPER_OUTL):
			GET_IOV;
           if(check_io_range(iov.port,minor))
               outl(iov.val,iov.port);
               else ret = -EPERM;
        break;

        case _IOC_NR(SVGAHELPER_INB):
			GET_IOV;
           if(check_io_range(iov.port,minor))
               iov.val=inb(iov.port);
               else ret = -EPERM;
		   PUT_IOV;
        break;

        case _IOC_NR(SVGAHELPER_INW):
			GET_IOV;
           if(check_io_range(iov.port,minor))
               iov.val=inw(iov.port);
               else ret = -EPERM;
		   PUT_IOV;
        break;

        case _IOC_NR(SVGAHELPER_INL):
			GET_IOV;
           if(check_io_range(iov.port,minor))
               iov.val=inl(iov.port);
               else ret = -EPERM;
		   PUT_IOV;
        break;

        case _IOC_NR(SVGAHELPER_WRITEB):
			GET_IOV;
           writeb(iov.val,(u8 *)iov.port);
        break;

        case _IOC_NR(SVGAHELPER_WRITEW):
			GET_IOV;
           writew(iov.val,(u16 *)iov.port);
        break;

        case _IOC_NR(SVGAHELPER_WRITEL):
			GET_IOV;
           writel(iov.val,(u32 *)iov.port);
        break;

        case _IOC_NR(SVGAHELPER_READB):
			GET_IOV;
           iov.val=readb((u8 *)iov.port);
		   PUT_IOV;
        break;

        case _IOC_NR(SVGAHELPER_READW):
			GET_IOV;
           iov.val=readw((u16 *)iov.port);
		   PUT_IOV;
        break;

        case _IOC_NR(SVGAHELPER_READL):
			GET_IOV;
           iov.val=readl((u32 *)iov.port);
		   PUT_IOV;
        break;

        case _IOC_NR(SVGAHELPER_PCIINB):
			GET_PCIV;
            pdev = get_pci_dev(pciv.pcipos, minor);
            if(!pdev) return -EINVAL;
            pci_read_config_byte(pdev, pciv.address, &pb);
            pciv.val=pb;
			PUT_PCIV;
            break;

        case _IOC_NR(SVGAHELPER_PCIINW):
			GET_PCIV;
            pdev = get_pci_dev(pciv.pcipos, minor);
            if(!pdev) return -EINVAL;
            pci_read_config_word(pdev, pciv.address, &pw);
            pciv.val=pw;
			PUT_PCIV;
            break;

        case _IOC_NR(SVGAHELPER_PCIINL):
			GET_PCIV;
            pdev = get_pci_dev(pciv.pcipos, minor);
            if(!pdev) return -EINVAL;
            pci_read_config_dword(pdev, pciv.address, &pl);
            pciv.val=pl;
			PUT_PCIV;
            break;

        case _IOC_NR(SVGAHELPER_PCIAPLEN):
			GET_PCIV;
            i = get_dev(pciv.pcipos, minor);
            if((i==0) | (pciv.address>5)) return -EINVAL;
            pciv.val=sh_pci_devs[i]->len[pciv.address];
			PUT_PCIV;
            break;

        case _IOC_NR(SVGAHELPER_PCIOUTB):
			GET_PCIV;
            pdev = get_pci_dev(pciv.pcipos, minor);
            if(!pdev) return -EINVAL;
            pb=pciv.val;
            pci_write_config_byte(pdev, pciv.address, pb);
            break;

        case _IOC_NR(SVGAHELPER_PCIOUTW):
			GET_PCIV;
            pdev = get_pci_dev(pciv.pcipos, minor);
            if(!pdev) return -EINVAL;
            pw=pciv.val;
            pci_write_config_word(pdev, pciv.address, pw);
            break;

        case _IOC_NR(SVGAHELPER_PCIOUTL):
			GET_PCIV;
            pdev = get_pci_dev(pciv.pcipos, minor);
            if(!pdev) return -EINVAL;
            pl=pciv.val;
            pci_write_config_dword(pdev, pciv.address, pl);
            break;
#ifdef __386__
        case _IOC_NR(SVGAHELPER_I810GTT):
            i=i810_make_gtt();
            copy_to_user((char *)arg, &i, sizeof(unsigned int));
            break;
        case _IOC_NR(SVGAHELPER_I810GTTE):
            copy_from_user(&i, (char *)arg, sizeof(unsigned int));
            if(i<I810_SIZE)
                copy_to_user((char *)arg, &i810_gttes[i], sizeof(unsigned long));
                else return -EINVAL;
            break;
#endif
        case _IOC_NR(SVGAHELPER_WAITRETRACE):

            /* Workaround for nvidia cards, which are not vga compatible */
//            if(!minor && num_devices==2) minor=1;

            if(minor) {
				int i;
                i=sh_pci_devs[minor]->dev->irq;
                if(i==0 || i==-1 || i==255) return -EINVAL;
				sh_pci_devs[minor]->vsync=1;
                sh_pci_devs[minor]->enable_vsync(sh_pci_devs[minor]);
	            wait_event_interruptible(sh_pci_devs[minor]->vsync_wait, 
						!sh_pci_devs[minor]->vsync);
                if(sh_pci_devs[minor]->vsync) sh_pci_devs[minor]->ack_vsync(sh_pci_devs[minor]);
            	if(sh_pci_devs[minor]->vsync) return -ERESTARTSYS;
            } else {
				int i;
                vsync=1;
                i=0;
                while(irqs[i]!=-1)
                    request_irq(irqs[i++], vsync_interrupt, SA_SHIRQ, "svgalib_helper", sdev_id);
                vga_enable_vsync((void *)sdev_id);
				wait_event_interruptible(vsync_wait, !vsync);
                i=0;
                if(vsync) vga_ack_vsync((void *)sdev_id);
                while(irqs[i]!=-1)
                    free_irq(irqs[i++], sdev_id);
            	if(vsync) return -ERESTARTSYS;
            }
            
            break;
#if 0
		case _IOC_NR(SVGAHELPER_IOPERM):
			if(minor) return -EPERM;
#if 1
			i=0;
			if(!capable(CAP_SYS_RAWIO)) {
				i=1;
				cap_raise(current->cap_effective, CAP_SYS_RAWIO);
			}
			s_ioperm=sys_call_table[__NR_ioperm];
			s_ioperm(0,0x400,1);
			if(i) {
				cap_lower(current->cap_effective, CAP_SYS_RAWIO);
			}
#else
			if(!ioperm) {
				struct thread_struct * t = &current->thread;
				struct tss_struct * tss;
				unsigned long *bitmap;
						
				ioperm=1;
				bitmap = kmalloc(IO_BITMAP_BYTES, GFP_KERNEL);
				if (!bitmap)
					return -ENOMEM;
				
				memset(bitmap, 0, IO_BITMAP_BYTES);
				t->io_bitmap_ptr = bitmap;
				tss = &per_cpu(init_tss, get_cpu());
				t->io_bitmap_max = IO_BITMAP_BYTES;
				tss->io_bitmap_base = INVALID_IO_BITMAP_OFFSET_LAZY;
				put_cpu();
			}
#endif
			
			break;
#endif
#ifndef NO_TASK
		case _IOC_NR(SVGAHELPER_SETDISPLAYSTART):
			if(minor) {
				if(sh_pci_devs[minor]->startad==-1)
					schedule_task(&sh_pci_devs[minor]->tq_startad);
				sh_pci_devs[minor]->startad=arg;
			}
			break;
#endif

#ifdef SVGA_HELPER_VIRTUAL
        case _IOC_NR(SVGAHELPER_VIRT_TO_PHYS):
			return dhahelper_virt_to_phys((dhahelper_vmi_t *)arg);
		case _IOC_NR(SVGAHELPER_VIRT_TO_BUS): 
			return dhahelper_virt_to_bus((dhahelper_vmi_t *)arg);
		case _IOC_NR(SVGAHELPER_ALLOC_PA):
			return dhahelper_alloc_pa((dhahelper_mem_t *)arg);
		case _IOC_NR(SVGAHELPER_FREE_PA): 
			return dhahelper_free_pa((dhahelper_mem_t *)arg);
		case _IOC_NR(SVGAHELPER_LOCK_MEM): 
			return dhahelper_lock_mem((dhahelper_mem_t *)arg);
		case _IOC_NR(SVGAHELPER_UNLOCK_MEM): 
			return dhahelper_unlock_mem((dhahelper_mem_t *)arg);
		default: 
#endif
            return -EINVAL;
    }
    return ret;
}   			


static int svgalib_helper_open( struct inode *inode, struct file * filp) {

	int minor = my_minor(inode->i_rdev);
   
	if(minor>=num_devices) return -ENODEV;

	if(minor) {
		int i=sh_pci_devs[minor]->dev->irq;
		sh_pci_devs[minor]->opencount++;
		if(sh_pci_devs[minor]->opencount==1 && i!=0 && i!=-1 && i!=255)
			request_irq(i, vsync_interrupt, SA_SHIRQ, "svgalib_helper", sh_pci_devs[minor]);
	}

#ifndef KERNEL_2_6
	MOD_INC_USE_COUNT;
#else
	try_module_get(THIS_MODULE);
#endif
   
	return 0;
}

static int svgalib_helper_release( struct inode *inode, struct file *filp) {

	int minor = my_minor(inode->i_rdev);

	if(minor) {
		int i=sh_pci_devs[minor]->dev->irq;
		sh_pci_devs[minor]->opencount--;
		if(sh_pci_devs[minor]->opencount==0 && i!=0 && i!=-1 && i!=255) {
			sh_pci_devs[minor]->disable_vsync(sh_pci_devs[minor]);
			free_irq(i, sh_pci_devs[minor]);
		}
	}
#ifndef KERNEL_2_6
	MOD_DEC_USE_COUNT;
#else
	module_put(THIS_MODULE);
#endif
	
	return 0;
}

int remap_cache(struct vm_area_struct *vma, unsigned long off) {

	vma->vm_pgoff = off >> PAGE_SHIFT;
	/* This is an IO map - tell maydump to skip this VMA */
	vma->vm_flags |= VM_IO;
#if defined(__sparc_v9__)
	vma->vm_flags |= (VM_SHM | VM_LOCKED);
	if (my_io_remap_page_range(vma, vma->vm_start, off,
				vma->vm_end - vma->vm_start, vma->vm_page_prot, 0))
		return -EAGAIN;
#else
#if defined(__mc68000__)
#if defined(CONFIG_SUN3)
	pgprot_val(vma->vm_page_prot) |= SUN3_PAGE_NOCACHE;
#else
	if (CPU_IS_020_OR_030)
		pgprot_val(vma->vm_page_prot) |= _PAGE_NOCACHE030;
	if (CPU_IS_040_OR_060) {
		pgprot_val(vma->vm_page_prot) &= _CACHEMASK040;
		/* Use no-cache mode, serialized */
		pgprot_val(vma->vm_page_prot) |= _PAGE_NOCACHE_S;
	}
#endif
#elif defined(__powerpc__)
	pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE|_PAGE_GUARDED;
#elif defined(__alpha__)
	/* Caching is off in the I/O space quadrant by design.  */
#elif defined(__i386__) || defined(__x86_64__)
	if (boot_cpu_data.x86 > 3)
		pgprot_val(vma->vm_page_prot) |= _PAGE_PCD;
#elif defined(__arm__) || defined(__mips__)
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#elif defined(__sh__)
	pgprot_val(vma->vm_page_prot) &= ~_PAGE_CACHABLE;
#elif defined(__hppa__)
	pgprot_val(vma->vm_page_prot) |= _PAGE_NO_CACHE; 
#elif defined(__ia64__)
	vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);
#else
#warning What do we have to do here??
#endif
	if (my_io_remap_page_range(vma, vma->vm_start, off,
			     vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
#endif /* !__sparc_v9__ */
	return 0;

}

int check_mem(int card, unsigned long start, unsigned long len) {
    int j;
    unsigned long rstart, rlen;
#ifdef __alpha__
    int type;
#endif
        
	if (start+len < start || len >= 0x40000000) return -3;
	
    rstart=start;
    rlen=len;

#ifdef __alpha__
    type = start>>32;
    switch(type) {
        case 2:
            start = start & 0xffffffff;
            start = start >> 5;
            len = len >> 5;
            break;
        case 3:
            start = start & 0xffffffff;
            break;
        default:
            return -1;
    }
#endif

    if(!card) {
        if( (start<0x110000) && (start+len<0x110000) ) return 0;
        for(j=1;j<num_devices;j++)
            if(!check_mem(j, rstart, rlen)) return 0;
    } else if(card<num_devices) {
        for(j=0;j<6;j++) if(sh_pci_devs[card]->mem[j])
            if((start>=sh_pci_devs[card]->mem[j])&&
               (start+len<=sh_pci_devs[card]->mem[j]+sh_pci_devs[card]->len[j])) {
               return 0;
           }
    }
    return -2;
}

static int svgalib_helper_mmap(struct file *filp, struct vm_area_struct *vma) {
   unsigned long start=vma->vm_start;
   unsigned long end=vma->vm_end;
   unsigned long minor = my_minor(filp->f_dentry->d_inode->i_rdev);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
   unsigned long ofs=vma->vm_pgoff*PAGE_SIZE;
#else
   unsigned long ofs=vma->vm_offset;
#endif

   if(check_mem(minor, ofs, end-start)) return -EPERM;
   if(remap_cache(vma, ofs)) return -EAGAIN;
   return 0;
}

struct file_operations svgalib_helper_fops = {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
   .owner	= THIS_MODULE,
#endif
   .ioctl	= svgalib_helper_ioctl,
   .mmap	= svgalib_helper_mmap,
   .open	= svgalib_helper_open,
   .release	= svgalib_helper_release,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
#define base_address(i) dev->resource[i].start
#else
#define base_address(i) (dev->base_address[i]&PCI_BASE_ADDRESS_MEM_MASK)
#endif

int init_module(void)
{
    int result, i, j;
    struct pci_dev *dev=NULL;
	char name[255];
#ifdef CONFIG_DEVFS_FS
# ifndef KERNEL_2_6
    devfs_handle_t slave_handle;
# endif
#endif
    /*
     * Register your major, and accept a dynamic number
     */
     
    printk(KERN_INFO "svgalib_helper: Initializing, version %s\n", versionstr);

	result = devfs_register_chrdev(SVGALIB_HELPER_MAJOR, "svgalib_helper", &svgalib_helper_fops);

	if (result < 0) {
        printk(KERN_WARNING "svgalib_helper: can't get major %d\n",SVGALIB_HELPER_MAJOR);
        return result;
    }

    if((sh_pci_devs[0]=kmalloc(sizeof(struct sh_pci_device),GFP_KERNEL))==NULL) {
        goto nomem_error;
    }
	
    memset(sh_pci_devs[0],0,sizeof(struct sh_pci_device));
    num_devices=1;
    for(i=1;i<MAX_NR_DEVICES;i++) sh_pci_devs[i]=NULL;

#ifdef CONFIG_DEVFS_FS
# ifndef KERNEL_2_6
    devfs_handle = devfs_mk_dir ( NULL, "svga_helper", NULL );
    devfs_register_series( devfs_handle,
               "%u", 8, DEVFS_FL_DEFAULT, SVGALIB_HELPER_MAJOR, 0,
                       S_IFCHR |  S_IRUGO | S_IRWXU, &svgalib_helper_fops, NULL ) ;
    DEVFS_MK_SYMLINK( NULL, "svga", 0, "svga_helper/0", &slave_handle, NULL );
    devfs_auto_unregister( devfs_handle, slave_handle );
# else
	devfs_mk_dir ("svga_helper");
	for (i = 0; i < 8; i++) {
		devfs_mk_cdev(MKDEV(SVGALIB_HELPER_MAJOR, i),
				S_IFCHR |  S_IRUGO | S_IRWXU, "svga_helper/%d", i);
	}
	DEVFS_MK_SYMLINK("svga", "svga_helper/0");
# endif
#endif /* devfsd support */

	SLH_SYSFS_REGISTER;
	SLH_SYSFS_ADD_CONTROL;

    if(pci_present()) {
        while((dev= all_devices ? 
					PCI_GET_DEVICE(PCI_ANY_ID, PCI_ANY_ID, dev) :
					PCI_GET_CLASS(PCI_CLASS_DISPLAY_VGA<<8,dev)) && 
              (num_devices<=MAX_NR_DEVICES)) {
            if((sh_pci_devs[num_devices]=kmalloc(sizeof(struct sh_pci_device),GFP_KERNEL))==NULL) {
                goto nomem_error;
            }
            memset(sh_pci_devs[num_devices],0,sizeof(struct sh_pci_device));
            sh_pci_devs[num_devices]->dev=dev;
            pci_read_config_word(dev,0,&sh_pci_devs[num_devices]->vendor);
            pci_read_config_word(dev,2,&sh_pci_devs[num_devices]->id);
            pci_read_config_byte(dev,8,&sh_pci_devs[num_devices]->revision);
            printk(KERN_DEBUG "svgalib_helper: device%d: vendor:%.4x id:%.4x\n",num_devices,
            sh_pci_devs[num_devices]->vendor,sh_pci_devs[num_devices]->id);
            for(i=0;i<6;i++){
                unsigned int t; 
                int len;
                pci_read_config_dword(dev,16+4*i,&result);
                if(result) {
                    pci_write_config_dword(dev,16+4*i,0xffffffff);
                    pci_read_config_dword(dev,16+4*i,&t);
                    pci_write_config_dword(dev,16+4*i,result);
                    len = ~(t&~0xf)+1;
                    if (len){
                       sh_pci_devs[num_devices]->mem[i]=result&~0xf;
                       sh_pci_devs[num_devices]->flags[i]=0x80 | (result&0xf);
                       sh_pci_devs[num_devices]->len[i]=len;
                       sh_pci_devs[num_devices]->mask[i]=t&~0xf;
                       printk(KERN_DEBUG "device%d: region%d, base=%.8x len=%d type=%d\n",
                       num_devices,i, result&(~0xf), len, result&0xf);
                    }
                }
            }
            vga_init_vsync(sh_pci_devs[num_devices]);
    		init_waitqueue_head(&sh_pci_devs[num_devices]->vsync_wait);
#ifndef NO_TASK
    		init_waitqueue_head(&sh_pci_devs[num_devices]->startad_wait);
			sh_pci_devs[num_devices]->tq_startad.routine=task_startad;
			sh_pci_devs[num_devices]->tq_startad.data=sh_pci_devs[num_devices];
			sh_pci_devs[num_devices]->tq_startad.sync=0;
			sh_pci_devs[num_devices]->startad=-1;
#endif
			sh_pci_devs[num_devices]->opencount=0;

			sprintf(name, "svga%d", num_devices);
			SLH_SYSFS_ADD_DEVICE(name, num_devices);
			
            num_devices++;
        }
    }
    
    j=0;
    for(i=1; i<num_devices;i++) {
        int k, l;
        k=sh_pci_devs[i]->dev->irq;
        if(k>0 && k<255) {
            for(l=0;l<j;l++) if(k==irqs[l])k=-1;
            if(k>0) {
				irqs[j]=k;
            	j++;
			}
        }
    }
    irqs[j]=-1;

	init_waitqueue_head(&vsync_wait);

#ifndef KERNEL_2_6
	EXPORT_NO_SYMBOLS;
#endif
	
    return 0; /* succeed */

nomem_error:
    for(i=0;i<MAX_NR_DEVICES;i++) 
        if(sh_pci_devs[i]) {
			SLH_SYSFS_REMOVE_DEVICE(i);
			kfree(sh_pci_devs[i]);
		}

	SLH_SYSFS_UNREGISTER;

	devfs_unregister_chrdev(SVGALIB_HELPER_MAJOR, "svgalib_helper");
		
	return result;
}

void cleanup_module(void)
{
    int i;
    for(i=0;i<MAX_NR_DEVICES;i++) 
        if(sh_pci_devs[i]) {
			SLH_SYSFS_REMOVE_DEVICE(i);
            kfree(sh_pci_devs[i]);
        }
	
	SLH_SYSFS_UNREGISTER;
     
#ifdef CONFIG_DEVFS_FS
# ifndef KERNEL_2_6
	devfs_unregister(devfs_handle);
# else
	for (i = 0; i < 8; i++)
		devfs_remove("svga_helper/%d", i);
	devfs_remove("svga_helper");
# endif	
#endif
	
    devfs_unregister_chrdev(SVGALIB_HELPER_MAJOR, "svgalib_helper");

}

MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug, "Debug output level.");

MODULE_PARM(all_devices, "i");
MODULE_PARM_DESC(all_devices, "Give access to all PCI devices, regardless of class.");


#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif

MODULE_AUTHOR("Matan Ziv-Av <matan@svgalib.org>");
MODULE_DESCRIPTION("Generic hardware access to vga cards");
