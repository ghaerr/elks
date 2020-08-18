#include <netinet/in.h>

ipaddr_t in_aton(const char *str);
ipaddr_t in_gethostbyname(char *str);
char *in_ntoa(ipaddr_t in);
