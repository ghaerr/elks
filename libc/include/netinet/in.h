#ifndef _ARPA_INET_H_
#define _ARPA_INET_H_

#include <features.h>
#include <sys/types.h>
#include __SYSINC__(in.h)
#include __SYSINC__(un.h)

typedef __u32 ipaddr_t;		/* ip address in network byte order*/

#ifdef __GNUC__
#define ntohs(x)	__builtin_bswap16(x)
#define ntohl(x)	__builtin_bswap32(x)

#else

#define ntohs(x)	( (((x) >> 8) & 0xff) | ((unsigned char)(x) << 8) )
#define ntohl(x)	( ((((unsigned long)x) >> 24) & 0xff)	| \
			  ((((unsigned long)x) >> 8 ) & 0xff00)		| \
			  ((((unsigned long)x) & 0xff00) << 8)		| \
			  ((((unsigned long)x) & 0xff) << 24) )
#endif

#define htons(x)	ntohs(x)
#define	htonl(x)	ntohl(x)

#endif /* _ARPA_INET_H_ */
