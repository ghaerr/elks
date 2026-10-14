/*
 * test_ktcp - measure ktcp overhead on CPU bound processes and socket reads
 *
 * Tests:
 *   cpu    time a fixed CPU bound loop with a socket idle, with received
 *          data left unread (pending), and after reading it (drained).
 *          With ktcp polling while data is unread, pending runs slower.
 *   read   receive a bulk transfer using several read sizes, reporting
 *          throughput and the longest wait between reads. Gaps of 100ms can
 *          come from the sender's write backoff when its send window is full;
 *          longer or repeated stalls suggest ktcp missed telling the kernel
 *          about remaining data.
 *   rtt    one byte ping-pong round trip time.
 *
 * By default the peer is a forked child connected over localhost.
 * With -r host, the cpu test instead connects to a remote server that
 * sends data on connect without being asked (e.g. an SSH server, port 22).
 *
 * Elapsed (wall clock) time is measured, since ktcp's CPU time is not
 * charged to this process. Timer resolution is one clock tick (10ms).
 *
 * Usage: test_ktcp [-v] [-t secs] [-n reps] [-p port] [-b bytes] [-r host]
 *		[cpu|read|rtt ...]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_PORT	5555
#define PENDING_LEN	512	/* bytes left unread in cpu test */
#define CHUNK		1024	/* sender write size in read test */
#define MAXREPS		16
#define RTT_ROUNDS	100

static int target_secs = 10;		/* length of each cpu test phase */
static int reps = 3;
static int port;
static long bulk_len = 16384L;
static char *remote_host;
static int verbose;

static long t0_sec;
static volatile unsigned int sink;
static unsigned long work_units;
static unsigned char buf[CHUNK];

#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))

static int read_sizes[] = { 16, 128, 512, CHUNK };

static long now_ms(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return (tv.tv_sec - t0_sec) * 1000L + tv.tv_usec / 1000;
}

static void step(const char *msg)
{
	if (verbose)
		fprintf(stderr, "[%d] %s\n", getpid(), msg);
}

static void fail(const char *msg)
{
	perror(msg);
	exit(1);
}

/* CPU bound work with no system calls, 1000 xorshift steps per unit */
static void work(unsigned long units)
{
	unsigned int a = 1, b = 0;
	int i;

	while (units--) {
		for (i = 0; i < 1000; i++) {
			a ^= a << 7;
			a ^= a >> 9;
			a ^= a << 8;
			b += a;
		}
	}
	sink = b;
}

static long timed_work(void)
{
	long start = now_ms();

	work(work_units);
	return now_ms() - start;
}

/* choose work_units so that one timed_work() takes about target_secs */
static void calibrate(void)
{
	unsigned long units = 1;
	long ms;

	for (;;) {
		long start = now_ms();
		work(units);
		ms = now_ms() - start;
		if (ms >= 1000)
			break;
		units <<= 1;
	}
	work_units = units * (target_secs * 1000L) / ms;
	if (work_units == 0)
		work_units = 1;
	printf("calibrate: %lu units = %ld ms, using %lu units for %d s\n",
		units, ms, work_units, target_secs);
}

static int wait_readable(int fd, long ms)
{
	fd_set fds;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	tv.tv_sec = ms / 1000;
	tv.tv_usec = (ms % 1000) * 1000L;
	return select(fd + 1, &fds, NULL, NULL, &tv) > 0;
}

/* read all data already received, never blocking in read() */
static long drain(int fd)
{
	long total = 0;
	int n;

	while (wait_readable(fd, 0)) {
		n = read(fd, buf, sizeof(buf));
		if (n <= 0)
			break;
		total += n;
	}
	return total;
}

static int writeall(int fd, unsigned char *p, int len)
{
	int n;

	while (len > 0) {
		n = write(fd, p, len);
		if (n <= 0)
			return -1;
		p += n;
		len -= n;
	}
	return 0;
}

static void sort_longs(long *v, int n)
{
	int i, j;
	long t;

	for (i = 1; i < n; i++)
		for (j = i; j > 0 && v[j - 1] > v[j]; j--) {
			t = v[j];
			v[j] = v[j - 1];
			v[j - 1] = t;
		}
}

static long median(long *v, int n)
{
	long tmp[MAXREPS];

	memcpy(tmp, v, n * sizeof(long));
	sort_longs(tmp, n);
	return tmp[n / 2];
}

/* print ms as seconds with 2 decimals */
static void print_secs(const char *label, long ms)
{
	printf("%s %3ld.%02ld s", label, ms / 1000, (ms % 1000) / 10);
}

