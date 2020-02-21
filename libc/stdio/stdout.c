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
    _IOFBF | __MODE_WRITE | __MODE_IOTRAN
   }
};
