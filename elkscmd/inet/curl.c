/*
 * curl -- HTTP/FTP download tool for ELKS
 *
 * Usage:
 *   curl [-vojf] [-u user:pass] [-d data] <url>
 *   curl --json [-d data] <url>
 *
 * Schemes: http://, ftp://
 *   -o file        write output to file instead of stdout
 *   -v             verbose (URL and status on stderr)
 *   -u u:p         user and password for auth
 *   -d "data"      HTTP POST with form-urlencoded body
 *   --json         HTTP POST with application/json body (use with -d)
 *   -f "key"       extract top-level JSON key from HTTP response
 *
 * 29 Mar 2024 Greg Haerr <greg@censoft.com>
 */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <arpa/inet.h>

char buffer[4096];
char pasv_host[64];
int pasv_port;

static int net_connect(char *host, int port)
{
	int fd;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;

	addr.sin_family = AF_INET;
	addr.sin_port = PORT_ANY;
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		goto err;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = in_gethostbyname(host);
	if (!addr.sin_addr.s_addr)
		goto err;

	if (in_connect(fd, (struct sockaddr *)&addr, sizeof(addr), 10) < 0)
		goto err;
	return fd;
err:
	close(fd);
	return -1;
}

static void net_close(int fd, int errflag)
{
	struct linger l;
	l.l_onoff = errflag ? 1 : 0;
	l.l_linger = 0;
	setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l));
	close(fd);
}

/*
 * Skip whitespace in JSON text
 */
static char *skip_spc(char *p)
{
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
	return p;
}

/*
 * Parse a JSON string at *p. On success set *start, *len and return
 * pointer after closing quote. On failure return NULL.
 */
static char *json_str(char *p, char **start, int *len)
{
	p = skip_spc(p);
	if (*p != '"') return NULL;
	p++;
	*start = p;
	while (*p && *p != '"') {
		if (*p == '\\') p++;
		p++;
	}
	*len = p - *start;
	if (*p == '"') p++;
	return p;
}

/*
 * Find the value for the first top-level JSON key matching `key`.
 * Returns pointer to the start of the value, sets *vlen to its length.
 */
