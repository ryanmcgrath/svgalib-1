#ifdef __PPC

#undef memset
static inline void memset(void *a, int c, int s)
{
   int i;
   for(i=0;i<s;i++) *((unsigned char *)a + i) = c;
}

#if 0
#undef bzero
static inline void bzero(void *a, int s)
{
   int i;
   for(i=0;i<s;i++) *((unsigned char *)a + i) = 0;
}
#endif

#else /* not PPC */
#include <string.h>
#endif
