#ifndef TCP_H
#define TCP_H

#include <sys/types.h>

#include "config.h"
#include "timer.h"
#include "ip.h"
#include <linuxmt/tcpdev.h>
#include <arpa/inet.h>

/*
 * /etc/tcpdev max read/write size
 * Must be at least as big as CB_NORMAL_BUFSIZ
 * And at least as big as TCPDEV_INBUFFERSIZE in <linuxmt/tcpdev.h> (currently 1500)
 */
#define TCPDEV_BUFSIZ	(CB_NORMAL_BUFSIZ + sizeof(struct tdb_return_data))

/* max tcp buffer size (no ip header)*/
#define TCP_BUFSIZ	(TCPDEV_BUFSIZ + sizeof(tcphdr_t) + TCP_OPT_MSS_LEN)

/* max ip buffer size (with link layer frame)*/
#define IP_BUFSIZ	(TCP_BUFSIZ + sizeof(iphdr_t) + sizeof(struct ip_ll))

/*
 * control block input buffer size - max window size, doesn't have to be power of two
 * default will be (ETH_MTU - IP_HDRSIZ) * 3 = (1500-40) * 3 = 4380
 */
#define CB_NORMAL_BUFSIZ	4380	/* normal input buffer size*/
#define USE_SWS			0	/* =1 to use silly window algorithm */

/* max outstanding send window size*/
#define TCP_SEND_WINDOW_MAX	1024	/* should be less than TCP_RETRANS_MAXMEM*/

/* threshold to wait before pushing data to application (turned off for now) */
//#define PUSH_THRESHOLD	512

/* timeout values in 1/16 seconds, or (seconds << 4). Half second = 8*/
#define TIMEOUT_ENTER_WAIT	(4<<4)	/* TIME_WAIT state (was 30, then 10)*/
#define TIMEOUT_CLOSE_WAIT	(10<<4)	/* CLOSING/LAST_ACK/FIN_WAIT states (was 240)*/
#define TIMEOUT_INITIAL_RTT	(1<<4)	/* initial RTT before retransmit (was 4)*/
#define TCP_RETRANS_MAXWAIT	(4<<4)	/* max retransmit wait (4 secs)*/
#define TCP_RETRANS_MINWAIT_SLIP 8	/* min retrans timeout for slip/cslip (1/2 sec)*/
#define TCP_RETRANS_MINWAIT_ETH	4	/* min retrans timeout for ethernet (1/4 sec)*/

/* retransmit settings*/
#define TCP_RTT_ALPHA			90
#define TCP_RETRANS_MAXMEM		4096	/* max retransmit total memory*/
#define TCP_RETRANS_MAXTRIES		6	/* max # retransmits (~12 secs total)*/

#define SEQ_LT(a,b)	((long)((a)-(b)) < 0)
#define SEQ_LEQ(a,b)	((long)((a)-(b)) <= 0)
#define SEQ_GT(a,b)	((long)((a)-(b)) > 0)
#define SEQ_GEQ(a,b)	((long)((a)-(b)) >= 0)

#define PROTO_TCP	0x06

#define	TF_FIN	0x01
#define TF_SYN	0x02
#define	TF_RST	0x04
#define	TF_PSH	0x08
#define	TF_ACK	0x10
#define TF_URG	0x20

#define TF_ALL (TF_FIN | TF_SYN | TF_RST | TF_PSH | TF_ACK | TF_URG)

#define	TCP_DATAOFF(x)		( (x)->data_off >> 2 )
#define TCP_SETHDRSIZE(c,s)	( (c)->data_off = (s) << 2 )

#define ENTER_TIME_WAIT(cb)	{ (cb)->time_wait_exp = Now + TIMEOUT_ENTER_WAIT; \
				  (cb)->state = TS_TIME_WAIT; \
				  tcp_timeruse++; \
				  cbs_in_time_wait++; }

#define LEAVE_TIME_WAIT(cb)	{ tcp_timeruse--; \
				  cbs_in_time_wait--; }

struct tcphdr_s {
	__u16	sport;
	__u16	dport;
	__u32	seqnum;
	__u32	acknum;
	__u8	data_off;
	__u8	flags;
	__u16	window;
	__u16	chksum;
	__u16	urgpnt;
	__u8	options[];
};

