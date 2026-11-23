/*
 * ept - ELKS Package Tool
 *
 * Copyright (c) 2026 Sam Warr
 * SPDX-License-Identifier: MIT
 *
 * Installs and updates programs from a plain HTTP package repository.
 * One source, two builds: the full ept, and with -DTINY tiny ept for floppy
 * systems, which leaves out everything behind #ifndef TINY.
 *
 *   ept list [-n]                 what the repositories have, and what is installed
 *   ept search [-n] [<text>]      packages by name or description     (full only)
 *   ept install [-n] [-r] <pkg> ...   install or update packages      (-r full only)
 *   ept update                    update everything installed         (full only)
 *   ept remove <pkg> ...          remove packages
 *   -n   the nightly builds instead of the tagged releases
 *   -r   fetch every file of the packages again, changed or not
 *
 * install first shows the space and files it needs against what is free on
 * the root filesystem, and asks. Scripts answer it: echo | ept install <pkg>
 * A download gets TRIES tries, each given up after STALL seconds without data;
 * the full ept resumes it where it stopped.
 *
 * The full ept reads its repositories from the REPO= lines of /etc/ept.cfg.
 * A package comes from the last one listed that has it, except ept itself:
 * only the first can update ept or install a file at its path. A "#min
 * <version>" line in its index names the oldest ept it suits; an older ept
 * installs the ept package first and stops.
 *
 * Tiny ept has the repository built in and looks it up with DNS alone (no
 * /etc/ept.cfg, no /etc/hosts, no stdio). "ept install ept" replaces it with
 * the full ept, which carries on with the same files.
 *
 * All it relies on is the repository format, which every ept shares:
 *   <repo>/index         first line "#ept1", last line "#end", other "#" lines
 *                        ignored; one line per file: name version md5 size mode dest
 *   <repo>/pool/<name><dest>   each file
 *   <repo>/nightly/...   the same again for the nightly builds
 * and the installed list /etc/ept/inst: name version md5 dest channel (s or n)
 */
#include <stdint.h>
#include <stdio.h>              /* rename() only; no stdio is used */
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef REPOHOST                /* overridable for testing */
#define REPOHOST    "ept-repo.com"
#define REPOPORT    80
#endif
#ifndef ROOT
#define ROOT        ""          /* prefix for every local path, for testing */
#endif
#define STALL       10          /* seconds without data before a try is given up */
#define TRIES       3           /* tries per download */
#define DIR         ROOT "/etc/ept"
#define INDEX       DIR "/index"
#define NINDEX      DIR "/nindex"
#define DB          DIR "/inst"
#define DBNEW       DIR "/inst.new"
#ifndef TINY
#define VERSION     "2.0"
#define VMAJOR      2           /* VERSION in numbers, for "#min" */
#define VMINOR      0
#define CONFIG      ROOT "/etc/ept.cfg"
#define INDEXNEW    DBNEW       /* the temporary files are never needed at once: */
#define DESC        DBNEW       /* a new list, a new installed list, descriptions */
#define SELF        "/bin/ept"
#define MAXREPO     8
#define URLSZ       160
#ifndef DEFREPO                 /* overridable for testing */
#define DEFREPO     "http://ept-repo.com"
#endif
#endif

#ifdef OLD_RESOLV               /* ELKS libc before in_resolv() took ancount */
#define resolve(h)  in_resolv(h, NULL)
#else
#define resolve(h)  in_resolv(h, NULL, NULL)
#endif

static char buf[1024];          /* network and copying */
#ifdef TINY
static char fbuf[512];          /* reading the lists */
static char line[256];          /* the line being handed out; longer ones are cut */
static int llen;
static void (*online)(char *);
#endif
static char path[192], tmp[192];
static char *pkg;               /* the package being installed or removed */
static int found, bad, drop, outfd;
static const char *why;         /* why the last try failed */
static int nfiles;              /* files of the packages to install */
static char newf[512];          /* files of the new version, NUL separated */
static unsigned nnew;           /* their length; more than fits: delete nothing */
static char rpath[200];         /* the file being downloaded */
#ifdef TINY
static ipaddr_t ip;             /* the repository's address, once looked up */
#endif
static char *chan = "";         /* -n: "/nightly" */
static char *idx = INDEX;       /* the list of the channel */
static char cname[] = " s\n";  /* the channel in the installed list */
static char *f[6];              /* fields of the current line */

#ifdef TINY                     /* the one repository */
#define RHOST       REPOHOST
#define RPORT       REPOPORT
#define RIP         ip
#else                           /* the repositories of the config */
static char repos[MAXREPO][URLSZ];  /* its REPO= lines; the first is the primary */
static int nrepo, repo;         /* how many, and the one in use */
static unsigned live[2];        /* those with a usable package list, per channel */
#define LIVE        live[*chan != 0]
static ipaddr_t ips[MAXREPO];   /* their addresses, each looked up once */
static char hosts[MAXREPO][64]; /* their host, port and path prefix */
static unsigned ports[MAXREPO];
static char *bases[MAXREPO];
static char idxname[sizeof(NINDEX) + 4]; /* its package list file */
static char whyb[80];
static int notfound, quiet;     /* the last reply was a 404; a 404 is no error */
static int listing;             /* ept list: an old package list beats none */
static int fresh;               /* -r: fetch every file again */
static unsigned long from;      /* bytes of the file already fetched: where to resume */
static int anyn, got;           /* a package came from the nightly builds; files fetched */
static char fver[256];          /* the version findrepo() found, or being installed */
static char *text;              /* ept search: what to look for */
static int matches;             /* packages search found */
static int newer;               /* the list just fetched needs a newer ept */
#define RHOST       hosts[repo]
#define RPORT       ports[repo]
#define RBASE       bases[repo]
#define RIP         ips[repo]
#endif

