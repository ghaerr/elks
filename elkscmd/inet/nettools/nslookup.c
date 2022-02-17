/*
 * DNS Name Lookup
 */
#include <stdio.h>
#include <arpa/inet.h>

int main(int ac, char **av)
{
	ipaddr_t result;
	char *server = NULL;

	if (ac < 2) {
		printf("Usage: nslookup <domain> [nameserver]\n");
		return 1;
	}
	if (ac > 2)
		server = av[2];

	result = in_resolv(av[1], server);
	if (result)
		printf("%s is %s\n", av[1], in_ntoa(result));
	else perror(av[1]);
	return (result == 0);
}
