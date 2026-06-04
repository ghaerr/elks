/*
 * ifconfig - configure network interface
 *
 * Uses ktcp's netconf protocol to query and set IP address, netmask, and gateway.
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
#include <sys/socket.h>
#include <arpa/inet.h>

#define NETCONF_PORT	2
#define NS_GET_CONFIG	6
#define NS_SET_IP	7
#define NS_SET_NETMASK	8
#define NS_SET_GATEWAY	9

struct config_info_s {
	unsigned long	local_ip;
	unsigned long	netmask_ip;
	unsigned long	gateway_ip;
	unsigned char	hwaddr[6];
};

static int netconf_connect(void)
{
	struct sockaddr_in local, rem;
	int s;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) return -1;

	local.sin_family = AF_INET;
	local.sin_port = 0;
	local.sin_addr.s_addr = 0;
	if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0)
		goto err;

	rem.sin_family = AF_INET;
	rem.sin_addr.s_addr = 0;
	rem.sin_port = htons(NETCONF_PORT);
	if (connect(s, (struct sockaddr *)&rem, sizeof(rem)) < 0)
		goto err;

	return s;
err:
	close(s);
	return -1;
}

static int netconf_cmd(int s, unsigned char type, unsigned long value)
{
	unsigned char buf[2 + sizeof(unsigned long)];
	unsigned char ack;

	buf[0] = type;
	buf[1] = 0;
	if (value != (unsigned long)-1) {
		unsigned long *vp = (unsigned long *)(buf + 2);
		*vp = value;
	}

	if (write(s, buf, value != (unsigned long)-1 ? sizeof(buf) : 2) < 0)
		return -1;

	/* read ack byte */
	if (read(s, &ack, 1) < 1)
		return -1;

	return 0;
}

static int show_config(int s)
{
	struct config_info_s config;
	unsigned char req[2];
	unsigned char *hw = config.hwaddr;
	int ret;

	req[0] = NS_GET_CONFIG;
	req[1] = 0;

	if (write(s, req, sizeof(req)) < 0) {
		perror("write");
		return -1;
	}

	ret = read(s, &config, sizeof(config));
	if (ret != (int)sizeof(config)) {
		if (ret < 0) perror("read");
		else fprintf(stderr, "short read %d\n", ret);
		return -1;
	}

	printf("ne0       HWaddr %02x:%02x:%02x:%02x:%02x:%02x\n",
	       hw[0], hw[1], hw[2], hw[3], hw[4], hw[5]);
	printf("          inet addr:%s", in_ntoa(config.local_ip));
	printf("  Mask:%s", in_ntoa(config.netmask_ip));
	if (config.gateway_ip)
		printf("  Gateway:%s", in_ntoa(config.gateway_ip));
	printf("\n");
	return 0;
}

static void usage(void)
{
	fprintf(stderr, "Usage: ifconfig [<ip>] [netmask <mask>] [gateway <gw>]\n");
	exit(1);
}

int main(int argc, char **argv)
{
	int s, i;
	unsigned long set_ip = (unsigned long)-1;
	unsigned long set_mask = (unsigned long)-1;
	unsigned long set_gw = (unsigned long)-1;

	s = netconf_connect();
	if (s < 0) {
		fprintf(stderr, "ifconfig: ktcp not running\n");
		return 1;
	}

	if (argc == 1) {
		/* Show current configuration */
		int ret = show_config(s);
		close(s);
		return ret != 0;
	}

	/* Parse arguments */
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "netmask") == 0) {
			if (++i >= argc) usage();
			set_mask = in_gethostbyname(argv[i]);
		} else if (strcmp(argv[i], "gateway") == 0) {
			if (++i >= argc) usage();
			set_gw = in_gethostbyname(argv[i]);
		} else if (argv[i][0] == '-') {
			usage();
		} else {
			set_ip = in_gethostbyname(argv[i]);
		}
	}

	if (set_ip != (unsigned long)-1) {
		if (netconf_cmd(s, NS_SET_IP, set_ip) < 0) {
			perror("set IP");
			close(s);
			return 1;
		}
	}

	if (set_mask != (unsigned long)-1) {
		if (netconf_cmd(s, NS_SET_NETMASK, set_mask) < 0) {
			perror("set netmask");
			close(s);
			return 1;
		}
	}

	if (set_gw != (unsigned long)-1) {
		if (netconf_cmd(s, NS_SET_GATEWAY, set_gw) < 0) {
			perror("set gateway");
			close(s);
			return 1;
		}
	}

	close(s);
	return 0;
}
