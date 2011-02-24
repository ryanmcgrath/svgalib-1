#ifdef SVGA_HELPER_VIRTUAL

#include <linux/version.h>
#include <asm/uaccess.h>
#include <asm/system.h>   /* cli(), *_flags */
#include <asm/segment.h>  /* memcpy and such */
#include <asm/io.h>
#include <asm/pgtable.h>
#include <linux/pagemap.h>
#include <linux/wrapper.h>
#include "svgalib_helper.h"

/*******************************/
/* Memory management functions */
/* from kernel:/drivers/media/video/bttv-driver.c */
/*******************************/

#define MDEBUG(x)	do { } while(0)		/* Debug memory management */

/* [DaveM] I've recoded most of this so that:
 * 1) It's easier to tell what is happening
 * 2) It's more portable, especially for translating things
 *    out of vmalloc mapped areas in the kernel.
 * 3) Less unnecessary translations happen.
 *
 * The code used to assume that the kernel vmalloc mappings
 * existed in the page tables of every process, this is simply
 * not guarenteed.  We now use pgd_offset_k which is the
 * defined way to get at the kernel page tables.
 */

/* Given PGD from the address space's page table, return the kernel
 * virtual mapping of the physical memory mapped at ADR.
 */
static inline unsigned long uvirt_to_kva(pgd_t *pgd, unsigned long adr)
{
        unsigned long ret = 0UL;
	pmd_t *pmd;
	pte_t *ptep, pte;
  
	if (!pgd_none(*pgd)) {
                pmd = pmd_offset(pgd, adr);
                if (!pmd_none(*pmd)) {
                        ptep = pte_offset(pmd, adr);
                        pte = *ptep;
                        if(pte_present(pte)) {
				ret  = (unsigned long) page_address(pte_page(pte));
				ret |= (adr & (PAGE_SIZE - 1));
				
			}
                }
        }
        MDEBUG(printk("uv2kva(%lx-->%lx)", adr, ret));
	return ret;
}

static inline unsigned long uvirt_to_bus(unsigned long adr) 
{
        unsigned long kva, ret;

        kva = uvirt_to_kva(pgd_offset(current->mm, adr), adr);
	ret = virt_to_bus((void *)kva);
        MDEBUG(printk("uv2b(%lx-->%lx)", adr, ret));
        return ret;
}

static inline unsigned long uvirt_to_pa(unsigned long adr) 
{
        unsigned long kva, ret;

        kva = uvirt_to_kva(pgd_offset(current->mm, adr), adr);
	ret = virt_to_phys((void *)kva);
        MDEBUG(printk("uv2b(%lx-->%lx)", adr, ret));
        return ret;
}

static inline unsigned long kvirt_to_bus(unsigned long adr) 
{
        unsigned long va, kva, ret;

        va = VMALLOC_VMADDR(adr);
        kva = uvirt_to_kva(pgd_offset_k(va), va);
	ret = virt_to_bus((void *)kva);
        MDEBUG(printk("kv2b(%lx-->%lx)", adr, ret));
        return ret;
}

/* Here we want the physical address of the memory.
 * This is used when initializing the contents of the
 * area and marking the pages as reserved.
 */
static inline unsigned long kvirt_to_pa(unsigned long adr) 
{
        unsigned long va, kva, ret;

        va = VMALLOC_VMADDR(adr);
        kva = uvirt_to_kva(pgd_offset_k(va), va);
	ret = __pa(kva);
        MDEBUG(printk("kv2pa(%lx-->%lx)", adr, ret));
        return ret;
}

static void * rvmalloc(signed long size)
{
	void * mem;
	unsigned long adr, page;

	mem=vmalloc_32(size);
	if (mem) 
	{
		memset(mem, 0, size); /* Clear the ram out, no junk to the user */
	        adr=(unsigned long) mem;
		while (size > 0) 
                {
	                page = kvirt_to_pa(adr);
			mem_map_reserve(virt_to_page(__va(page)));
			adr+=PAGE_SIZE;
			size-=PAGE_SIZE;
		}
	}
	return mem;
}

static int pag_lock(unsigned long addr)
{
	unsigned long page;
	unsigned long kva;

	kva = uvirt_to_kva(pgd_offset(current->mm, addr), addr);
	if(kva)
	{
	    lock_it:
	    page = uvirt_to_pa((unsigned long)addr);
	    LockPage(virt_to_page(__va(page)));
	    SetPageReserved(virt_to_page(__va(page)));
	}
	else
	{
	    copy_from_user(&page,(char *)addr,1); /* try access it */
	    kva = uvirt_to_kva(pgd_offset(current->mm, addr), addr);
	    if(kva) goto lock_it;
	    else return EPERM;
	}
	return 0;
}

static int pag_unlock(unsigned long addr)
{
	    unsigned long page;
	    unsigned long kva;

	    kva = uvirt_to_kva(pgd_offset(current->mm, addr), addr);
	    if(kva)
	    {
		page = uvirt_to_pa((unsigned long)addr);
		UnlockPage(virt_to_page(__va(page)));
		ClearPageReserved(virt_to_page(__va(page)));
		return 0;
	    }
	    return EPERM;
}


static void rvfree(void * mem, signed long size)
{
        unsigned long adr, page;
        
	if (mem) 
	{
	        adr=(unsigned long) mem;
		while (size > 0) 
                {
	                page = kvirt_to_pa(adr);
			mem_map_unreserve(virt_to_page(__va(page)));
			adr+=PAGE_SIZE;
			size-=PAGE_SIZE;
		}
		vfree(mem);
	}
}


