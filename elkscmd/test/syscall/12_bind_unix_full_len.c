#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "common.h"

int main(void)
{
    struct sockaddr_un sun;
    int s;
    int rc;

    memset(&sun, 'A', sizeof(sun));
    sun.sun_family = AF_UNIX;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0)
        return skip_rc("bind_unix_full_len", "AF_UNIX unavailable");

    errno = 0;
    rc = bind(s, (struct sockaddr *)&sun, sizeof(sun));
    close(s);

    /*
     * Under the minimal hardening patch, this should be rejected because
     * there is no room for a kernel-added terminator. If your fix chooses
     * a different API policy, adjust this expected result accordingly.
     */
    return expect_errno_eq("bind_unix_full_len", rc, EINVAL);
}
