#include <malloc.h>

#ifdef __MINI_MALLOC__
void *(*__alloca_alloc)(size_t) = __mini_malloc;
#endif
