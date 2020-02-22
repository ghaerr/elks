#include <malloc.h>

void *(*__alloca_alloc) __P ((size_t)) = __mini_malloc;