typedef struct tcphdr_s tcphdr_t;

struct iptcp_s {
	struct iphdr_s	*iph;
	struct tcphdr_s	*tcph;
	__u16		tcplen;
};

#define	TS_CLOSED	0
#define	TS_LISTEN	1
#define	TS_SYN_SENT	2
#define	TS_SYN_RECEIVED	3
#define	TS_ESTABLISHED	4
#define	TS_FIN_WAIT_1	5
#define	TS_FIN_WAIT_2	6
#define	TS_CLOSE_WAIT	7
#define	TS_CLOSING	8
#define	TS_LAST_ACK	9
#define	TS_TIME_WAIT	10

#define CB_BUF_SPACE(x)	((x)->buf_size - (x)->buf_used)

struct tcpcb_s {
	void *	newsock;
	void *	sock;			/* the socket in kernel space */

	__u32	localaddr;		/* in network byte order */
	__u32	remaddr;		/* in host byte order */
	__u16	remport;		/* in network byte order */
	__u16	localport;		/* in host byte order */

	__u8	state;
	__u8	unaccepted;		/* boolean */
	timeq_t	rtt;			/* in 1/16 secs*/

	__u32	time_wait_exp;
	//__u16	wait_data;

	short	bytes_to_push;

	__u32	send_una;
	__u32	send_nxt;
	//__u16	send_wnd;
	__u32	iss;

	__u32	rcv_nxt;
	__u16	rcv_wnd;
	__u32	irs;

	__u32	seg_seq;
	__u32	seg_ack;

	__u16	flags;
	__u8	*data;
	__u16	datalen;

	__u16	buf_head;
	__u16	buf_tail;
	__u16	buf_used;		/* # valid bytes in buffer */
	__u16	buf_size;		/* total buffer size */
	__u8	buf_base[];
};

/* TCP options*/
#define TCP_OPT_EOL		0
#define TCP_OPT_NOP		1
#define TCP_OPT_MSS		2

#define TCP_OPT_MSS_LEN		4	/* total MSS option length*/

struct	tcpcb_list_s {
	struct tcpcb_list_s	*prev;
	struct tcpcb_list_s	*next;
	struct tcpcb_s		tcpcb;	/* must be last */
};

struct	tcp_retrans_list_s {
	struct tcp_retrans_list_s	*prev;
	struct tcp_retrans_list_s	*next;

	int				retrans_num;
	timeq_t 			rto;
	timeq_t 			next_retrans;
	timeq_t 			first_trans;

	struct tcpcb_s			*cb;
	struct addr_pair		apair;
	__u16				len;
	struct tcphdr_s 		tcphdr[];

};

extern int tcp_timeruse;	/* retrans timer active, call tcp_retrans */
extern int cbs_in_time_wait;	/* time_wait timer active, call tcp_expire_timeouts */
extern int cbs_in_user_timeout;	/* fin_wait/closing/last_ack active, call " */
extern int tcpcb_need_push;	/* push required, tcpcb_push_data/call tcpcb_checkread */
extern int tcp_retrans_memory;	/* total retransmit memory in use */

struct tcpcb_list_s *tcpcb_new(int bufsize);
struct tcpcb_list_s *tcpcb_find(__u32 addr, __u16 lport, __u16 rport);
struct tcpcb_list_s *tcpcb_find_by_sock(void *sock);

__u16 tcp_chksum(struct iptcp_s *h);
__u16 tcp_chksumraw(struct tcphdr_s *h, __u32 saddr, __u32 daddr, __u16 len);
void tcp_print(struct iptcp_s *head, int recv, struct tcpcb_s *cb);
void tcp_output(struct tcpcb_s *cb);
int tcp_init(void);
void tcp_process(struct iphdr_s *iph);
void tcp_connect(struct tcpcb_s *cb);
void tcp_send_ack(struct tcpcb_s *cb);
void tcp_send_fin(struct tcpcb_s *cb);
void tcp_send_reset(struct tcpcb_s *cb);
void tcp_reset_connection(struct tcpcb_s *cb);

void hexdump(unsigned char *addr, int count, int summary, char *prefix);

extern char *tcp_states[];	/* used in DEBUG_CLOSE only*/
#endif
