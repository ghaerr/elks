#include <stdio.h>
#include <arpa/inet.h>

/* ip address to ascii*/
char *in_ntoa(ipaddr_t in)
{
	register unsigned char *p;
	static char b[18];

	p = (unsigned char *)&in;
	sprintf(b, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
	return b;
}