/* ---- MD5 (RFC 1321), one step per pass; x86 is little-endian, so the
 * block and the digest are used in place */
static uint32_t st[4];
static union { unsigned char c[64]; uint32_t w[16]; } mb;
static unsigned long mlen;

static const uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391,
};
static const unsigned char S[16] = { 7, 12, 17, 22, 5, 9, 14, 20, 4, 11, 16, 23, 6, 10, 15, 21 };

static void md5_block(void)
{
    uint32_t a = st[0], b = st[1], c = st[2], d = st[3], x;
    int i, g, s;

    for (i = 0; i < 64; i++) {
        switch (i >> 4) {
        case 0:  x = (b & c) | (~b & d); g = i;                break;
        case 1:  x = (d & b) | (~d & c); g = (5 * i + 1) & 15; break;
        case 2:  x = b ^ c ^ d;          g = (3 * i + 5) & 15; break;
        default: x = c ^ (b | ~d);       g = (7 * i) & 15;     break;
        }
        x += a + K[i] + mb.w[g];
        s = S[(i >> 4) << 2 | (i & 3)];
        a = d;
        d = c;
        c = b;
        b += x << s | x >> (32 - s);
    }
    st[0] += a;
    st[1] += b;
    st[2] += c;
    st[3] += d;
}

static void md5_init(void)
{
    st[0] = 0x67452301;
    st[1] = 0xefcdab89;
    st[2] = 0x98badcfe;
    st[3] = 0x10325476;
    mlen = 0;
}

static void md5_add(const char *p, int n)
{
    while (n--) {
        mb.c[(unsigned)mlen & 63] = *p++;
        if (!((unsigned)++mlen & 63))
            md5_block();
    }
}

/* finish, then compare with a hex digest: 0 if they match */
static int md5_check(const char *sum)
{
    static const char hex[] = "0123456789abcdef";
    unsigned long n = mlen;
    unsigned char *d = (unsigned char *)st;
    int i;

    md5_add("\200", 1);
    while (((unsigned)mlen & 63) != 56)
        md5_add("", 1);
    mb.w[14] = n << 3;
    mb.w[15] = n >> 29;
    md5_block();
    for (i = 0; i < 16; i++)
        if (sum[2 * i] != hex[d[i] >> 4] || sum[2 * i + 1] != hex[d[i] & 15])
            return 1;
    return 0;
}

/* ---- output, without stdio */
static void out(const char *s)
{
    write(STDOUT_FILENO, s, strlen(s));
}

static void say(const char *a, const char *b)
{
    write(STDERR_FILENO, "ept: ", 5);
    write(STDERR_FILENO, a, strlen(a));
    write(STDERR_FILENO, b, strlen(b));
    write(STDERR_FILENO, "\n", 1);
}

/* n in decimal; 16 bits is enough for K and file counts */
static char *num(unsigned n)
{
    static char t[6];
    char *p = t + 5;

    do
        *--p = '0' + n % 10;
    while (n /= 10);
    return p;
}

static unsigned long atoun(const char *s, int base)
{
    unsigned long n = 0;

    while (*s >= '0' && *s < '0' + base)
        n = n * base + (*s++ - '0');
    return n;
}

/* ---- reading lists line by line */
#ifdef TINY
static void feed(const char *p, int n)
{
    char c;

    while (n--) {
        if ((c = *p++) == '\n') {
            if (llen && line[llen - 1] == '\r')
                llen--;
            line[llen] = '\0';
            online(line);
            llen = 0;
        } else if (llen < sizeof(line) - 1)
            line[llen++] = c;
    }
}

/* call fn for each line of a file; -1 if it cannot be read */
static int eachline(const char *file, void (*fn)(char *))
{
    int fd, n;

    if ((fd = open(file, O_RDONLY)) < 0)
        return -1;
    online = fn;
    llen = 0;
    while ((n = read(fd, fbuf, sizeof(fbuf))) > 0)
        feed(fbuf, n);
    close(fd);
    if (llen)
        feed("\n", 1);
    return n < 0 ? -1 : 0;
}
#else
/* call fn for each line of a file; -1 if it cannot be read. The buffers are
 * its own, so fn can read another file the same way */
static int eachline(const char *file, void (*fn)(char *))
{
    char fb[512], ln[256];
    int fd, n, i, len = 0;

    if ((fd = open(file, O_RDONLY)) < 0)
        return -1;
    while ((n = read(fd, fb, sizeof(fb))) > 0)
        for (i = 0; i < n; i++) {
            if (fb[i] != '\n') {
                if (len < sizeof(ln) - 1)
                    ln[len++] = fb[i];      /* longer lines are cut */
                continue;
            }
            if (len && ln[len - 1] == '\r')
                len--;
            ln[len] = '\0';
            fn(ln);
            len = 0;
        }
    close(fd);
    if (len) {
        if (ln[len - 1] == '\r')
            len--;
        ln[len] = '\0';
        fn(ln);
    }
    return n < 0 ? -1 : 0;
}
#endif

