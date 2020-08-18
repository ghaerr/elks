#include <arpa/inet.h>

/* ascii to ip address in host byte order*/
ipaddr_t in_aton(const char *str)
{
	unsigned long addr = 0;
	unsigned int val;
	int i;

	for (i = 0; i < 4; i++) {
		addr <<= 8;
		if (*str) {
			val = 0;
			while (*str && *str != '.') {
				val *= 10;
				val += *str++ - '0';
			}
			addr |= val;
			if (*str)
				str++;
		}
	}
	return htonl(addr);
}
