/*
 * tcpdump - display network traffic via ktcp netconf protocol
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NETCONF_PORT	2
#define NS_START_CAPTURE	11
#define NS_STOP_CAPTURE		12

struct capture_hdr {
    unsigned char  direction;
    unsigned short pktlen;
};

#define ETH_TYPE_IP	0x0008
#define ETH_TYPE_ARP	0x0608

struct ethhdr {
    unsigned char  dst[6];
    unsigned char  src[6];
    unsigned short type;
};

#define ARP_REQUEST	1
#define ARP_REPLY	2

struct arphdr {
    unsigned short htype;
    unsigned short ptype;
    unsigned char  hlen;
    unsigned char  plen;
    unsigned short oper;
    unsigned char  sha[6];
    unsigned long  spa;
    unsigned char  tha[6];
    unsigned long  tpa;
};

#define IPPROTO_ICMP	1
#define IPPROTO_TCP	6
#define IPPROTO_UDP	17

struct iphdr {
    unsigned char  ver_ihl;
    unsigned char  tos;
    unsigned short tot_len;
    unsigned short id;
    unsigned short frag_off;
    unsigned char  ttl;
    unsigned char  protocol;
    unsigned short check;
    unsigned long  saddr;
    unsigned long  daddr;
};

#define ICMP_ECHO_REPLY	0
#define ICMP_ECHO	8
#define ICMP_TIME_EXCEEDED	11

struct icmphdr {
    unsigned char  type;
    unsigned char  code;
    unsigned short chksum;
    unsigned short id;
    unsigned short seq;
};

struct tcphdr {
    unsigned short sport;
    unsigned short dport;
    unsigned long  seq;
    unsigned long  ack;
    unsigned char  data_off;
    unsigned char  flags;
    unsigned short window;
    unsigned short chksum;
    unsigned short urgpnt;
};

struct udphdr {
    unsigned short sport;
    unsigned short dport;
    unsigned short len;
    unsigned short chksum;
};

#define TCP_FLAG_FIN	0x01
#define TCP_FLAG_SYN	0x02
#define TCP_FLAG_RST	0x04
#define TCP_FLAG_PSH	0x08
#define TCP_FLAG_ACK	0x10
#define TCP_FLAG_URG	0x20

#define MAX_PKT		1536

static int stopflag;
static int s;

static void handle_sigint(int sig)
{
    stopflag = 1;
}

static int read_full(int fd, void *buf, int len)
{
    int rd, off = 0;
    while (off < len) {
	rd = read(fd, (unsigned char *)buf + off, len - off);
	if (rd <= 0) return -1;
	off += rd;
    }
    return 0;
}

static void print_mac(unsigned char *mac)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
	   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void print_ip(unsigned long ip)
{
    printf("%s", in_ntoa(ip));
}

static void print_packet(unsigned char *buf, int len, int dir)
{
    struct ethhdr *eth = (struct ethhdr *)buf;

    printf("%s ", dir ? "OUT" : "IN ");

    if (eth->type == ETH_TYPE_IP &&
		len >= (int)sizeof(struct ethhdr) + (int)sizeof(struct iphdr)) {
	struct iphdr *ip = (struct iphdr *)(buf + sizeof(struct ethhdr));
	unsigned char *payload = (unsigned char *)ip + ((ip->ver_ihl & 0x0f) * 4);

	printf("IP ");
	print_ip(ip->saddr);
	printf(" > ");
	print_ip(ip->daddr);

	switch (ip->protocol) {
	case IPPROTO_ICMP: {
	    struct icmphdr *icmp = (struct icmphdr *)payload;
	    switch (icmp->type) {
	    case ICMP_ECHO_REPLY:   printf(" ICMP echo reply"); break;
	    case ICMP_ECHO:         printf(" ICMP echo request"); break;
	    case ICMP_TIME_EXCEEDED: printf(" ICMP time exceeded"); break;
	    default:                printf(" ICMP type=%d", icmp->type); break;
	    }
	    break;
	}
	case IPPROTO_TCP: {
	    struct tcphdr *tcp = (struct tcphdr *)payload;
	    unsigned char flg = tcp->flags;
	    int comma = 0;
	    printf(" TCP %u > %u", ntohs(tcp->sport), ntohs(tcp->dport));
	    printf(" [");
	    if (flg & TCP_FLAG_SYN) { printf("SYN"); comma = 1; }
	    if (flg & TCP_FLAG_ACK) { printf("%sACK", comma ? "," : ""); comma = 1; }
	    if (flg & TCP_FLAG_FIN) { printf("%sFIN", comma ? "," : ""); comma = 1; }
	    if (flg & TCP_FLAG_RST) { printf("%sRST", comma ? "," : ""); comma = 1; }
	    if (flg & TCP_FLAG_PSH) { printf("%sPSH", comma ? "," : ""); comma = 1; }
	    if (flg & TCP_FLAG_URG) { printf("%sURG", comma ? "," : ""); comma = 1; }
	    printf("]");
	    printf(" seq %lu", ntohl(tcp->seq));
	    if (flg & TCP_FLAG_ACK)
		printf(" ack %lu", ntohl(tcp->ack));
	    break;
	}
	case IPPROTO_UDP: {
	    struct udphdr *udp = (struct udphdr *)payload;
	    printf(" UDP %u > %u", ntohs(udp->sport), ntohs(udp->dport));
	    break;
	}
	default:
	    printf(" proto=%d", ip->protocol);
	    break;
	}
	printf("\n");
    } else if (eth->type == ETH_TYPE_ARP &&
		len >= (int)sizeof(struct ethhdr) + (int)sizeof(struct arphdr)) {
	struct arphdr *arp = (struct arphdr *)(buf + sizeof(struct ethhdr));
	printf("ARP ");
	if (arp->oper == htons(ARP_REQUEST)) {
	    printf("who-has ");
	    print_ip(arp->tpa);
	    printf(" tell ");
	    print_ip(arp->spa);
	} else if (arp->oper == htons(ARP_REPLY)) {
	    print_ip(arp->spa);
	    printf(" is-at ");
	    print_mac(arp->sha);
	} else {
	    printf("op=%d", ntohs(arp->oper));
	}
	printf("\n");
    } else {
	printf("type 0x%04x len %d\n", eth->type, len);
    }
}

int main(int argc, char **argv)
{
    struct sockaddr_in local, rem;
    unsigned char buf[MAX_PKT];
    int count = -1;
    int captured = 0;
    int ch;

    unsigned char cmd_start[2] = { NS_START_CAPTURE, 0 };

    signal(SIGINT, handle_sigint);

    while ((ch = getopt(argc, argv, "c:")) != -1) {
	switch (ch) {
	case 'c':
	    count = atoi(optarg);
	    break;
	default:
	    fprintf(stderr, "Usage: tcpdump [-c count]\n");
	    return 1;
	}
    }

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	perror("socket");
	return 1;
    }

    local.sin_family = AF_INET;
    local.sin_port = 0;
    local.sin_addr.s_addr = 0;
    if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0) {
	perror("bind");
	close(s);
	return 1;
    }

    rem.sin_family = AF_INET;
    rem.sin_addr.s_addr = 0;
    rem.sin_port = htons(NETCONF_PORT);
    if (connect(s, (struct sockaddr *)&rem, sizeof(rem)) < 0) {
	fprintf(stderr, "tcpdump: ktcp not running\n");
	close(s);
	return 1;
    }

    write(s, cmd_start, 2);

    while (!stopflag && (count < 0 || captured < count)) {
	struct capture_hdr hdr;
	unsigned short pktlen;

	if (read_full(s, &hdr, sizeof(hdr)) < 0)
	    break;


	pktlen = ntohs(hdr.pktlen);
	if (pktlen > MAX_PKT)
	    pktlen = MAX_PKT;

	if (read_full(s, buf, pktlen) < 0)
           break;

	print_packet(buf, pktlen, hdr.direction);
	captured++;
    }

    {
	unsigned char cmd[2] = { NS_STOP_CAPTURE, 0 };
	write(s, cmd, 2);
    }

    close(s);
    return 0;
}