/* print a/b as a ratio with 2 decimals */
static void print_ratio(const char *label, long a, long b)
{
	long r = b ? (a * 100L + b / 2) / b : 0;

	printf("%s %ld.%02ldx", label, r / 100, r % 100);
}

/***** forked localhost peers *****/

/* cpu test: send PENDING_LEN bytes each time a command byte is received */
static void cpu_child(int fd)
{
	char c;

	memset(buf, 'p', PENDING_LEN);
	while (read(fd, &c, 1) == 1)
		if (writeall(fd, buf, PENDING_LEN) < 0)
			break;
}

/* read test: send bulk_len bytes then wait for close */
static void bulk_child(int fd)
{
	long left = bulk_len;
	int n;

	memset(buf, 'b', sizeof(buf));
	while (left > 0) {
		n = left > CHUNK ? CHUNK : (int)left;
		if (writeall(fd, buf, n) < 0)
			return;
		left -= n;
	}
	while (read(fd, buf, sizeof(buf)) > 0)
		continue;
}

/* rtt test: echo everything */
static void echo_child(int fd)
{
	int n;

	while ((n = read(fd, buf, sizeof(buf))) > 0)
		if (writeall(fd, buf, n) < 0)
			break;
}

static pid_t child;

/* create a socket bound to an ephemeral port, required before connect */
static int client_socket(void)
{
	struct sockaddr_in local;
	int fd;

	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		fail("socket");
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = htonl(INADDR_ANY);
	local.sin_port = 0;
	if (bind(fd, (struct sockaddr *)&local, sizeof(local)) < 0)
		fail("bind local");
	return fd;
}

