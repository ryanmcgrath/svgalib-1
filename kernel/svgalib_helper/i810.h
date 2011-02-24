#define I810_SIZE 1024

unsigned long i810_alloc_page(void);
unsigned int i810_make_gtt(void);
extern unsigned long i810_gttes[I810_SIZE];

