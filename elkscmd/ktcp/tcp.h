#ifndef TCP_H
#define TCP_H

#include <sys/types.h>
#include "config.h"
#include "timer.h"
#include "ip.h"
#include <linuxmt/tcpdev.h>
#include <arpa/inet.h>

/*
 * /dev/tcpdev max read/write size
 * must be at least as big as CB_NORMAL_BUFSIZ
 * and at least as big as TCPDEV_INBUFFERSIZE in <linuxmt/tcpdev.h> (currently 1500)
 */
#define TCPDEV_BUFSIZ	(CB_NORMAL_BUFSIZ + sizeof(struct tdb_return_data))

/* max tcp buffer size (no ip header)*/
#define TCP_BUFSIZ      (TCPDEV_BUFSIZ + sizeof(tcphdr_t) + TCP_OPT_MSS_LEN)

/* max ip buffer size (with link layer frame)*/
#define IP_BUFSIZ       (TCP_BUFSIZ + sizeof(iphdr_t) + sizeof(struct ip_ll))

/*
 * control block input buffer size - max window size, doesn't have to be power of two
 * default will be (ETH_MTU - IP_HDRSIZ) * 3 = (1500-40) * 3 = 4380
 */
#define CB_NORMAL_BUFSIZ    4380    /* normal input buffer size*/
#define USE_SWS             0       /* =1 to use silly window algorithm */

/* max outstanding send window size */
#define TCP_SEND_WINDOW_MAX 1800    /* should be less than TCP_RETRANS_MAXMEM
                                     * was 1024, 1560 works well, higher is experimental
                                     * too high is bad for XT type slow systems */

/* threshold to wait before pushing data to application (turned off for now) */
//#define PUSH_THRESHOLD    512

/* timeout values - unit is set by 'Now' in ktcp.c, currently 60ms */
#define TIMEOUT_ENTER_WAIT  (4<<4)  /* TIME_WAIT state (was 30, then 10, now 4 secs) */
#define TIMEOUT_CLOSE_WAIT  (10<<4) /* CLOSING/LAST_ACK/FIN_WAIT states (10 secs) */
                                    /* Initial RTT & RTO values are not important,
                                     * as they get adjusted automatically as soon
                                     * as the connection is up and running. Recommended
                                     * values per RFC 1122 are RTT=0, RTO=3s */
#define TIMEOUT_INITIAL_RTT 1       /* Still we set RTT to 1 because the smoothing
                                     * algorithm is (currently) disabled when RTT=0 */
#define TIMEOUT_INITIAL_RTO (3<<4)  /* 3 seconds */
#define TCP_RETRANS_MAXWAIT (4<<4)  /* max retransmit wait (4 secs) */
#define TCP_RETRANS_MINWAIT_SLIP 8  /* min retrans timeout for slip/cslip (1/2 sec) */
#define TCP_RETRANS_MINWAIT_ETH  4  /* min retrans timeout for ethernet (1/4 sec) */
#define TCP_RETRANS_ADJUST  (3*TIME_CNV_FACTOR) /* retrans multiplier for slow peers */

/* retransmit settings */
#define TCP_RTT_ALPHA       40      /* was 90, need much faster convergence for
                                     * slow systems */
#define TCP_RETRANS_MAXMEM  5120    /* max retransmit total memory (was 4096) */
#define TCP_RETRANS_MAXTRIES 6      /* max # retransmits (~12 secs total) */

/* Slow start/congestion avoidance */
#define TCP_INIT_CWND       2       /* initial congestion window, don't set to 1 */
                                    /* will cause connection deadlock on slow systems */
#define TCP_INIT_SSTHRESH   100     /* SS threshold, could be infinity */

#define SEQ_LT(a,b)     ((long)((a)-(b)) < 0)
#define SEQ_LEQ(a,b)    ((long)((a)-(b)) <= 0)
#define SEQ_GT(a,b)     ((long)((a)-(b)) > 0)
#define SEQ_GEQ(a,b)    ((long)((a)-(b)) >= 0)

#define PROTO_TCP       0x06

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
	__u8	retrans_act;		/* set when a retrans has been sent on a connection */
	__u16	rtt;			/* roundtriptime */
	__u16	cwnd;			/* congestion window */
	__u16	inflight;		/* # of unacked packets for this cb */
	__u16	ssthresh;		/* slow start threshold */
	__u32	sstimer;		/* congestion avoidance timer EXPERIMENTAL */

	__u32	time_wait_exp;

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

/* TCP options */
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
	struct tcp_retrans_list_s *prev;
	struct tcp_retrans_list_s *next;
	int		retrans_num;	/* retrans counter for this packet */
	__u16	 	rto;		/* retrans timeout */
	timeq_t 	next_retrans;	/* time to resend, initially first_trans + rto */
	timeq_t 	first_trans;	/* time initially sent */
	struct tcpcb_s *cb;		/* connection this packet belongs to */
	struct addr_pair apair;		/* actual addr, may be different
					 * from packet addr for routing */
	__u16		len;		/* data length, placed after header */
	struct tcphdr_s tcphdr[];
};

extern int tcp_timeruse;        /* retrans timer active, call tcp_retrans */
extern int cbs_in_time_wait;    /* time_wait timer active, call tcp_expire_timeouts */
extern int cbs_in_user_timeout; /* fin_wait/closing/last_ack active, call " */
extern int tcpcb_need_push;     /* push required,tcpcb_push_data/call notify_data_avail */
extern int tcp_retrans_memory;  /* total retransmit memory in use */

struct tcpcb_list_s *tcpcb_new(int bufsize);
struct tcpcb_list_s *tcpcb_find(__u32 addr, __u16 lport, __u16 rport);
struct tcpcb_list_s *tcpcb_find_by_sock(void *sock);
void tcpcb_update_sstimer(void);

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
void tcp_reject(struct iphdr_s *);


void hexdump(unsigned char *addr, int count, int summary, char *prefix);

extern char *tcp_states[];      /* used in DEBUG_CLOSE only*/
#endif
