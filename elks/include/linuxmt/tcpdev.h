#ifndef _LINUXMT_TCPDEV_H_
#define _LINUXMT_TCPDEV_H_

/*
 * tcpdev.h header for ELKS kernel
 * Copyright (C) 2001 Harry Kalogirou
 */

#include <linuxmt/in.h>

#define TCP_DEVICE_NAME	"tcpdev"

#define TCPDEV_INBUFFERSIZE	1024
#define TCPDEV_OUTBUFFERSIZE	128

#define TCPDEV_MAXREAD TCPDEV_INBUFFERSIZE - sizeof(struct tdb_return_data)

#define TDC_BIND	1
#define TDC_ACCEPT	2
#define TDC_CONNECT	3
#define TDC_LISTEN	4
#define TDC_RELEASE	5
#define TDC_READ	8
#define TDC_WRITE	9

struct tdb_release {
	unsigned char	cmd;
	unsigned short sock;
};

struct tdb_accept {
	unsigned char	cmd;
	unsigned short sock;
	unsigned short newsock;
	int	nonblock;	
};

struct tdb_listen {
	unsigned char	cmd;
	unsigned short sock;
	int	backlog;
};

struct tdb_bind {
	unsigned char	cmd;
	unsigned short sock;
	struct	sockaddr_in addr;	
};

struct tdb_connect {
	unsigned char	cmd;
	unsigned short sock;
	struct	sockaddr_in addr;
};

struct tdb_read {
	unsigned char 	cmd;
	unsigned short	sock;
	int				size;
	int				nonblock;
};

struct tdb_write {
	unsigned char 	cmd;
	unsigned short	sock;
	int				size;
	int				nonblock;
#define	TDB_WRITE_MAX	100
	unsigned char	data[TDB_WRITE_MAX];
};

#define	TDT_RETURN	1

struct tdb_return_data {
#define	TDT_CHG_STATE	2
#define	TDT_AVAIL_DATA		3	
	char	type;
	int		ret_value;
	unsigned short sock;
	int		size;
	unsigned char data;
};

struct tdb_accept_ret {
	char	type;
	int		ret_value;
	unsigned short sock;
	__u32	addr_ip;
	__u16	addr_port;
};


#endif

