#include <string.h>

int
memcmp(const void *s1, const void *s2, size_t n)
{
    const unsigned char *su1 = s1;
    const unsigned char *su2 = s2;
    char res = 0;

    for (; n > 0; n--)
	if ((res = *su1++ - *su2++))
            break;

    return res;
}
