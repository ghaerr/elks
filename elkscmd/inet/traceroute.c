/*
 * traceroute - print route to destination host via ktcp netconf protocol
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
#include <sys/time.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define NETCONF_PORT	2
#define NS_ICMP_TRACEROUTE	10
#define MAX_HOPS		30
#define PROBE_TIMEOUT		5000
#define ICMP_TRACEROUTE_ECHO_REPLY	0
#define ICMP_TRACEROUTE_TIME_EXCEED	1

struct icmp_traceroute_request_s {
	unsigned long   target_ip;
	unsigned char   ttl;
	unsigned short  id;
	unsigned short  seq;
	unsigned long   timestamp;
};

struct icmp_traceroute_reply_s {
	unsigned long   timestamp;
	unsigned long   resp_ip;
	unsigned char   ttl;
	unsigned char   status;
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

static int traceroute_via_ktcp(ipaddr_t target, char *hostname, int maxhops, int s)
{
	unsigned char buf[2 + sizeof(struct icmp_traceroute_request_s)];
	struct icmp_traceroute_reply_s reply;
	int ttl;
	unsigned short id = getpid();
	unsigned long total_rtt = 0;
	int hops = 0;

	printf("traceroute to %s (%s), %d hops max, 12 byte packets\n",
	       hostname, in_ntoa(target), maxhops);

	for (ttl = 1; ttl <= maxhops && !stopflag; ttl++) {
		unsigned long tstart;
		int ret, timedout = 0;

		tstart = get_ms();

		buf[0] = NS_ICMP_TRACEROUTE;
		buf[1] = 0;
		{
			struct icmp_traceroute_request_s *req =
				(struct icmp_traceroute_request_s *)(buf + 2);
			req->target_ip = target;
			req->ttl = ttl;
			req->id = htons(id);
			req->seq = htons(ttl);
			req->timestamp = htonl(tstart);
		}

		ret = write(s, buf, sizeof(buf));
		if (ret != (int)sizeof(buf)) {
			perror("write");
			break;
		}

		{
			fd_set rfds;
			struct timeval tv;
			unsigned long deadline = get_ms() + PROBE_TIMEOUT;

			while (get_ms() < deadline && !stopflag) {
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

				break;
			}
			if (get_ms() >= deadline)
				timedout = 1;
		}

		printf("%2d  ", ttl);

		if (timedout) {
			printf("* * *\n");
		} else if (reply.status == ICMP_TRACEROUTE_TIME_EXCEED) {
			unsigned long now = get_ms();
			unsigned long rtt = now - tstart;
			total_rtt += rtt;
			printf("%s  %lu ms\n", in_ntoa(reply.resp_ip), rtt);
		} else if (reply.status == ICMP_TRACEROUTE_ECHO_REPLY) {
			unsigned long now = get_ms();
			unsigned long rtt = now - tstart;
			total_rtt += rtt;
			printf("%s  %lu ms\n", in_ntoa(reply.resp_ip), rtt);
			hops = ttl;
			break;
		}

		hops = ttl;
	}

	close(s);

	printf("\n--- %s traceroute statistics ---\n", hostname);
	printf("%d hops, avg rtt %lu ms\n", hops,
	       hops > 0 ? total_rtt / hops : 0UL);

	return 0;
}

static void usage(void)
{
	printf("Usage: traceroute [-m max_ttl] host\n");
	exit(1);
}

int main(int argc, char **argv)
{
	ipaddr_t target_ip;
	int maxhops = MAX_HOPS;
	int ch;

	signal(SIGINT, sigint);

	while ((ch = getopt(argc, argv, "m:")) != -1) {
		switch (ch) {
		case 'm':
			maxhops = atoi(optarg);
			if (maxhops < 1 || maxhops > 255) {
				fprintf(stderr, "traceroute: max hops must be 1-255\n");
				return 1;
			}
			break;
		default:
			usage();
		}
	}
	if (optind >= argc)
		usage();

	{
		char *host = argv[optind];
		int ancount = 0;
		/* check for IP literal */
		if (*host >= '0' && *host <= '9')
			target_ip = in_aton(host);
		else {
			target_ip = in_resolv(host, NULL, &ancount);
			if (target_ip == 0) {
				fprintf(stderr, "traceroute: unknown host %s\n", host);
				return 1;
			}
			if (ancount > 1)
				printf("warning: %s has multiple addresses, using %s\n",
				       host, in_ntoa(target_ip));
		}
	}
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
			fprintf(stderr, "traceroute: ktcp not running\n");
			close(s);
			return 1;
		}
		return traceroute_via_ktcp(target_ip, argv[optind], maxhops, s);
	}
}
