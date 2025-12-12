/*
 * Simple TCP client/server (netcat clone) for ELKS
 *
 * Copyright (C) 2025 Jose Beneyto <sepen@crux-arm.nu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>

/* ELKS resolver */
extern unsigned long in_gethostbyname(const char *name);

/* Globals from env */
static int opt_debug = 0;
static long opt_maxbytes = -1;

/* IPv4 to string */
static const char *ip_to_str(unsigned long ip)
{
    static char buf[16];
    unsigned char *p = (unsigned char *)&ip;
    sprintf(buf, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);
    return buf;
}

static void debug(const char *s)
{
    if (!opt_debug)
        return;
    write(1, "[debug] ", 8);
    write(1, s, strlen(s));
    write(1, "\n", 1);
}

static void msg(const char *s)
{
    write(2, s, strlen(s));
    write(2, "\n", 1);
    exit(1);
}

static void die(const char *s)
{
    write(2, "ERROR: ", 7);
    write(2, s, strlen(s));
    write(2, "\n", 1);
    exit(1);
}

/* Load NETCAT_* env vars */
static void load_env_options(void)
{
    char *p;

    p = getenv("NETCAT_MAXBYTES");
    if (p)
        opt_maxbytes = atol(p);

    p = getenv("NETCAT_DEBUG");
    if (p && atoi(p) == 1)
        opt_debug = 1;
}

/* Main function */
int main(int argc, char **argv)
{
    int listen_mode = 0;
    int port = 0;
    int s, c;
    char *host = NULL;
    struct sockaddr_in addr;
    struct sockaddr_in local;
    fd_set fds;
    long total_bytes = 0;

    load_env_options();
    debug("netcat starting...");

    if (argc < 2)
        msg("usage: netcat <host> <port> | -l <port>");

    if (!strcmp(argv[1], "-l")) {
        if (argc != 3)
            msg("usage: netcat -l <port>");
        listen_mode = 1;
        port = atoi(argv[2]);
        debug("listen mode enabled");
    } else {
        if (argc != 3)
            msg("usage: netcat <host> <port>");
        host = argv[1];
        port = atoi(argv[2]);
        debug("client mode enabled");
    }

    debug("creating socket...");
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0)
        die("socket failed");

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (listen_mode) {

        addr.sin_addr.s_addr = INADDR_ANY;
        debug("binding...");
        if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            die("bind failed");

        debug("listening...");
        if (listen(s, 1) < 0)
            die("listen failed");

        debug("waiting for connection...");
        c = accept(s, NULL, NULL);
        if (c < 0)
            die("accept failed");

        debug("client connected");
        close(s);
        
        /* replace listener with accepted socket */
        s = c;

    } else {
        debug("resolving hostname...");
        unsigned long ip = in_gethostbyname(host);
        if (!ip)
            die("DNS failed or host unknown");

        debug("resolved IP:");
        debug(ip_to_str(ip));

        addr.sin_addr.s_addr = ip;

        debug("binding ephemeral local port...");
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = 0;
        local.sin_addr.s_addr = INADDR_ANY;

        if (bind(s, (struct sockaddr *)&local, sizeof(local)) < 0)
            die("bind-local failed");

        debug("connecting...");
        if (connect(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
            die("connect failed");

        debug("connected!");
    }

    debug("entering I/O loop...");
    for (;;) {
        FD_ZERO(&fds);
        FD_SET(0, &fds);
        FD_SET(s, &fds);

        int rc = select(s + 1, &fds, NULL, NULL, NULL);
        if (rc < 0)
            die("select failed");

        /* stdin to socket */
        if (FD_ISSET(0, &fds)) {
            char b[256];
            int n = read(0, b, sizeof(b));
            if (n <= 0) {
                debug("stdin closed");
                break;
            }
            write(s, b, n);
            total_bytes += n;

            if (opt_maxbytes >= 0 && total_bytes >= opt_maxbytes) {
                debug("maxbytes reached");
                break;
            }
        }

        /* socket to stdout */
        if (FD_ISSET(s, &fds)) {
            char b[256];
            int n = read(s, b, sizeof(b));
            if (n <= 0) {
                debug("remote closed");
                break;
            }
            write(1, b, n);
            total_bytes += n;

            if (opt_maxbytes >= 0 && total_bytes >= opt_maxbytes) {
                debug("maxbytes reached");
                break;
            }
        }
    }

    debug("closing socket");
    close(s);
    debug("done");
    return 0;
}
