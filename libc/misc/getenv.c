/* Copyright (C) 1995,1996 Robert de Bath <rdebath@cix.compulink.co.uk>
 * This file is part of the Linux-8086 C library and is distributed
 * under the GNU Library General Public License.
 */

#include <string.h>
#include <unistd.h>

char *
getenv(const char *name)
{
    size_t l = strlen(name);
    char **ep = environ;

    if (ep == 0 || l == 0)
        return 0;

    while (*ep) {
        if (memcmp(name, *ep, l) == 0 && (*ep)[l] == '=')
            return *ep + l + 1;
        ep++;
    }
    return 0;
}
