
#define NETCONF_PORT	2

struct general_stats_s {
	__u8	cb_num;	/* Number of control blocks */
	__u16	retrans_memory;	/*Memory used for retransmition buffers */
};

struct cb_stats_s {
	__u8	valid;
	__u8	state;
	__u16	rtt;	/* Round trip time in ms */
	__u32	remaddr;
	__u16	remport;
	__u16	localport;
};

struct stat_request_s {
	__u8	type;
#define NS_IDLE	0
#define NS_GENERAL	1
#define NS_CB	2
	__u8	extra;
};

void netconf_init();
void netconf_send();
void netconf_request();

