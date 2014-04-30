#ifndef __MYLIB_H__
#define __MYLIB_H__

#ifdef __ELKS__

#ifndef ntohs
#define ntohs(x)	( ((x) >> 8 & 0xff)|((x) << 8) )
#define htons(x)	ntohs(x)
#endif

#define ntohl(x)	((((x) >> 24)&0xff)| \
			(((x) >> 8 )&0xff00)| \
			(((x) & 0xff00) << 8)| \
			(((x) & 0xff) << 24) )

#define	htonl(x)	ntohl(x)
							
#endif

#endif
