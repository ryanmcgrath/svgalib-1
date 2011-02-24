/* vram size in pages */

#ifdef __386__

#include <linux/version.h>
#include <asm/uaccess.h>
#include <asm/system.h>   /* cli(), *_flags */
#include <asm/segment.h>  /* memcpy and such */
#include <asm/io.h>
#include <asm/pgtable.h>

#define I810_SIZE 1024

unsigned long i810_alloc_page()
{
	unsigned long address;

	address = __get_free_page(GFP_KERNEL);
	if(address == 0UL)
		return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,3,0)
	atomic_inc(&virt_to_page(address)->count);
	set_bit(PG_locked, &virt_to_page(address)->flags);
#endif
	return address;
}

#if 0
static void i810_free_page(unsigned long page)
{
	if(page == 0UL)
		return;

	atomic_dec(&virt_to_page(page)->count);
	clear_bit(PG_locked, &virt_to_page(page)->flags);
	wake_up(&virt_to_page(page)->wait);
	free_page(page);
	return;
}
#endif

static unsigned long i810_pages[I810_SIZE];
unsigned long i810_gttes[I810_SIZE];
static unsigned long i810_gttpage=0;

unsigned int i810_make_gtt() {
	int i;
        
        if(i810_gttpage) return virt_to_bus((void *)i810_gttpage);
        
	i810_gttpage=i810_alloc_page();
	
	for(i=0;i<I810_SIZE;i++) {
            	unsigned int gtte;
		i810_pages[i]=i810_alloc_page();
                gtte=virt_to_bus((void *)i810_pages[i]);
                gtte &= 0x3ffff000;
                gtte |= 1; /* valid, system memory, not cached */
                i810_gttes[i]=gtte;
	}

        return virt_to_bus((void *)i810_gttpage);
}

#endif
