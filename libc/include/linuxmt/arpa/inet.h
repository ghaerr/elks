#ifndef _ARPA_INET_H_
#define	_ARPA_INET_H_

#ifndef ntohs
#define ntohs(x)	(((unsigned)(x) >> 8) | ((x) << 8))

#define htons(x)	ntohs(x)
#endif


#define ntohl(x)	( ((((unsigned long)x) >> 24) & 0xff)	| \
			  ((((unsigned long)x) >> 8 ) & 0xff00)		| \
			  ((((unsigned long)x) & 0xff00) << 8)		| \
			  ((((unsigned long)x) & 0xff) << 24) )

#define	htonl(x)	ntohl(x)

#endif	/* _ARPA_INET_H_ */
