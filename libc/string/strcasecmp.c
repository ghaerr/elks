/* From GNU C Library.
 * Licensed under the GNU Lesser General Public License v2.1 or later.
 */
#include <string.h>
#include <ctype.h>

int strcasecmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;
    int result;

    while ((result = tolower(*p1) - tolower(*p2++)) == 0)
        if (*p1++ == '\0')
            break;

    return result;
}
