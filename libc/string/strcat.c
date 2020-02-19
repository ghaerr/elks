#include <string.h>

char *
strcat(char *d, const char * s)
{
   (void) strcpy(d+strlen(d), s);
   return d;
}
