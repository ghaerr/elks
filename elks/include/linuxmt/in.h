#ifndef __LINUXMT_IN_H__
#define __LINUXMT_IN_H__

struct in_addr
{
    unsigned long s_addr;
};

struct sockaddr_in
{
    unsigned short sin_family;	/* AF_INET */
    unsigned short sin_port;	/* port in network order */
    struct in_addr sin_addr;
};

#define INADDR_ANY	((unsigned long int) 0x00000000)
#define PORT_ANY	((unsigned int) 0x0000)
#define INADDR_NODE	0xffffffff
#define INADDR_LOOPBACK	0x7f000001

#endif
