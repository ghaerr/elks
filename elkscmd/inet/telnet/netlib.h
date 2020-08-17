typedef __u32 ipaddr_t;		/* ip address in network byte order*/

#define INADDR_LOOPBACK	0x7f000001	/* move, from in.h*/

ipaddr_t in_aton(const char *str);
ipaddr_t in_gethostbyname(char *str);
char *in_ntoa(ipaddr_t in);
