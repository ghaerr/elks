#include <malloc.h>

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

/* request paras from main memory, returns segment */
int _fmemalloc(int paras, unsigned short *pseg);

/* alloc from main memory */
void __far *fmemalloc(unsigned long size)
{
    unsigned short seg;
    unsigned int paras = (unsigned int)((size + 15) >> 4);

    if (_fmemalloc(paras, &seg))
        return 0;
    return _MK_FP(seg, 0);
}
