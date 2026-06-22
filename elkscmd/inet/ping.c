/*
 * ping - send ICMP ECHO_REQUEST packets via ktcp netconf protocol
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define DEFAULT_INTERVAL	1000
#define REPLY_TIMEOUT		2000

#define NETCONF_PORT	2
#define NS_ICMP_ECHO	5
#define ICMP_ECHO_REPLY_SUCCESS	0

struct icmp_echo_request_s {
	ipaddr_t       target_ip;
	unsigned short id;
	unsigned short seq;
	unsigned long  timestamp;
};

struct icmp_echo_reply_s {
	unsigned long  timestamp;
	unsigned char  ttl;
	unsigned char  status;
};

static int stopflag;

static void sigint(int sig)
{
	stopflag = 1;
}

static unsigned long get_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000UL + tv.tv_usec / 1000UL;
}

static int ping_via_ktcp(ipaddr_t target, char *hostname, int count, int interval, int s)
{
	unsigned char buf[2 + sizeof(struct icmp_echo_request_s)];
	struct icmp_echo_reply_s reply;
	int seq, sent, recv;
	unsigned long rtt, min_rtt, max_rtt, total_rtt;
	unsigned short id = getpid();

	printf("PING %s (%s): 12 data bytes\n",
	       hostname, in_ntoa(target));

	sent = 0;
	recv = 0;
	min_rtt = ~0UL;
	max_rtt = 0;
	total_rtt = 0;
	seq = 0;

	while (!stopflag && (count < 0 || sent < count)) {
		unsigned long tstart;
		int ret;

		seq++;
		tstart = get_ms();

		buf[0] = NS_ICMP_ECHO;
		buf[1] = 0;
		{
			struct icmp_echo_request_s *req = (struct icmp_echo_request_s *)(buf + 2);
			req->target_ip = target;
			req->id = htons(id);
			req->seq = htons(seq);
			req->timestamp = htonl(tstart);
		}

		ret = write(s, buf, sizeof(buf));
		if (ret != (int)sizeof(buf)) {
			perror("write");
			break;
		}
		sent++;

		{
			fd_set rfds;
			struct timeval tv;
			unsigned long deadline = get_ms() + REPLY_TIMEOUT;

			while (get_ms() < deadline) {
				FD_ZERO(&rfds);
				FD_SET(s, &rfds);
				tv.tv_sec = 0;
				tv.tv_usec = 100000;

				ret = select(s + 1, &rfds, NULL, NULL, &tv);
				if (ret < 0) break;
				if (ret == 0) continue;

				ret = read(s, &reply, sizeof(reply));
				if (ret != (int)sizeof(reply))
					continue;

				if (reply.status == ICMP_ECHO_REPLY_SUCCESS) {
					unsigned long now = get_ms();
					recv++;
					rtt = now - ntohl(reply.timestamp);
					if (rtt < min_rtt) min_rtt = rtt;
					if (rtt > max_rtt) max_rtt = rtt;
					total_rtt += rtt;
				printf("12 bytes from %s: icmp_seq=%d ttl=%d time=%lu ms\n",
				       in_ntoa(target), seq, reply.ttl, rtt);
				} else {
					printf("Request timeout for icmp_seq %d\n", seq);
				}
				break;
			}
			if (get_ms() >= deadline)
				printf("Request timeout for icmp_seq %d\n", seq);
		}

		if (stopflag)
			break;

		{
			unsigned long elapsed = get_ms() - tstart;
			if (elapsed < (unsigned long)interval) {
				struct timeval slp;
				unsigned long slp_ms = interval - elapsed;
				slp.tv_sec = slp_ms / 1000;
				slp.tv_usec = (slp_ms % 1000) * 1000;
				select(0, NULL, NULL, NULL, &slp);
			}
		}
	}

	close(s);

	printf("--- %s ping statistics ---\n", hostname);
	printf("%d packets transmitted, %d packets received, %d%% packet loss\n",
	       sent, recv,
	       sent ? ((sent - recv) * 100 / sent) : 0);

	if (recv > 0) {
		printf("round-trip min/avg/max = %lu/%lu/%lu ms\n",
		       min_rtt, total_rtt / recv, max_rtt);
	}

	return recv > 0 ? 0 : 1;
}

static void usage(void)
{
	printf("Usage: ping [-c count] [-I local_ip] [-i interval] host\n");
	exit(1);
}

int main(int argc, char **argv)
{
	ipaddr_t local_ip = 0, target_ip;
	int count = -1;
	int interval = DEFAULT_INTERVAL;
	int ch;

	signal(SIGINT, sigint);

	{
		char *hostname = getenv("HOSTNAME");
		if (hostname)
			local_ip = in_gethostbyname(hostname);
	}

	while ((ch = getopt(argc, argv, "c:i:I:")) != -1) {
		switch (ch) {
		case 'c':
			count = atoi(optarg);
			break;
		case 'i':
			interval = atoi(optarg) * 1000;
			break;
		case 'I':
			local_ip = in_gethostbyname(optarg);
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();

	target_ip = in_gethostbyname(argv[optind]);
	if (target_ip == 0) {
		fprintf(stderr, "ping: unknown host %s\n", argv[optind]);
		return 1;
	}
	if (local_ip == 0)
		local_ip = in_gethostbyname("10.0.2.15");

	{
		struct sockaddr_in local, rem;
		int s = socket(AF_INET, SOCK_STREAM, 0);
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
			fprintf(stderr, "ping: ktcp not running\n");
			close(s);
			return 1;
		}
		return ping_via_ktcp(target_ip, argv[optind], count, interval, s);
	}
}
