#ifndef AF_NANO_H
#define AF_NANO_H

#include <linuxmt/na.h>

#define NSOCKETS_NANO 16

#define last_nano_data (nano_datas + NSOCKETS_NANO - 1)
#define NA_DATA(SOCK) ((struct nano_proto_data *)(SOCK)->data)
#define NA_BUF_SIZE 64

#define NA_BUF_AVAIL(NPD)	(((NPD)->npd_bp_head - (NPD)->npd_bp_tail) & \
							(NA_BUF_SIZE-1))
#define NA_BUF_SPACE(NPD)	((NA_BUF_SIZE-1) - NA_BUF_AVAIL(NPD))



struct nano_proto_data {
	int		npd_refcnt;
	struct socket * npd_socket;
#if BLOAT_NET
	int		npd_protocol;
#endif
	struct sockaddr_na npd_sockaddr_na;
	short		npd_sockaddr_len;
	char		npd_buf[NA_BUF_SIZE];
	int		npd_bp_head, npd_bp_tail;
	int		npd_srvno;
	struct nano_proto_data *	npd_peerupd;
	struct wait_queue npd_wait;
	int		npd_lock_flag;
	short npd_sem;
};

#endif AF_NANO_H