/* split s at blanks into f[]; the number of fields */
static int split(char *s)
{
    int n = 0;

    while (n < 6) {
        while (*s == ' ' || *s == '\t')
            s++;
        if (!*s)
            break;
        f[n++] = s;
        while (*s && *s != ' ' && *s != '\t')
            s++;
        if (*s)
            *s++ = '\0';
    }
    return n;
}

/* ---- HTTP */

/* SIGALRM only has to interrupt a read() that waits too long. ELKS resets the
 * handler after each signal, so get() sets it again before every read() */
static void onalarm(int sig)
{
}

/* GET <path> into fd. 0 on a complete 200 reply of size bytes (any size if
 * size < 0) whose md5 matches sum (if sum) */
static int get(const char *rpath, int fd, long size, const char *sum)
{
    int s, n, i, hdr = 1, nl = 0, slen = 0, rc = -1;
    unsigned long total = 0, mark = size;
    char status[12];
    struct sockaddr_in a;

#ifndef TINY
    total = from;
#endif

#ifdef TINY
    why = "cannot reach " REPOHOST;
#else
    strcpy(whyb, "cannot reach ");
    strcat(whyb, RHOST);
    why = whyb;
    notfound = 0;
#endif
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    a.sin_family = AF_INET;
    a.sin_port = PORT_ANY;
    a.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (struct sockaddr *)&a, sizeof(a)) < 0)
        goto done;
    a.sin_port = htons(RPORT);
    /* look the host up once: ELKS asks DNS over TCP, and ktcp has room for
     * only a few connections at a time, closed ones included for a while */
    if (!RIP)
#ifdef TINY
        RIP = resolve(RHOST);
#else
        RIP = *RHOST >= '0' && *RHOST <= '9' ? in_aton(RHOST) : resolve(RHOST);
#endif
    if (!(a.sin_addr.s_addr = RIP)
            || in_connect(s, (struct sockaddr *)&a, sizeof(a), 10) < 0)
        goto done;
    why = "connection lost";
    strcpy(buf, "GET ");
#ifdef TINY
    strcat(buf, rpath);
    strcat(buf, " HTTP/1.0\r\nHost: " REPOHOST "\r\nUser-Agent: ept-tiny\r\nConnection: close\r\n\r\n");
#else
    strcat(buf, RBASE);
    strcat(buf, rpath);
    strcat(buf, " HTTP/1.0\r\nHost: ");
    strncat(buf, repos[repo] + 7, RBASE - (repos[repo] + 7));    /* host[:port] as written */
    if (from) {
        strcat(buf, "\r\nRange: bytes=");
        strcat(buf, ultostr(from, 10));
        strcat(buf, "-");
    }
    strcat(buf, "\r\nUser-Agent: ept/" VERSION "\r\n\r\n");     /* HTTP/1.0: closed after */
#endif
    n = strlen(buf);
    if (write(s, buf, n) != n)
        goto done;
#ifndef TINY
    if (!from)                          /* resuming: the md5 so far still holds */
#endif
    md5_init();
    if (sum)
        out("  [................]\r  [");
    while (signal(SIGALRM, onalarm), alarm(STALL), (n = read(s, buf, sizeof(buf))) > 0) {
        for (i = 0; i < n && hdr; i++) {
            if (slen < sizeof(status))
                status[slen++] = buf[i];
            if (buf[i] == '\n') {
                if (nl)
                    hdr = 0;            /* blank line ends the header */
                nl = 1;
            } else if (buf[i] != '\r')
                nl = 0;
        }
        if (hdr)
            continue;
#ifndef TINY
        if (from && !strncmp(status + 9, "20", 2)) {    /* the reply to a resume */
            if (status[11] == '6')
                status[11] = '0';       /* the rest, as asked */
            else {                      /* the whole file after all: start over */
                lseek(fd, 0, SEEK_SET);
                md5_init();
                total = 0;
                mark = size;
                out("\r  [");
            }
            from = 0;
        }
#endif
        if (slen < sizeof(status) || strncmp(status, "HTTP/1.", 7) || strncmp(status + 9, "200", 3)) {
            why = "server error";
#ifndef TINY
            if (!strncmp(status + 9, "404", 3)) {
                why = "not found";
                notfound = 1;
            }
#endif
            goto done;
        }
        n -= i;
        if (write(fd, buf + i, n) != n) {
            why = "write error";
            goto done;
        }
        md5_add(buf + i, n);
        total += n;
        while (sum && (total << 4) >= mark) {
            out("#");                   /* a sixteenth of the file */
            mark += size;
        }
    }
    if (n >= 0 && !hdr && (size < 0 || total == size)) {
        why = "checksum mismatch";
        if (!sum || !md5_check(sum))
            rc = 0;
    }
done:
    alarm(0);
    close(s);
#ifndef TINY
    from = sum && total < size ? total : 0;     /* complete but bad: start over */
#endif
    return rc;
}

