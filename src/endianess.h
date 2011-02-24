#include <endian.h>
#include <byteswap.h>

#if __BYTE_ORDER == __BIG_ENDIAN

#define LE32(x) bswap_32(x)
#define BE32(x) (x)

#else /* little endian */

#define LE32(x) (x)
#define BE32(x) bswap_32(x)

#endif
