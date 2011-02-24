#ifndef KERNEL_VERSION
# include <linux/version.h>
#endif
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)

# ifdef KERNEL_2_6
#  undef KERNEL_2_6
# endif

# define PCI_GET_CLASS pci_find_class
# define PCI_GET_DEVICE pci_find_device

# if defined (PG_chainlock)
#  define my_io_remap_page_range(vma, start, ofs, len, prot) \
		io_remap_page_range(vma,start,ofs,len,prot)
# else
#  ifdef __alpha__ /* Is alpha really the issue here ??? */
#   define my_io_remap_page_range(vma, start, ofs, len, prot) \
		   remap_page_range(start,ofs,len,prot)
#  else
#   define my_io_remap_page_range(vma, start, ofs, len, prot) \
		   io_remap_page_range(start,ofs,len,prot)
#  endif
# endif
# ifndef IRQ_HANDLED 
typedef void irqreturn_t;
#  define IRQ_NONE
#  define IRQ_HANDLED
# endif

#else /* Kernel 2.6 */

#define NO_TASK

# ifndef KERNEL_2_6
#  define KERNEL_2_6
# endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,16)
# undef MODULE_PARM
# define MODULE_PARM(x,y) module_param(x, int, 0)
#endif

/* WHY ? */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10)
# define PCI_GET_CLASS pci_find_class
# define PCI_GET_DEVICE pci_find_device
# define DEVFS_MK_SYMLINK(a,b) devfs_mk_symlink(a,b)
#else
# define PCI_GET_CLASS pci_get_class
# define PCI_GET_DEVICE pci_get_device
# define DEVFS_MK_SYMLINK(a,b)
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13)
# define io_remap_page_range(vma, vaddr, paddr, size, prot) \
	remap_pfn_range(vma, vaddr, (paddr) >> PAGE_SHIFT, size, prot)
#endif

# define my_io_remap_page_range(vma, start, ofs, len, prot) \
		io_remap_page_range(vma,start,ofs,len,prot)

# define pci_present() 1

# ifdef CONFIG_DEVFS_FS
typedef void* devfs_handle_t;
# endif

#endif

/* These are also not present in 2.6 kernels ... */
#if (!defined _LINUX_DEVFS_FS_KERNEL_H) || (defined KERNEL_2_6)
static inline int devfs_register_chrdev (unsigned int major, const char *name,
                                         struct file_operations *fops)
{
    return register_chrdev (major, name, fops);
}
static inline int devfs_unregister_chrdev (unsigned int major,const char *name)
{
    return unregister_chrdev (major, name);
}
#endif

#if defined(KERNEL_2_6) && !defined(CONFIG_DEVFS_FS)

# if !defined(CLASS_SIMPLE) /* no class_simple */

# define K_CLASS class
#  define SLH_SYSFS_REGISTER                                            \
     svgalib_helper_class = class_create(THIS_MODULE, "svgalib_helper");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,15)
#  define SLH_SYSFS_ADD_CONTROL                                         \
     class_device_create(svgalib_helper_class,                      	\
                             MKDEV(SVGALIB_HELPER_MAJOR, 0),            \
                             NULL, "svga");

#  define SLH_SYSFS_ADD_DEVICE(_name, _minor)                           \
     class_device_create(svgalib_helper_class,                      	\
                             MKDEV(SVGALIB_HELPER_MAJOR, _minor),       \
                             &sh_pci_devs[_minor]->dev->dev, _name);
#else /* 2.6.15 changed class_device_create */
#  define SLH_SYSFS_ADD_CONTROL                                         \
     class_device_create(svgalib_helper_class, NULL,                	\
                             MKDEV(SVGALIB_HELPER_MAJOR, 0),            \
                             NULL, "svga");

#  define SLH_SYSFS_ADD_DEVICE(_name, _minor)                           \
     class_device_create(svgalib_helper_class, NULL,                	\
                             MKDEV(SVGALIB_HELPER_MAJOR, _minor),       \
                             &sh_pci_devs[_minor]->dev->dev, _name);
#endif /* 2.6.15 */

#  define SLH_SYSFS_REMOVE_DEVICE(i)                                    \
     class_destroy(svgalib_helper_class);

#  define SLH_SYSFS_UNREGISTER                                          \
     class_destroy(svgalib_helper_class);

# else /* class_simple */

# define K_CLASS class_simple
#  define SLH_SYSFS_REGISTER                                            \
     svgalib_helper_class = class_simple_create(THIS_MODULE, "svgalib_helper");

#  define SLH_SYSFS_ADD_CONTROL                                         \
     class_simple_device_add(svgalib_helper_class,                      \
                             MKDEV(SVGALIB_HELPER_MAJOR, 0),            \
                             NULL, "svga");

#  define SLH_SYSFS_ADD_DEVICE(_name, _minor)                           \
     class_simple_device_add(svgalib_helper_class,                      \
                             MKDEV(SVGALIB_HELPER_MAJOR, _minor),       \
                             &sh_pci_devs[_minor]->dev->dev, _name);

#  define SLH_SYSFS_REMOVE_DEVICE(i)                                    \
     class_simple_device_remove(MKDEV(SVGALIB_HELPER_MAJOR, i));

#  define SLH_SYSFS_UNREGISTER                                          \
     class_simple_destroy(svgalib_helper_class);
# endif /* class_simple */

#else
#  define SLH_SYSFS_REGISTER
#  define SLH_SYSFS_ADD_CONTROL
#  define SLH_SYSFS_ADD_DEVICE(_name, _minor)
#  define SLH_SYSFS_REMOVE_DEVICE(i)
#  define SLH_SYSFS_UNREGISTER
#endif

#if (defined MINOR)
# define my_minor(x) MINOR(x)
#else
# define my_minor(x) minor(x)
#endif

#if !defined(MODULE_VERSION) 
# define MODULE_VERSION(x)
#endif

#ifndef PCI_VENDOR_ID_RENDITION 
#define PCI_VENDOR_ID_RENDITION               0x1163
#endif
