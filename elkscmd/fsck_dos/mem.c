/* malloc/free wholesale replacement for 8086 toolchain */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MALLOC_ARENA_SIZE   8192  /* size of initial arena fmemalloc (max 65520)*/
#define MALLOC_ARENA_THRESH 1500U   /* max size to allocate from arena-managed heap */

unsigned int malloc_arena_size = MALLOC_ARENA_SIZE;
unsigned int malloc_arena_thresh = MALLOC_ARENA_THRESH;

#define FP_SEG(fp)          ((unsigned)((unsigned long)(void __far *)(fp) >> 16))
#define FP_OFF(fp)          ((unsigned)(unsigned long)(void __far *)(fp))

static void __far *heap;

void *hmalloc(unsigned long size)
{
    char *p;

    if (heap == NULL) {
        heap = fmemalloc(malloc_arena_size);
        if (!heap) {
            __dprintf("FATAL: Can't fmemalloc %u\n", malloc_arena_size);
            system("meminfo > /dev/console");
            exit(1);
        }
        _fmalloc_add_heap(heap, malloc_arena_size);
    }

    if (size <= malloc_arena_thresh)
        p = _fmalloc(size);
    else p = fmemalloc(size);
    return p;
}

void *malloc(size_t size)
{
    return hmalloc(size);
}

void *hcalloc(unsigned long count, size_t size)
{
    char *mem;
    char __huge *clr;

    count *= size;
    clr = mem = hmalloc(count);
    if (!mem) return NULL;
    do {
        unsigned long n = count;
        if (n > 16384) n = 16384;
        fmemset(clr, '\0', n);
        count -= n;
        clr += n;
    } while (count);
    return mem;
}

void free(void *ptr)
{
    if (ptr == NULL)
        return;
    if (FP_OFF(ptr) == 0)       /* non-arena pointer */
        fmemfree(ptr);
    else
        _ffree(ptr);
}

#if UNUSED
void *realloc(void *ptr, size_t size)
{
    void *new;
    size_t osize = size;

    if (ptr == 0)
        return hmalloc(size);

#if LATER
    /* we can't yet get size from fmemalloc'd block */
    osize = _fmalloc_usable_size(ptr);
    __dprintf("old %u new %u\n", osize, size);
    if (size < osize || osize == 0)
        osize = size;           /* copy less bytes in memcpy below */
#endif

    new = hmalloc(size);
    if (new == 0) {
        __dprintf("realloc: Out of memory\n");
        return 0;
    }
    memcpy(new, ptr, osize);    /* FIXME copies too much but can't get real osize */
    free(ptr);
    return new;
}
#endif
