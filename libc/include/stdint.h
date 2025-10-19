/* For application compatibility, not used in ELKS C Library except libc/math */

#ifdef __WATCOMC__
#include <watcom/stdint.h>
#endif

#ifdef __C86__
#include <c86/stdint.h>
#endif

#ifdef __GNUC__
#include <stdint-gcc.h>
#endif
