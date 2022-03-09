/*
 * arp - display arp cache
 *
 * 18 Jul 20 Greg Haerr
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ktcp/tcp.h>
#include <ktcp/netconf.h>
#include <ktcp/deveth.h>
#include <ktcp/arp.h>
#include <arpa/inet.h>

struct arp_cache arp_cache[ARP_CACHE_MAX];

char *mac_ntoa(eth_addr_t eth_addr)
{
    unsigned char *p = (unsigned char *)eth_addr;
    static char b[18];

    sprintf(b, "%02x.%02x.%02x.%02x.%02x.%02x",p[0],p[1],p[2],p[3],p[4],p[5]);
    return b;
}

int main(int ac, char **av)
{
    int i, s, ret;
    struct stat_request_s sr;
    struct sockaddr_in localadr, remaddr;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	perror("arp");
	return 1;
    }

    localadr.sin_family = AF_INET;
    localadr.sin_port = PORT_ANY;
    localadr.sin_addr.s_addr = INADDR_ANY;  
    if (bind(s, (struct sockaddr *)&localadr, sizeof(struct sockaddr_in)) < 0) {
	perror("bind");
	return 1;
    }

    remaddr.sin_family = AF_INET;
    remaddr.sin_port = htons(NETCONF_PORT);
    remaddr.sin_addr.s_addr = 0;
    if (connect(s, (struct sockaddr *)&remaddr, sizeof(struct sockaddr_in)) < 0) {
	perror("connect");
	return 1;
    }

    sr.type = NS_ARP;
    write(s, &sr, sizeof(sr));	
    ret = read(s, arp_cache, ARP_CACHE_MAX*sizeof(struct arp_cache));
    if (ret != ARP_CACHE_MAX*sizeof(struct arp_cache)) {
	perror("read");
	return 1;
    }

    for(i=0; i<ARP_CACHE_MAX; i++) {
	if (arp_cache[i].ip_addr)
		printf("%-15s %s\n", in_ntoa(arp_cache[i].ip_addr),
			mac_ntoa(arp_cache[i].eth_addr));
    }
    return 1;
}
