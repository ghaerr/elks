#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "common.h"

int main(void)
{
    struct {
        unsigned char b[2];
    } tiny;
    int s;
    int rc;

    tiny.b[0] = AF_INET;
    tiny.b[1] = 0;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        return skip_rc("connect_inet_short_len", "AF_INET unavailable");

    errno = 0;
    rc = connect(s, (struct sockaddr *)&tiny, 1);
    close(s);

    return expect_errno_eq("connect_inet_short_len", rc, EINVAL);
}
