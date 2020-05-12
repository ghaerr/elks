#ifndef ICMP_H
#define ICMP_H

#define PROTO_ICMP	1

int icmp_init(void);
void icmp_process(struct iphdr_s *iph, char *packet);

#endif
