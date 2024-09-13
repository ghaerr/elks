#include <stdio.h>

static unsigned char bufout[BUFSIZ];

FILE  stdout[1] =
{
   {
    bufout,
    bufout,
    bufout,
    bufout,
    bufout + sizeof(bufout),
    1,
#ifdef __WATCOMC__
    _IOLBF | __MODE_WRITE | __MODE_IOTRAN   /* FIXME flush on exit to fix */
#else
    _IOFBF | __MODE_WRITE | __MODE_IOTRAN
#endif
   }
};
