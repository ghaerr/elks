/*
 * DNS Name Lookup
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#define msg(str) write(STDOUT_FILENO, str, sizeof(str) - 1)
#define msgstr(str) write(STDOUT_FILENO, str, strlen(str))
#define errmsg(str) write(STDERR_FILENO, str, sizeof(str) - 1)
#define errstr(str) write(STDERR_FILENO, str, strlen(str))

int main(int ac, char **av)
{
	ipaddr_t result;
	char *server = NULL;

	if (ac < 2) {
		errmsg("Usage: nslookup <domain> [nameserver]\n");
		return 1;
	}
	if (ac > 2)
		server = av[2];

	result = in_resolv(av[1], server);
	if (result) {
		msgstr(av[1]);
		msg(" is ");
		msgstr(in_ntoa(result));
	} else perror(av[1]);
	return (result == 0);
}