/* fork a peer running fn connected over localhost, return our end */
static int open_loopback(void (*fn)(int))
{
	struct sockaddr_in addr;
	int lfd, fd, on = 1;

	if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		fail("socket");
	if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		perror("SO_REUSEADDR");
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(lfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		fail("bind");
	if (listen(lfd, 1) < 0)
		fail("listen");
	step("listening");

	child = fork();
	if (child == (pid_t)-1)
		fail("fork");
	if (child == 0) {
		close(lfd);
		fd = client_socket();
		addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		step("child connecting");
		if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
			fail("child connect");
		step("child connected");
		fn(fd);
		close(fd);
		exit(0);
	}

	if ((fd = accept(lfd, NULL, NULL)) < 0)
		fail("accept");
	step("accepted");
	close(lfd);
	return fd;
}

static void close_loopback(int fd)
{
	close(fd);
	waitpid(child, NULL, 0);
	child = 0;
}

static int open_remote(void)
{
	struct sockaddr_in addr;
	int fd;

	fd = client_socket();
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = in_gethostbyname(remote_host);
	addr.sin_port = htons(port);
	if (addr.sin_addr.s_addr == 0) {
		fprintf(stderr, "Can't resolve %s\n", remote_host);
		exit(1);
	}
	if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		fail("connect");
	return fd;
}

/***** tests *****/

static void test_cpu(void)
{
	long idle[MAXREPS], pend[MAXREPS], drained[MAXREPS];
	long got;
	int i, fd = -1;

	printf("\n== cpu: %d s CPU loop, socket idle / %s unread / drained ==\n",
		target_secs, remote_host ? "server data" : "512 bytes");
	calibrate();
	if (!remote_host)
		fd = open_loopback(cpu_child);

	for (i = 0; i < reps; i++) {
		/* with -r, idle phase has no connection */
		sleep(1);
		idle[i] = timed_work();
		step("idle done");

		if (remote_host)
			fd = open_remote();
		else if (write(fd, "g", 1) != 1)
			fail("write");
		if (!wait_readable(fd, 10000L)) {
			fprintf(stderr, "No data received from peer\n");
			exit(1);
		}
		step("data readable");
		sleep(1);		/* let ktcp finish ACKs and settle */
		if (!wait_readable(fd, 0)) {
			fprintf(stderr, "Pending data disappeared\n");
			exit(1);
		}
		pend[i] = timed_work();
		step("pending done");

		got = drain(fd);
		sleep(1);
		drained[i] = timed_work();
		if (remote_host)
			close(fd);

		printf("rep %d:", i + 1);
		print_secs("  idle", idle[i]);
		print_secs("  pending", pend[i]);
		print_secs("  drained", drained[i]);
		print_ratio("  pending/idle", pend[i], idle[i]);
		printf("  (%ld bytes)\n", got);
	}
	if (!remote_host)
		close_loopback(fd);

	printf("median:");
	print_secs(" idle", median(idle, reps));
	print_secs("  pending", median(pend, reps));
	print_secs("  drained", median(drained, reps));
	print_ratio("  pending/idle", median(pend, reps), median(idle, reps));
	printf("\n");
}

static void test_read(void)
{
	long elapsed[MAXREPS], start, last, t, gap, maxgap, got;
	unsigned int s;
	int i, n, fd, size, nreads;

	printf("\n== read: receive %ld bytes over localhost by read size ==\n",
		bulk_len);
	for (s = 0; s < ARRAY_SIZE(read_sizes); s++) {
		size = read_sizes[s];
		maxgap = 0;
		nreads = 0;
		for (i = 0; i < reps; i++) {
			fd = open_loopback(bulk_child);
			got = 0;
			nreads = 0;
			start = last = now_ms();
			while (got < bulk_len) {
				n = read(fd, buf, size);
				if (n <= 0)
					fail("read");
				t = now_ms();
				gap = t - last;
				if (gap > maxgap)
					maxgap = gap;
				last = t;
				got += n;
				nreads++;
			}
			elapsed[i] = now_ms() - start;
			close_loopback(fd);
		}
		t = median(elapsed, reps);
		printf("size %4d: median %5ld ms  %5ld bytes/s  %4d reads  max gap %ld ms\n",
			size, t, t ? bulk_len * 1000L / t : 0L, nreads, maxgap);
	}
}

static void test_rtt(void)
{
	long elapsed[MAXREPS], start, t;
	int i, r, fd;
	char c = 'x';

	printf("\n== rtt: %d one byte round trips over localhost ==\n",
		RTT_ROUNDS);
	fd = open_loopback(echo_child);
	for (i = 0; i < reps; i++) {
		start = now_ms();
		for (r = 0; r < RTT_ROUNDS; r++) {
			if (write(fd, &c, 1) != 1 || read(fd, &c, 1) != 1)
				fail("rtt");
		}
		elapsed[i] = now_ms() - start;
	}
	close_loopback(fd);
	t = median(elapsed, reps);
	printf("median %ld ms total, %ld.%01ld ms per round trip\n",
		t, t / RTT_ROUNDS, (t * 10 / RTT_ROUNDS) % 10);
}

static void usage(void)
{
	fprintf(stderr,
		"Usage: test_ktcp [-v] [-t secs] [-n reps] [-p port] [-b bytes] [-r host] [cpu|read|rtt ...]\n"
		"  -t  seconds per cpu test phase (default 10)\n"
		"  -n  repetitions (default 3, max %d)\n"
		"  -p  port (default %d, or 22 with -r)\n"
		"  -b  bytes per read test transfer (default 16384)\n"
		"  -r  cpu test against remote server that sends on connect (e.g. sshd)\n"
		"  -v  print progress of each step\n",
		MAXREPS, DEFAULT_PORT);
	exit(1);
}

static void cleanup(int sig)
{
	if (child > 0)
		kill(child, SIGTERM);
	exit(1);
}

int main(int argc, char **argv)
{
	struct timeval tv;
	int c, i, run_cpu = 0, run_read = 0, run_rtt = 0;

	while ((c = getopt(argc, argv, "t:n:p:b:r:v")) != -1) {
		switch (c) {
		case 't':
			target_secs = atoi(optarg);
			break;
		case 'n':
			reps = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'b':
			bulk_len = atol(optarg);
			break;
		case 'r':
			remote_host = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			usage();
		}
	}
	if (target_secs < 1 || reps < 1 || reps > MAXREPS || bulk_len < 1)
		usage();
	if (!port)
		port = remote_host ? 22 : DEFAULT_PORT;

	for (i = optind; i < argc; i++) {
		if (!strcmp(argv[i], "cpu"))
			run_cpu = 1;
		else if (!strcmp(argv[i], "read"))
			run_read = 1;
		else if (!strcmp(argv[i], "rtt"))
			run_rtt = 1;
		else
			usage();
	}
	if (!run_cpu && !run_read && !run_rtt)
		run_cpu = run_read = run_rtt = 1;
	if (remote_host && (run_read || run_rtt)) {
		if (optind < argc) {
			fprintf(stderr, "read and rtt tests are localhost only\n");
			exit(1);
		}
		run_read = run_rtt = 0;
	}

	signal(SIGINT, cleanup);
	signal(SIGPIPE, SIG_IGN);
	gettimeofday(&tv, NULL);
	t0_sec = tv.tv_sec;
	setvbuf(stdout, NULL, _IONBF, 0);

	if (run_cpu)
		test_cpu();
	if (run_read)
		test_read();
	if (run_rtt)
		test_rtt();
	return 0;
}