/* rename, or copy where that fails (FAT); the old file must already be gone */
static int put(const char *from, const char *to)
{
    int in, o, n;

    if (rename(from, to) == 0)
        return 0;
    if ((in = open(from, O_RDONLY)) < 0)
        return -1;
    if ((o = open(to, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        close(in);
        return -1;
    }
    while ((n = read(in, buf, sizeof(buf))) > 0 && write(o, buf, n) == n)
        ;
    close(in);
    if (close(o) < 0 || n != 0)
        return -1;
    unlink(from);
    return 0;
}

/* replace file 'to' by 'from'; a running program can be unlinked but not overwritten */
static int replace(const char *from, const char *to)
{
    unlink(to);
    if (put(from, to) != 0) {
        say("cannot write ", to);
        unlink(from);
        return -1;
    }
    return 0;
}

/* download <path> to file, up to TRIES times; only the last failure says why */
static int fetch(const char *rpath, const char *file, long size, const char *sum)
{
    int fd, rc, try;
#ifndef TINY
    unsigned long before;

    from = 0;
#endif

    for (try = 1; ; try++) {
#ifdef TINY
        if ((fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
#else
        before = from;
        if ((fd = open(file, from ? O_WRONLY : O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0
                || lseek(fd, from, SEEK_SET) < 0) {
#endif
            say("cannot write ", file);
            return -1;
        }
        rc = get(rpath, fd, size, sum);
        if (close(fd) < 0) {
            rc = -1;
            why = "write error";
        }
        if (rc == 0)
            return 0;
#ifndef TINY
        if (notfound)                   /* trying again will not change that */
            break;
        if (from > before)
            try = 0;                    /* it got further: the tries start over */
#endif
        if (try == TRIES)
            break;
#ifndef TINY
        {   /* give ktcp a moment: closed connections hold its memory for a while */
            struct timeval tv;
            tv.tv_sec = 5;
            tv.tv_usec = 0;
            select(0, NULL, NULL, NULL, &tv);
        }
#endif
        say("retrying", "");
    }
#ifndef TINY
    if (!(notfound && quiet))
#endif
    say(why, "");
    unlink(file);
    return -1;
}

/* ---- the package list */
static int lines, known, last;

/* a package line of the list, in f[]? */
static int pkgline(char *s)
{
    return s[0] != '#' && split(s) == 6;
}

#ifndef TINY
/* is version v (major.minor, the rest ignored) newer than this ept? */
static int newerthan(const char *v)
{
    unsigned n = atoun(v, 10);

    if (n != VMAJOR)
        return n > VMAJOR;
    v = strchr(v, '.');
    return v && atoun(v + 1, 10) > VMINOR;
}
#endif

static void checkline(char *s)
{
    if (lines++ == 0)
        known = !strcmp(s, "#ept1");    /* a list this ept understands */
    last = !strcmp(s, "#end");
#ifndef TINY
    if (!strncmp(s, "#min ", 5) && newerthan(s + 5))
        newer = 1;                      /* "#min <version>": the oldest ept it suits */
#endif
}

#ifdef TINY
static int update(void)
{
    mkdir(DIR, 0755);
    strcpy(rpath, chan);
    strcat(rpath, "/index");
    if (fetch(rpath, idx, -1, NULL) != 0)
        return -1;
    lines = known = last = 0;
    eachline(idx, checkline);
    if (!known || !last) {              /* a newer format, or cut short */
        say("unusable package list", "");
        unlink(idx);
        return -1;
    }
    return 0;
}
#else
/* ---- repositories */

/* use repository i, and its package list for the channel (index, index.1,
 * ... or nindex, nindex.1, ...) */
static void setrepo(int i)
{
    repo = i;
    strcpy(idxname, *chan ? NINDEX : INDEX);
    if (i) {
        strcat(idxname, ".");
        strcat(idxname, num(i));
    }
    idx = idxname;
}

/* a REPO=http://host[:port][/path] line of the config */
static void cfgline(char *s)
{
    char *p, *u;
    unsigned n, port = 80;

    if (strncmp(s, "REPO=", 5))
        return;
    s += 5;
    if (strncmp(s, "http://", 7) || strlen(s) >= URLSZ || nrepo == MAXREPO)
        goto bad;
    for (p = u = s + 7; *p != ':' && *p != '/' && *p; p++)
        ;
    n = p - u;
    if (*p == ':') {
        if (!(port = atoun(p + 1, 10)))
            goto bad;                   /* no port, or 0 */
        while (*++p >= '0' && *p <= '9')
            ;
    }
    if (!n || n >= sizeof(hosts[0]) || (*p && *p != '/'))
        goto bad;
    for (u = p + strlen(p); u > p && u[-1] == '/'; )
        *--u = '\0';                   /* no trailing slash */
    memcpy(hosts[nrepo], s + 7, n);
    ports[nrepo] = port;
    bases[nrepo] = strcpy(repos[nrepo], s) + (p - s);
    nrepo++;
    return;
bad:
    say("bad repository in " CONFIG ": ", s);
    exit(1);
}

/* the repositories of the config; without a config, write the full ept's */
static void readcfg(void)
{
    static const char cfg[] =
        "# ept repositories, one REPO= line each.\n"
        "# WARNING: every repository can replace any file on this system.\n"
        "# Only add ones you trust. Only the FIRST can update ept; add others\n"
        "# below it: for other packages, a later line overrides an earlier one.\n"
        "\n"
        "REPO=" DEFREPO "\n";
    int fd;

    if (eachline(CONFIG, cfgline) < 0
            && (fd = open(CONFIG, O_WRONLY | O_CREAT | O_EXCL, 0644)) >= 0) {
        write(fd, cfg, sizeof(cfg) - 1);
        close(fd);
    }
    if (!nrepo)
        cfgline((char *)"REPO=" DEFREPO);
}

static int has;
static char prev[256], avail[256]; /* fields of a line fit */
static int install(char *name);

static void hasline(char *s);

/* a package of the list just fetched that the saved one does not have */
static void newline_(char *s)
{
    if (!pkgline(s) || !strcmp(f[0], prev))
        return;
    strcpy(prev, f[0]);
    pkg = prev;
    has = 0;
    eachline(idx, hasline);
    if (!has) {
        out("New package: ");
        out(prev);
        out(*chan ? " (nightly)\n" : "\n");
    }
}

/* the first repository needs a newer ept: install it, then stop, as the
 * command is for the new one to carry out */
static void selfupdate(void)
{
    LIVE |= 1;
    fresh = 0;
    if (install("ept") == 0) {
        if (got)
            out("ept is updated: run the command again\n");
        else                            /* what it has is this one */
            say("no newer ept in ", repos[0]);
    }
    exit(1);
}

/* fetch the package list of the repository in use */
static int fetchlist(void)
{
    out("Fetching http://");
    out(repos[repo] + 7);
    out(chan);
    out("/index\n");
    strcpy(rpath, chan);
    strcat(rpath, "/index");
    if (fetch(rpath, INDEXNEW, -1, NULL) != 0)
        return -1;
    lines = known = last = newer = 0;
    eachline(INDEXNEW, checkline);
    if (!known || !last) {              /* a newer format, or cut short */
        say("unusable package list from ", repos[repo]);
        unlink(INDEXNEW);
        return -1;
    }
    if (access(idx, 0) == 0) {          /* not the first list from it */
        prev[0] = '\0';
        eachline(INDEXNEW, newline_);
    }
    return replace(INDEXNEW, idx);
}

/* fetch every repository's package list. One that fails is left out, except
 * that ept list makes do with the one saved last time */
static int update(void)
{
    int i, none = 0;

    mkdir(DIR, 0755);
    quiet = *chan;                      /* a repository need not have nightly builds */
    for (i = 0; i < nrepo; i++) {
        setrepo(i);
        if (fetchlist() == 0) {
            if (!newer)
                LIVE |= 1 << i;
            else if (!i)
                selfupdate();           /* runs the new ept: no return */
            else
                say("needs a newer ept, skipping ", repos[i]);
        } else if (quiet && notfound)
            none++;
        else if (listing && access(idx, 0) == 0) {
            out("Showing the saved package list\n");
            LIVE |= 1 << i;
        } else if (nrepo > 1)
            say("skipping ", repos[i]);
    }
    quiet = 0;
    if (none == nrepo)
        say("no nightly builds in ", nrepo > 1 ? "any repository" : repos[0]);
    return LIVE ? 0 : -1;
}
#endif

#ifndef TINY
static void hasline(char *s)
{
    if (pkgline(s) && !strcmp(f[0], pkg)) {
        has = 1;
        strcpy(fver, f[1]);
    }
}

/* use the repository that provides name: the last usable one whose list has
 * it, but ept itself only ever comes from the first. -1 if none has it */
static int findrepo(char *name)
{
    char *save = pkg;
    int i;

    pkg = name;
    for (i = strcmp(name, "ept") ? nrepo - 1 : 0; i >= 0; i--) {
        if (!(LIVE & 1 << i))
            continue;
        setrepo(i);
        has = 0;
        eachline(idx, hasline);
        if (has)
            break;
    }
    pkg = save;
    return i;
}
#endif

/* ---- list */
static char inst[512];          /* installed packages: " name version" each */
#ifdef TINY
static char prev[256], avail[256]; /* fields of a line fit */
#endif
static unsigned long total;
#ifndef TINY
static char ich;                /* the channel of the last instver() found */
static int cur, show;           /* the repository listed; its package is shown */
#endif

static char *instver(const char *name);

static void instline(char *s)
{
    if (split(s) != 5 || instver(f[0]))
        return;
#ifndef TINY
    if (f[4][0] == 'n')
        anyn = 1;
#endif
#ifdef TINY
    if (strlen(inst) + strlen(f[0]) + strlen(f[1]) + 3 < sizeof(inst)) {
#else
    if (strlen(inst) + strlen(f[0]) + strlen(f[1]) + 5 < sizeof(inst)) {
#endif
        strcat(inst, " ");
        strcat(inst, f[0]);
        strcat(inst, " ");
        strcat(inst, f[1]);
        strcat(inst, " ");
#ifndef TINY
        strcat(inst, f[4]);             /* " name version channel " */
        strcat(inst, " ");
#endif
    }
}

/* the installed version of name, or NULL */
static char *instver(const char *name)
{
    static char v[24];
    char *p, *e;
    int n = strlen(name);

    for (p = inst + 1; *p; p++)         /* " name version " */
        if (p[-1] == ' ' && !strncmp(p, name, n) && p[n] == ' ') {
            p += n + 1;
            e = strchr(p, ' ');
            if (e - p < sizeof(v)) {
                memcpy(v, p, e - p);
                v[e - p] = '\0';
#ifndef TINY
                ich = e[1];
#endif
                return v;
            }
        }
    return NULL;
}

static void pad(const char *s, int w)
{
    out(s);
    for (w -= strlen(s); w > 0; w--)
        out(" ");
}

#ifndef TINY
/* the description of pkg, into avail */
static void descline(char *s)
{
    int n = strlen(pkg);

    if (!strncmp(s, pkg, n) && s[n] == ' ')
        strcpy(avail, s + n + 1);
}

/* does s have the search text in it, in any case? */
static int hastext(const char *s)
{
    int i;

    for (;; s++) {
        for (i = 0; text[i] && s[i] && (s[i] | 32) == (text[i] | 32); i++)
            ;
        if (!text[i])
            return 1;
        if (!*s)
            return 0;
    }
}
#endif

static void showpkg(void)
{
    char *v;

    if (!prev[0])
        return;
#ifndef TINY
    if (!show)
        return;                         /* another repository provides it */
    if (text) {                         /* ept search: the name and description */
        pkg = prev;
        avail[0] = '\0';
        eachline(DESC, descline);
        if (hastext(prev) || hastext(avail)) {
            pad(prev, 15);
            out(avail);
            out("\n");
            matches++;
        }
        return;
    }
#endif
    pad(prev, 15);
    pad(avail, 13);
    pad("", 5 - strlen(num((total + 1023) / 1024)));
    out(num((total + 1023) / 1024));
    out("K");
    if ((v = instver(prev)) != NULL) {
        out("  [installed");
#ifdef TINY
        if (strcmp(v, avail)) {
            out(" ");
            out(v);
        }
#else
        if (strcmp(v, avail) || (ich == 'n') != (*chan != 0)) {
            out(" ");
            out(v);                     /* another version, or channel */
            if (ich == 'n')
                out(" nightly");
        }
#endif
        out("]");
    }
    out("\n");
}

static void listline(char *s)
{
    if (!pkgline(s))
        return;
    if (strcmp(f[0], prev)) {
        showpkg();
        strcpy(prev, f[0]);
        strcpy(avail, f[1]);
        total = 0;
#ifndef TINY
        total = atoun(f[3], 10);
        show = findrepo(prev) == cur;   /* reads other lists: f[] is done with */
        return;
#endif
    }
    total += atoun(f[3], 10);
}

#ifdef TINY
static int list(void)
{
    inst[0] = ' ';
    eachline(DB, instline);
    eachline(idx, listline);
    showpkg();
    return 0;
}
#else
/* an installed package that no package list has */
static void goneline(char *s)
{
    char *v;
    int c;

    if (split(s) != 5 || !strcmp(f[0], prev))
        return;
    strcpy(prev, f[0]);
    v = f[1];                           /* findrepo() reuses f[] */
    c = f[4][0];
    if (findrepo(prev) >= 0)
        return;
    pad(prev, 15);
    pad("-", 13);
    pad("", 5);
    out("-  [installed ");
    out(v);
    if (c == 'n')
        out(" nightly");
    out("]\n");
}

static int list(void)
{
    inst[0] = ' ';
    eachline(DB, instline);
    for (cur = 0; cur < nrepo; cur++) {
        if (!(LIVE & 1 << cur))
            continue;
        setrepo(cur);
        if (text) {                     /* its descriptions, if it has any */
            strcpy(rpath, chan);
            strcat(rpath, "/desc");
            quiet = 1;
            fetch(rpath, DESC, -1, NULL);
            quiet = 0;
        }
        prev[0] = '\0';
        eachline(idx, listline);
        showpkg();
    }
    if (text) {
        unlink(DESC);
        if (matches)
            return 0;
        say("no package matches ", text);
        return 1;
    }
    prev[0] = '\0';
    eachline(DB, goneline);
    return 0;
}
#endif

/* ---- install */

/* is p anything but an absolute path without empty, "." or ".." parts? */
static int badpath(const char *p)
{
    const char *q;
    int n;

    if (*p != '/' || strlen(ROOT) + strlen(p) + 8 >= sizeof(path))
        return 1;
    while (*p) {
        q = ++p;
        while (*p && *p != '/')
            p++;
        n = p - q;
        if (n == 0 || (q[0] == '.' && (n == 1 || (n == 2 && q[1] == '.'))))
            return 1;
    }
    return 0;
}

#ifndef TINY
static int same;
static char *smd5, *sdest;

static void sameline(char *s)
{
    if (split(s) == 5 && !strcmp(f[0], pkg) && !strcmp(f[2], smd5) && !strcmp(f[3], sdest))
        same = 1;
}

/* is the file of the package line in f[] installed as listed, and still
 * there? Then it need not be fetched again. Leaves its path in path */
static int unchanged(void)
{
    char *save[6];

    memcpy(save, f, sizeof(f));
    smd5 = f[2];
    sdest = f[5];
    same = 0;
    eachline(DB, sameline);
    memcpy(f, save, sizeof(f));
    strcpy(path, ROOT);
    strcat(path, f[5]);
    return same && access(path, 0) == 0;
}
#endif

/* download and install one file of the package */
static void fileline(char *s)
{
    char *p;
    unsigned n;

    if (bad || !pkgline(s) || strcmp(f[0], pkg))
        return;
    if (!found++) {
#ifdef TINY
        out("Installing ");
        out(pkg);
        out(" ");
        out(f[1]);
        out("\n");
#else
        strcpy(fver, f[1]);
#endif
    }
    if (badpath(f[5])) {
        say("bad path in package list: ", f[5]);
        bad = 1;
        return;
    }
#ifndef TINY
    if (repo && !strcasecmp(f[5], SELF)) {      /* any case: FAT has only one */
        say("only the first repository can install ", f[5]);
        bad = 1;
        return;
    }
#endif
    n = strlen(f[5]) + 1;
    if (nnew + n <= sizeof(newf))
        memcpy(newf + nnew, f[5], n);
    nnew += n;
#ifndef TINY
    if (!fresh && unchanged()) {        /* installed as listed: keep it */
        chmod(path, atoun(f[4], 8));
        return;
    }
    if (!got++) {
        out("Installing ");
        out(pkg);
        out(" ");
        out(fver);
        out(*chan ? " (nightly)\n" : "\n");
    }
#endif
    out("  ");
    out(f[5]);
    out("\n");
    strcpy(path, ROOT);
    strcat(path, f[5]);
    for (p = path + strlen(ROOT) + 1; *p; p++)  /* create the parent directories */
        if (*p == '/') {
            *p = '\0';
            mkdir(path, 0755);
            *p = '/';
        }
    strcpy(tmp, path);
    strcpy(strrchr(tmp, '/') + 1, "ept.tmp");
    strcpy(rpath, chan);
    strcat(rpath, "/pool/");
    strcat(rpath, pkg);
    strcat(rpath, f[5]);
    if (fetch(rpath, tmp, atoun(f[3], 10), f[2]) != 0
            || replace(tmp, path) != 0) {
        bad = 1;
        return;
    }
    chmod(path, atoun(f[4], 8));
    out("\n");
}

/* is d one of the files just installed? (only if they all fitted in newf) */
static int innew(const char *d)
{
    char *p;

    for (p = newf; p < newf + nnew; p += strlen(p) + 1)
        if (!strcmp(p, d))
            return 1;
    return 0;
}

/* the installed list without the package. Its files are deleted when it is
 * being removed, or when the new version no longer has them. A file that
 * another package listed now belongs to the package just installed */
static void keepline(char *s)
{
    char *t = s + strlen(s);
#ifndef TINY
    char *line = s;                     /* the reader's own buffer */
#endif

    if (split(s) != 5)
        return;
    if (!strcmp(f[0], pkg)) {
        if (!drop && (nnew > sizeof(newf) || innew(f[3])))
            return;                     /* the new version has it too, or can't tell */
        strcpy(path, ROOT);
        strcat(path, f[3]);
        unlink(path);
    } else if (!drop && nnew <= sizeof(newf) && innew(f[3])) {
        return;                         /* it belongs to the new package now */
    } else {
        for (; s < t; s++)              /* split() put NULs between the fields */
            if (!*s)
                *s = ' ';
        *t++ = '\n';
        write(outfd, line, t - line);
    }
}

/* the package's lines from the package list, as installed lines */
static void newline(char *s)
{
    if (!pkgline(s) || strcmp(f[0], pkg))
        return;
    strcpy(buf, f[0]);
    strcat(buf, " ");
    strcat(buf, f[1]);
    strcat(buf, " ");
    strcat(buf, f[2]);
    strcat(buf, " ");
    strcat(buf, f[5]);
    strcat(buf, cname);
    write(outfd, buf, strlen(buf));
}

/* write the installed list without pkg, plus pkg's lines from the package list if add */
static int rewrite(int add)
{
    drop = !add;
    if ((outfd = open(DBNEW, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
        say("cannot write ", DBNEW);
        return -1;
    }
    eachline(DB, keepline);
    if (add)
        eachline(idx, newline);
    if (close(outfd) < 0)
        return -1;
    return replace(DBNEW, DB);
}

/* add up the size and files of the package */
static void sizeline(char *s)
{
    if (pkgline(s) && !strcmp(f[0], pkg)
#ifndef TINY
            && (fresh || !unchanged())  /* only what will be fetched */
#endif
            ) {
        total += atoun(f[3], 10);
        nfiles++;
    }
}

/* show what the install needs and what is free; 1 to go ahead */
static int ask(void)
{
    struct stat st;
    struct statfs sf;

    out("Install ");
    out(num((total + 1023) / 1024));
    out("K, ");
    out(num(nfiles));
    out(" files");
    if (stat(ROOT "/", &st) == 0 && ustatfs(st.st_dev, &sf, 0) == 0) {
        out(" (");
        out(num(sf.f_bavail > 65535 ? 65535 : sf.f_bavail));
        out("K");
        if (sf.f_files) {               /* MINIX; FAT has no limit on files */
            out(", ");
            out(num(sf.f_ffree));
            out(" files");
        }
        out(" free)");
    }
    out("? [Y/n] ");
    return read(0, buf, sizeof(buf)) > 0 && (buf[0] == 'y' || buf[0] == 'Y' || buf[0] == '\n');
}

static int install(char *name)
{
#ifndef TINY
    if (findrepo(name) < 0) {
        say("no package named ", name);
        return 1;
    }
#endif
    pkg = name;
    found = bad = nnew = 0;
#ifndef TINY
    got = 0;
#endif
    eachline(idx, fileline);
    if (!found) {
        say("no package named ", name);
        return 1;
    }
    if (bad)
        return 1;
    if (rewrite(1) != 0)
        return 1;
#ifdef TINY
    if (!strcmp(name, "ept"))
        out("The full ept is installed; run ept again to use it\n");
#else
    if (!got) {
        out(name);
        out(" ");
        out(fver);
        out(" is up to date\n");
    }
#endif
    return 0;
}

static int remove_(char *name)
{
    pkg = name;
    inst[0] = ' ';
    inst[1] = '\0';
    eachline(DB, instline);
    if (!instver(name)) {
        say(name, " is not installed");
        return 1;
    }
    out("Removing ");
    out(name);
    out("\n");
    return rewrite(0) != 0;
}

#ifndef TINY
static char upg[512];           /* to update: channel letter, name NUL, version NUL each */
static unsigned nup;
static char *ups[2 + sizeof(upg) / 4];  /* the names in upg, from [2] on like argv */
static int nups;

/* take the channel of a package: s or n */
static void setchan(int c)
{
    chan = c == 'n' ? "/nightly" : "";
    cname[1] = c == 'n' ? 'n' : 's';
}

/* an installed package whose package list has another version */
static void upline(char *s)
{
    char *v;
    unsigned n;

    if (split(s) != 5 || !strcmp(f[0], prev))
        return;
    strcpy(prev, f[0]);
    v = f[1];                           /* findrepo() reuses f[] */
    setchan(f[4][0]);
    if (findrepo(prev) < 0 || !strcmp(v, fver))
        return;                         /* in no package list, or up to date */
    n = strlen(prev) + strlen(v) + 3;
    if (nup + n < sizeof(upg)) {
        upg[nup] = cname[1];
        ups[2 + nups++] = strcpy(upg + nup + 1, prev);
        strcpy(upg + nup + 2 + strlen(prev), v);
        nup += n;
    }
}

/* ept update: fetch the lists and find what changed. The number of packages
 * to update (in ups) or -1; main() then installs them as for ept install */
static int upall(void)
{
    inst[0] = ' ';
    eachline(DB, instline);             /* notes whether anything is nightly */
    setchan('s');
    if (update() != 0)
        return -1;
    if (anyn) {                         /* nightly lists only when they are needed */
        setchan('n');
        update();
    }
    prev[0] = '\0';
    eachline(DB, upline);
    if (!nups)
        out("All packages are up to date\n");
    return nups;
}
#endif

int main(int argc, char **argv)
{
    int i, rc = 0;
#ifndef TINY
    unsigned long t;
#endif

#ifdef TINY
    if (argc > 2 && !strcmp(argv[2], "-n")) {
        chan = "/nightly";
        idx = NINDEX;
        cname[1] = 'n';
        argv[2] = argv[1];
        argv++;
        argc--;
    }
    if (argc < 2 || (strcmp(argv[1], "list") && argc < 3)
            || (strcmp(argv[1], "list") && strcmp(argv[1], "install") && strcmp(argv[1], "remove"))) {
        out("usage: ept list [-n] | ept install [-n] <pkgs> | ept remove <pkgs>\n"
            "This is tinyept; \"ept install ept\" installs the full one.\n");
        return 1;
    }
#else
    while (argc > 2 && argv[2][0] == '-') {     /* -n and -r, in any order */
        if (argv[2][1] == 'n')
            setchan('n');
        else if (argv[2][1] == 'r')
            fresh = 1;
        else
            argc = 0;                   /* anything else: usage */
        argv[2] = argv[1];
        argv++;
        argc--;
    }
    /* list and update take no names, install and remove need some, search one at most */
    if (argc < 2 || (strcmp(argv[1], "list") && strcmp(argv[1], "install") && strcmp(argv[1], "remove")
                && strcmp(argv[1], "update") && strcmp(argv[1], "search"))
            || (argv[1][0] == 's' ? argc > 3 : (argc < 3) != (argv[1][0] == 'l' || argv[1][0] == 'u'))) {
        out("ept " VERSION "\n"
            "usage: ept list [-n]\n"
            "       ept search [-n] [<text>]\n"
            "       ept install [-n] [-r] <pkgs>\n"
            "       ept update\n"
            "       ept remove <pkgs>\n");
        return 1;
    }
#endif
    if (argv[1][0] == 'r') {
        for (i = 2; i < argc; i++)
            rc |= remove_(argv[i]);
        return rc;
    }
#ifdef TINY
    out("Fetching http://" REPOHOST);
    out(chan);
    out("/index\n");
#else
    readcfg();
    if (argv[1][0] == 's')
        text = argc > 2 ? argv[2] : "";
    listing = argv[1][0] == 'l' || text;
    if (argv[1][0] == 'u') {            /* update: the packages that changed */
        if ((i = upall()) <= 0)
            return i < 0;
        ups[1] = argv[1];
        argv = ups;
        argc = i + 2;
    } else
#endif
    if (update() != 0)
        return 1;
#ifdef TINY
    if (argv[1][0] == 'l')
#else
    if (argv[1][0] == 'l' || text)
#endif
        return list();
    for (i = 2; i < argc; i++) {        /* one question for all the packages */
        pkg = argv[i];
#ifndef TINY
        if (argv == ups)
            setchan(pkg[-1]);           /* its own channel */
        t = total;
        if (findrepo(pkg) >= 0)
#endif
        eachline(idx, sizeline);
#ifndef TINY
        if (argv == ups) {              /* "  name  old -> new  NK" */
            out("  ");
            pad(pkg, 15);
            out(pkg + strlen(pkg) + 1);
            out(" -> ");
            out(fver);
            out("  ");
            out(num((total - t + 1023) / 1024));
            out(*chan ? "K (nightly)\n" : "K\n");
        }
#endif
    }
    if (total && !ask())
        return 0;
    for (i = 2; i < argc; i++) {
#ifndef TINY
        if (argv == ups)
            setchan(argv[i][-1]);
#endif
        rc |= install(argv[i]);
    }
    return rc;
}