int dhahelper_virt_to_phys(dhahelper_vmi_t *arg)
{
	dhahelper_vmi_t mem;
	unsigned long i,nitems;
	char *addr;
	if (copy_from_user(&mem, arg, sizeof(dhahelper_vmi_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy from userspace\n");
		return -EFAULT;
	}
	nitems = mem.length / PAGE_SIZE;
	if(mem.length % PAGE_SIZE) nitems++;
	addr = mem.virtaddr;
	for(i=0;i<nitems;i++)
	{
	    unsigned long result;
	    result = uvirt_to_pa((unsigned long)addr);
	    if (copy_to_user(&mem.realaddr[i], &result, sizeof(unsigned long)))
	    {
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy to userspace\n");
		return -EFAULT;
	    }
	    addr += PAGE_SIZE;
	}
	return 0;
}

int dhahelper_virt_to_bus(dhahelper_vmi_t *arg)
{
	dhahelper_vmi_t mem;
	unsigned long i,nitems;
	char *addr;
	if (copy_from_user(&mem, arg, sizeof(dhahelper_vmi_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy from userspace\n");
		return -EFAULT;
	}
	nitems = mem.length / PAGE_SIZE;
	if(mem.length % PAGE_SIZE) nitems++;
	addr = mem.virtaddr;
	for(i=0;i<nitems;i++)
	{
	    unsigned long result;
	    result = uvirt_to_bus((unsigned long)addr);
	    if (copy_to_user(&mem.realaddr[i], &result, sizeof(unsigned long)))
	    {
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy to userspace\n");
		return -EFAULT;
	    }
	    addr += PAGE_SIZE;
	}
	return 0;
}


int dhahelper_alloc_pa(dhahelper_mem_t *arg)
{
	dhahelper_mem_t mem;
	if (copy_from_user(&mem, arg, sizeof(dhahelper_mem_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy from userspace\n");
		return -EFAULT;
	}
	mem.addr = rvmalloc(mem.length);
	if (copy_to_user(arg, &mem, sizeof(dhahelper_mem_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy to userspace\n");
		return -EFAULT;
	}
	return 0;
}

int dhahelper_free_pa(dhahelper_mem_t *arg)
{
	dhahelper_mem_t mem;
	if (copy_from_user(&mem, arg, sizeof(dhahelper_mem_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy from userspace\n");
		return -EFAULT;
	}
	rvfree(mem.addr,mem.length);
	return 0;
}

int dhahelper_lock_mem(dhahelper_mem_t *arg)
{
	dhahelper_mem_t mem;
	int retval;
	unsigned long i,nitems,addr;
	if (copy_from_user(&mem, arg, sizeof(dhahelper_mem_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy from userspace\n");
		return -EFAULT;
	}
	nitems = mem.length / PAGE_SIZE;
	if(mem.length % PAGE_SIZE) nitems++;
	addr = (unsigned long)mem.addr;
	for(i=0;i<nitems;i++)
	{
	    retval = pag_lock((unsigned long)addr);
	    if(retval)
	    {
		unsigned long j;
		addr = (unsigned long)mem.addr;
		for(j=0;j<i;j++)
		{
		    pag_unlock(addr);
		    addr +=  PAGE_SIZE;
		}
		return retval;
	    }
	    addr += PAGE_SIZE;
	}
	return 0;
}

int dhahelper_unlock_mem(dhahelper_mem_t *arg)
{
	dhahelper_mem_t mem;
	int retval;
	unsigned long i,nitems,addr;
	if (copy_from_user(&mem, arg, sizeof(dhahelper_mem_t)))
	{
		if (debug > 0)
		    printk(KERN_ERR "dhahelper: failed copy from userspace\n");
		return -EFAULT;
	}
	nitems = mem.length / PAGE_SIZE;
	if(mem.length % PAGE_SIZE) nitems++;
	addr = (unsigned long)mem.addr;
	for(i=0;i<nitems;i++)
	{
	    retval = pag_unlock((unsigned long)addr);
	    if(retval) return retval;
	    addr += PAGE_SIZE;
	}
	return 0;
}

static int dhahelper_ioctl(struct inode *inode, struct file *file,
    unsigned int cmd, unsigned long arg)
{
    if (debug > 1)
	printk(KERN_DEBUG "dhahelper: ioctl(cmd=%x, arg=%lx)\n",
	    cmd, arg);

    if (MINOR(inode->i_rdev) != 0)
	return -ENXIO;

    switch(cmd)
    {
	case DHAHELPER_GET_VERSION: return dhahelper_get_version((int *)arg);
	case DHAHELPER_PORT:	    return dhahelper_port((dhahelper_port_t *)arg);
	case DHAHELPER_VIRT_TO_PHYS:return dhahelper_virt_to_phys((dhahelper_vmi_t *)arg);
	case DHAHELPER_VIRT_TO_BUS: return dhahelper_virt_to_bus((dhahelper_vmi_t *)arg);
	case DHAHELPER_ALLOC_PA:return dhahelper_alloc_pa((dhahelper_mem_t *)arg);
	case DHAHELPER_FREE_PA: return dhahelper_free_pa((dhahelper_mem_t *)arg);
	case DHAHELPER_LOCK_MEM: return dhahelper_lock_mem((dhahelper_mem_t *)arg);
	case DHAHELPER_UNLOCK_MEM: return dhahelper_unlock_mem((dhahelper_mem_t *)arg);
	default:
    	    if (debug > 0)
		printk(KERN_ERR "dhahelper: invalid ioctl (%x)\n", cmd);
	    return -EINVAL;
    }
    return 0;
}

#endif