static char *json_find(char *body, const char *key, int *vlen)
{
	char *p, *kstart, *vp;
	int klen, depth;

	p = skip_spc(body);
	if (*p != '{') return NULL;
	p++;

	while (*p) {
		p = skip_spc(p);
		if (*p == '}') return NULL;
		if (*p != '"') return NULL;

		p = json_str(p, &kstart, &klen);
		if (!p) return NULL;
		p = skip_spc(p);
		if (*p != ':') return NULL;
		p++;
		p = skip_spc(p);

		if (klen == (int)strlen(key) && strncmp(kstart, key, klen) == 0) {
			*vlen = 0;
			vp = p;
			if (*vp == '"') {
				p = json_str(p, &kstart, &klen);
				if (!p) return NULL;
				*vlen = p - vp;
				/* strip surrounding quotes from value */
				*vlen -= 2;
				return vp + 1;
			}
			depth = 0;
			if (*vp == '{' || *vp == '[') {
				char close = (*vp == '{') ? '}' : ']';
				while (*p && !(depth == 0 && (*p == close))) {
					if (*p == '{' || *p == '[') depth++;
					if (*p == '}' || *p == ']') depth--;
					p++;
				}
				if (*p == close) p++;
				*vlen = p - vp;
			} else {
				while (*p && *p != ',' && *p != '}' && *p != ']'
					&& *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
					p++;
				*vlen = p - vp;
			}
			return vp;
		}

		/* skip this value */
		if (*p == '{' || *p == '[') {
			char close = (*p == '{') ? '}' : ']';
			depth = 1;
			p++;
			while (*p && depth > 0) {
				if (*p == '"') { p = json_str(p, &kstart, &klen); continue; }
				if (*p == close) depth--;
				if (depth > 0) p++;
			}
			if (*p) p++;
		} else if (*p == '"') {
			p = json_str(p, &kstart, &klen);
		} else {
			while (*p && *p != ',' && *p != '}' && *p != ']') p++;
		}
		p = skip_spc(p);
		if (*p == ',') p++;
	}
	return NULL;
}

/*
 * HTTP GET/POST: send request, strip headers, write body to outfd.
 * If key is set, buffer body and extract that JSON key instead.
 */
static int http_get(char *host, int port, char *path, int outfd,
	char *postdata, int use_json, char *key)
{
	int fd, skip, n, remaining;
	char *p;
	int body_len, vlen;

	if (port == 0) port = 80;
	fd = net_connect(host, port);
	if (fd < 0) {
		perror(host);
		return -1;
	}

	if (postdata)
		write(fd, "POST ", 5);
	else
		write(fd, "GET ", 4);
	write(fd, path, strlen(path));
	write(fd, " HTTP/1.0\r\n", 11);
	write(fd, "User-Agent: curl\r\n", 18);
	write(fd, "Connection: Close\r\n", 19);
	write(fd, "Host: ", 6);
	write(fd, host, strlen(host));
	write(fd, "\r\n", 2);
	if (postdata) {
		if (use_json)
			write(fd, "Content-Type: application/json\r\n", 32);
		else
			write(fd, "Content-Type: application/x-www-form-urlencoded\r\n", 49);
		{
			char cl[32];
			int len = strlen(postdata);
			sprintf(cl, "Content-Length: %d\r\n", len);
			write(fd, cl, strlen(cl));
		}
	}
	write(fd, "\r\n", 2);
	if (postdata)
		write(fd, postdata, strlen(postdata));

	if (key) {
		/* buffer mode for JSON extraction */
		body_len = 0;
		skip = 1;
		while ((n = read(fd, buffer + body_len,
			sizeof(buffer) - body_len)) > 0) {
			int off = 0;
			if (skip) {
				p = buffer + body_len;
				remaining = n;
				while (remaining > 0 && skip) {
					if (*p == '\r' || *p == '\n') {
						if (*p == '\n') skip = 0;
						p++; remaining--; off++;
					} else {
						while (remaining > 0 && *p != '\n')
							{ p++; remaining--; off++; }
						if (remaining > 0) { p++; remaining--; off++; }
						if (remaining > 0 && *p == '\n')
							{ p++; remaining--; off++; skip = 0; }
					}
				}
			body_len += remaining;
			if (remaining > 0)
			    memmove(buffer, buffer + off, body_len);
			} else {
				body_len += n;
				if (body_len >= (int)sizeof(buffer))
					break;
			}
		}
		net_close(fd, 0);

		if (body_len == 0) return 0;
		p = json_find(buffer, key, &vlen);
		if (p) {
			if (write(outfd, p, vlen) != vlen) {
				perror("write");
				return -1;
			}
		}
		return 0;
	}

	/* write-through mode */
	skip = 1;
	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
		if (skip) {
			p = buffer;
			remaining = n;
			while (remaining > 0 && skip) {
				if (*p == '\r' || *p == '\n') {
					if (*p == '\n') skip = 0;
					p++; remaining--;
				} else {
					while (remaining > 0 && *p != '\n')
						{ p++; remaining--; }
					if (remaining > 0) { p++; remaining--; }
					if (remaining > 0 && *p == '\n')
						{ p++; remaining--; skip = 0; }
				}
			}
			if (remaining > 0)
				if (write(outfd, p, remaining) != remaining) {
					perror("write");
					net_close(fd, 1);
					return -1;
				}
		} else {
			if (write(outfd, buffer, n) != n) {
				perror("write");
				net_close(fd, 1);
				return -1;
			}
		}
	}
	net_close(fd, 0);
	return 0;
}

/*
 * Raw TCP mode: connect, send path+newline, read response to outfd.
 */
static int tcp_get(char *host, int port, char *path, int outfd)
{
	int fd, n;

	if (port == 0) {
		fprintf(stderr, "curl: tcp:// requires a port\n");
		return -1;
	}
	fd = net_connect(host, port);
	if (fd < 0) {
		perror(host);
		return -1;
	}
	if (*path == '/') path++;
	write(fd, path, strlen(path));
	write(fd, "\n", 1);
	while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
		if (write(outfd, buffer, n) != n) {
			perror("write");
			net_close(fd, 1);
			return -1;
		}
	}
	net_close(fd, 0);
	return 0;
}

/*
 * Parse 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
 */
static void parse_pasv(char *reply)
{
	unsigned char n[6];
	int i;
	char *p;

	pasv_port = 0;
	p = reply;
	while (*p && *p != '(') p++;
	if (!*p) return;
	p++;
	for (i = 0; i < 6; i++) {
		n[i] = atoi(p);
		if (i == 5) break;
		p = strchr(p, ',');
		if (!p) return;
		p++;
	}
	sprintf(pasv_host, "%d.%d.%d.%d", n[0], n[1], n[2], n[3]);
	pasv_port = n[4] * 256 + n[5];
}

/*
 * Read a line from socket, handling \r\n termination
 */
static int ftp_read_line(int fd, char *buf, int size)
{
	int i = 0;
	char c;
	
	while (i < size - 1) {
		if (read(fd, &c, 1) <= 0)
			return -1;
		if (c == '\n')
			break;
		if (c != '\r')
			buf[i++] = c;
	}
	buf[i] = '\0';
	return i;
}

/*
 * Write a line to socket with \r\n termination
 */
