#ifdef L_setpgrp
#include <unistd.h>

int
setpgrp(void)
{
   return setpgid(0,0);
}
#endif
