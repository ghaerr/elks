#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <sys/sysctl.h>
/* fmemalloc/fmemfree - allocate/free from main memory */

#ifndef __C86__
#define DEBUG       1       /* =1 use sysctl, =2 debug output */
#endif

#if DEBUG
#define debug(...)  do { if (debug_level > 1) __dprintf(__VA_ARGS__); } while (0)
static int debug_level = DEBUG;
static unsigned long total;
static unsigned long maxsize;
#else
#define debug(...)
#endif

#define FP_SEG(fp)       ((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define FP_OFF(fp)       ((unsigned)(unsigned long)(void __far *)(fp))
#define MK_FP(seg,off)   ((void __far *)((((unsigned long)(seg)) << 16) | \
                                           ((unsigned int)(off))))

void __far *fmemalloc(unsigned long size)
{
    unsigned short seg;
    unsigned int paras = (unsigned int)((size + 15) >> 4);

#if DEBUG == 1
    sysctl(CTL_GET, "malloc.debug", &debug_level);
#endif
    debug("(%d)FMEMALLOC(%5lu) ", getpid(), size);
    if (_fmemalloc(paras, &seg)) {
        debug("= FAIL\n");
        return 0;
    }
#if DEBUG
    total += size;
    if (size > maxsize) maxsize = size;
    debug("total %lu, maxsize %lu\n", total, maxsize);
#endif
    return MK_FP(seg, 0);
}

int fmemfree(void __far *ptr)
{
    debug("(%d) fmemfree()\n", getpid());    // FIXME get size of allocation
    if (FP_OFF(ptr)) {
        debug("  FAIL (bad pointer)\n");    // FIXME convert to ASSERT
        errno = EINVAL;
        return -1;
    }
    return _fmemfree(FP_SEG(ptr));
}