static int ftp_write_line(int fd, char *fmt, ...)
{
	char buf[256];
	va_list ap;
	int len;
	
	va_start(ap, fmt);
	len = vsnprintf(buf, sizeof(buf) - 2, fmt, ap);
	va_end(ap);
	
	if (len < 0) return -1;
	buf[len++] = '\r';
	buf[len++] = '\n';
	return write(fd, buf, len);
}

/*
 * Read FTP reply, handling multi-line responses
 */
static int ftp_reply(int fd)
{
	char reply[256];
	char code[4];
	int s, first;

	do {
		first = 1;
		do {
			if (ftp_read_line(fd, reply, sizeof(reply)) < 0)
				return -1;
			if (first) {
				first = 0;
				strncpy(code, reply, 3);
				code[3] = '\0';
			}
		} while (strncmp(reply, code, 3) || reply[3] == '-');
		s = atoi(code);
		if (s == 227)
			parse_pasv(reply);
	} while (s < 200 && s != 125 && s != 150);
	return s;
}

/*
 * Send FTP command and read response
 */
static int ftp_cmd(int fd, char *cmd, char *arg)
{
	if (ftp_write_line(fd, "%s%s%s", cmd, *arg ? " " : "", arg) < 0)
		return -1;
	return ftp_reply(fd);
}

/*
 * FTP passive mode GET, PUT, or LIST.  If upload is set, store that file.
 */
static int ftp_get(char *host, int port, char *user, char *pass,
	char *path, int outfd, char *upload, int verbose)
{
	int fd, dfd, s, n, is_dir;
	int local;
	char *p;

	if (port == 0) port = 21;
	if (!*user) strcpy(user, "anonymous");
	if (!*pass) strcpy(pass, "curl@x.com");
	is_dir = path[strlen(path) - 1] == '/';

	fd = net_connect(host, port);
	if (fd < 0) {
		perror(host);
		return -1;
	}

	s = ftp_reply(fd);
	if (s / 100 != 2) {
		fprintf(stderr, "curl: FTP connect error: %d\n", s);
		goto err;
	}
	s = ftp_cmd(fd, "USER", user);
	if (s / 100 == 3)
		s = ftp_cmd(fd, "PASS", pass);
	if (s / 100 != 2) {
		fprintf(stderr, "curl: FTP auth failed: %d\n", s);
		goto err;
	}

	p = path;
	if (*p == '/') p++;
	while (*p) {
		char *slash = strchr(p, '/');
		if (slash) {
			*slash = '\0';
			if (*p) {
				s = ftp_cmd(fd, "CWD", p);
				if (s / 100 != 2) {
					fprintf(stderr, "curl: CWD %s failed: %d\n", p, s);
					*slash = '/';
					goto err;
				}
			}
			*slash = '/';
			p = slash + 1;
		} else {
			break;
		}
	}

	s = ftp_cmd(fd, "TYPE", "I");
	if (s / 100 != 2) {
		fprintf(stderr, "curl: TYPE I failed: %d\n", s);
		goto err;
	}

	pasv_port = 0;
	s = ftp_cmd(fd, "PASV", "");
	if (s != 227 || pasv_port == 0) {
		fprintf(stderr, "curl: PASV failed: %d\n", s);
		goto err;
	}
	dfd = net_connect(pasv_host, pasv_port);
	if (dfd < 0) {
		perror(pasv_host);
		goto err;
	}

	if (upload) {
		local = open(upload, O_RDONLY);
		if (local < 0) {
			perror(upload);
			net_close(dfd, 1);
			goto err;
		}
		s = ftp_cmd(fd, "STOR", p);
		if (s / 100 != 1) {
			fprintf(stderr, "curl: STOR failed: %d\n", s);
			close(local);
			net_close(dfd, 1);
			goto err;
		}
		while ((n = read(local, buffer, sizeof(buffer))) > 0) {
			if (write(dfd, buffer, n) != n) {
				perror("write data");
				close(local);
				net_close(dfd, 1);
				goto err;
			}
		}
		close(local);
	} else if (is_dir) {
		s = ftp_cmd(fd, verbose ? "LIST" : "NLST", "");
		if (s / 100 != 1) {
			fprintf(stderr, "curl: LIST failed: %d\n", s);
			net_close(dfd, 1);
			goto err;
		}
		while ((n = read(dfd, buffer, sizeof(buffer))) > 0) {
			if (write(outfd, buffer, n) != n) {
				perror("write");
				net_close(dfd, 1);
				goto err;
			}
		}
	} else {
		char *fname = path;
		char *last = strrchr(path, '/');
		if (last) fname = last + 1;
		s = ftp_cmd(fd, "RETR", fname);
		if (s / 100 != 1) {
			fprintf(stderr, "curl: RETR failed: %d\n", s);
			net_close(dfd, 1);
			goto err;
		}
		while ((n = read(dfd, buffer, sizeof(buffer))) > 0) {
			if (write(outfd, buffer, n) != n) {
				perror("write");
				net_close(dfd, 1);
				goto err;
			}
		}
	}
	net_close(dfd, 0);
	s = ftp_reply(fd);
	net_close(fd, 0);
	return 0;

err:
	ftp_cmd(fd, "QUIT", "");
	net_close(fd, 1);
	return -1;
}

