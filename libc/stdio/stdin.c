#include <stdio.h>

static unsigned char bufin[BUFSIZ];

FILE  stdin[1] =
{
   {
    bufin,
    bufin,
    bufin,
    bufin,
    bufin + sizeof(bufin),
    0,
    _IOFBF | __MODE_READ | __MODE_IOTRAN
   }
};
