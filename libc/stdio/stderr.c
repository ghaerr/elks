#include <stdio.h>

#ifndef buferr
static unsigned char buferr[BUFSIZ];
#endif

FILE  stderr[1] =
{
   {
    buferr,
    buferr,
    buferr,
    buferr,
    buferr + sizeof(buferr),
    2,
    _IONBF | __MODE_WRITE | __MODE_IOTRAN
   }
};