static void usage(void)
{
	fprintf(stderr, "Usage: curl [-vojfT] [-u user:pass] [-d data] <url>\n");
	fprintf(stderr, "  -o file    output to file\n");
	fprintf(stderr, "  -v         verbose\n");
	fprintf(stderr, "  -u u:p     user:password\n");
	fprintf(stderr, "  -d data    HTTP POST with form-urlencoded body\n");
	fprintf(stderr, "  --json     HTTP POST with application/json (use with -d)\n");
	fprintf(stderr, "  -f key     extract top-level JSON key from response\n");
	fprintf(stderr, "  -T file    upload file via FTP\n");
	fprintf(stderr, "Schemes: http://, ftp://, tcp://\n");
}

int main(int argc, char **argv)
{
	char *url, host[64], user[64], pass[64], path[256];
	int port, outfd, verbose;
	char *outfile, *ps, *p, *at, *postdata, *jkey, *upload;
	int use_json;

	user[0] = pass[0] = '\0';
	outfile = NULL;
	outfd = 1;
	verbose = 0;
	postdata = NULL;
	jkey = NULL;
	upload = NULL;
	use_json = 0;

	argv++; argc--;
	while (argc > 0 && argv[0][0] == '-') {
		if (argv[0][1] == '-' && strcmp(argv[0], "--json") == 0) {
			use_json = 1;
			argv++; argc--;
			continue;
		}
		switch (argv[0][1]) {
		case 'o':
			argv++; argc--;
			if (argc == 0) { usage(); return 1; }
			outfile = argv[0];
			break;
		case 'v':
			verbose = 1;
			break;
		case 'u':
			argv++; argc--;
			if (argc == 0) { usage(); return 1; }
			p = strchr(argv[0], ':');
			if (p) {
				*p = '\0';
				strcpy(user, argv[0]);
				*p = ':';
				strcpy(pass, p + 1);
			} else {
				strcpy(user, argv[0]);
			}
			break;
		case 'd':
			argv++; argc--;
			if (argc == 0) { usage(); return 1; }
			postdata = argv[0];
			break;
		case 'f':
			argv++; argc--;
			if (argc == 0) { usage(); return 1; }
			jkey = argv[0];
			break;
		case 'T':
			argv++; argc--;
			if (argc == 0) { usage(); return 1; }
			upload = argv[0];
			break;
		default:
			usage();
			return 1;
		}
		argv++; argc--;
	}

	if (argc != 1) { usage(); return 1; }
	url = argv[0];

	if (strncmp(url, "http://", 7) == 0)
		ps = url + 7;
	else if (strncmp(url, "ftp://", 6) == 0)
		ps = url + 6;
	else if (strncmp(url, "tcp://", 6) == 0)
		ps = url + 6;
	else {
		fprintf(stderr, "curl: must use http://, ftp:// or tcp://\n");
		return 1;
	}

	host[0] = '\0';
	port = 0;

	at = strchr(ps, '@');
	if (at) {
		*at = '\0';
		p = strchr(ps, ':');
		if (p) {
			*p = '\0';
			strcpy(user, ps);
			strcpy(pass, p + 1);
		} else {
			strcpy(user, ps);
		}
		ps = at + 1;
	}

	p = ps;
	while (*p && *p != '/' && *p != ':') p++;
	strncpy(host, ps, p - ps);
	host[p - ps] = '\0';
	if (*p == ':') {
		p++;
		port = 0;
		while (*p >= '0' && *p <= '9')
			port = port * 10 + (*p++ - '0');
	}
	if (*p == '/')
		strcpy(path, p);
	else
		strcpy(path, "/");

	if (verbose) {
		char *scheme = "http";
		int defport = 80;
		if (strncmp(url, "ftp://", 6) == 0)
			{ scheme = "ftp"; defport = 21; }
		else if (strncmp(url, "tcp://", 6) == 0)
			{ scheme = "tcp"; defport = 0; }
		fprintf(stderr, "curl: %s://%s:%d%s\n",
			scheme, host, port ? port : defport, path);
	}

	if (outfile) {
		outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (outfd < 0) {
			perror(outfile);
			return 1;
		}
	}

	if (strncmp(url, "http://", 7) == 0)
		return http_get(host, port, path, outfd, postdata, use_json, jkey);
	if (strncmp(url, "tcp://", 6) == 0)
		return tcp_get(host, port, path, outfd);

	return ftp_get(host, port, user, pass, path, outfd, upload, verbose);
}
