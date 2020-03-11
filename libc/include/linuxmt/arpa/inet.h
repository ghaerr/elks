#ifndef _ARPA_INET_H_
#define	_ARPA_INET_H_

#ifndef ntohs

#define ntohs(x)	( (((x) >> 8) & 0xff) | ((x) << 8) )
#define htons(x)	ntohs(x)
#endif

/* do not use <<24 and >>24 with bcc */
#define ntohl(x)	( ((((unsigned long)x) /0x1000000) & 0xff)	| \
			  ((((unsigned long)x) >> 8 ) & 0xff00)		| \
			  ((((unsigned long)x) & 0xff00) << 8)		| \
			  ((((unsigned long)x) & 0xff) *0x1000000) )

#define	htonl(x)	ntohl(x)

#endif	/* _ARPA_INET_H_ */
