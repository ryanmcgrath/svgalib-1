extern int __svgalib_pci_find_vendor_vga_pos(unsigned int vendor, unsigned int *conf);
extern int __svgalib_pci_helper_idev;
extern int __svgalib_pci_nohelper_idev;
extern int __svgalib_pci_card_found_at;
extern void __svgalib_pci_write_config_dword(int pos, int address, unsigned int data);
extern unsigned int __svgalib_pci_read_config_dword(int pos, int address);
extern unsigned long __svgalib_pci_read_aperture_len(int pos, int address);
extern int memorytest(uint32_t *m, int max_mem);
