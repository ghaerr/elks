#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
/*
 * Aug 2020 Greg Haerr
 *
 * Inspired by very old BSD code, Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 */

/* return ip address in network byte order of host by reading /etc/hosts file*/
ipaddr_t in_gethostbyname(char *str)
{
	char *p, *cp;
	FILE *fp;
	ipaddr_t addr = 0;
	char *name;
	char buf[80];

	/* very basic check for ip address*/
	if (*str >= '0' && *str <= '9')
		return in_aton(str);

	/* handle localhost without opening /etc/hosts*/
	if (!strcmp(str, "localhost"))
		return htonl(INADDR_LOOPBACK);

	if ((fp = fopen(_PATH_HOSTS, "r")) == NULL)
		goto try_resolver;

	while ((p = fgets(buf, sizeof(buf), fp)) != NULL) {
		if (*p == '#')
			continue;
		cp = strpbrk(p, "#\n");
		if (cp == NULL)
			continue;
		*cp = '\0';
		cp = strpbrk(p, " \t");
		if (cp == NULL)
			continue;
		*cp++ = '\0';

		addr = in_aton(p);
		while (*cp == ' ' || *cp == '\t')
			cp++;
		name = cp;
		cp = strpbrk(cp, " \t");
		if (cp != NULL)
			*cp++ = '\0';
		//printf("name %s addr %s\n", name, in_ntoa(addr));
		if (!strcmp(name, str))
			goto found;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			name = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
			if (!strcmp(name, str))
				goto found;
		}
	}

	/* read all of /etc/hosts, no match found*/
try_resolver:
	addr = in_resolv(str, NULL);

found:
	//if (addr) printf("found %s\n", name);
	fclose(fp);
	return addr;
}
