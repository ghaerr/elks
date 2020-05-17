#ifndef ICMP_H
#define ICMP_H

#define PROTO_ICMP	1

#define ICMP_ECHO_REPLY			0
#define ICMP_ECHO_DESTUNREACHABLE	3
#define ICMP_ECHO_REQUEST		8

struct icmp_echo_s {
	__u8	type;
	__u8	code;
	__u16	chksum;
	__u16	id;
	__u16	seqnum;
};

int icmp_init(void);
void icmp_process(struct iphdr_s *iph, unsigned char *packet);

#endif
