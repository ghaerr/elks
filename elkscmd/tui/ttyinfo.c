#if 0
/*─────────────────────────────────────────────────────────────────╗
│ To the extent possible under law, Justine Tunney has waived      │
│ all copyright and related or neighboring rights to this file,    │
│ as it is written in the following disclaimers:                   │
│   • http://unlicense.org/                                        │
│   • http://creativecommons.org/publicdomain/zero/1.0/            │
╚─────────────────────────────────────────────────────────────────*/
#endif
//#include "dsp/tty/tty.h"
//#include "libc/calls/calls.h"
//#include "libc/calls/ioctl.h"
//#include "libc/calls/struct/sigaction.h"
//#include "libc/calls/termios.h"
//#include "libc/errno.h"
//#include "libc/fmt/fmt.h"
//#include "libc/log/check.h"
//#include "libc/log/log.h"
//#include "libc/runtime/runtime.h"
//#include "libc/stdio/stdio.h"
//#include "libc/str/str.h"
//#include "libc/sysv/consts/exit.h"
//#include "libc/sysv/consts/fileno.h"
//#include "libc/sysv/consts/o.h"
//#include "libc/sysv/consts/sig.h"
//#include "libc/sysv/consts/termios.h"
//#include "libc/x/x.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <errno.h>
//#include <ctype.h>
#include <signal.h>
//#include <locale.h>
#include "runes.h"
#include "unikey.h"

typedef int bool;
#define false 0
#define true  1

#undef CTRL
#define CTRL(C)                ((C) ^ 0b01000000)

#define WRITE(FD, SLIT)             write(FD, SLIT, strlen(SLIT))
#define PROBE_DISPLAY_SIZE          "\e7\e[9979;9979H\e[6n\e8"

bool startswith(const char *s, const char *prefix) {
  for (;;) {
    if (!*prefix) return true;
    if (!*s) return false;
    if (*s++ != *prefix++) return false;
  }
}

bool endswith(const char *s, const char *suffix) {
  size_t n, m;
  n = strlen(s);
  m = strlen(suffix);
  if (m > n) return false;
  return !memcmp(s + n - m, suffix, m);
}

/**
 * Returns dimensions of controlling terminal.
 *
 * It is recommended that programs err on the side of showing more
 * information, if this value can't be obtained with certainty.
 *
 * @param out stores determined dimensions, only on success
 * @returns -1 on error or something else on success
 */
int getttysize(int fd, struct winsize *out) {
  //if (__nocolor) {
    //out->ws_col = strtoimax(firstnonnull(getenv("COLUMNS"), "80"), NULL, 0);
    //out->ws_row = strtoimax(firstnonnull(getenv("ROWS"), "40"), NULL, 0);
    //out->ws_xpixel = 0;
    //out->ws_ypixel = 0;
    //return 0;
  //} else {
    return ioctl(fd, TIOCGWINSZ, out);  //FIXME ELKS
  //}
}

char code[512];
struct winsize wsize;
volatile bool resized, killed;

void onresize(int sig) {
  resized = true;
}

void onkilled(int sig) {
  killed = true;
  exit(1);
}

void getsize(void) {
  if (getttysize(1, &wsize) != -1) {
    printf("termios says terminal size is %hu×%hu\r\n", wsize.ws_col,
           wsize.ws_row);
  } else {
    printf("gettysize/ioctl TIOCGWINSZ fail\r\n");
  }
}

int main(int argc, char *argv[]) {
  int e, y, x, n, k, yn, xn;

  tty_init(FullMouseTracking|CatchISig|Utf8);
  signal(SIGTERM, onkilled);
  signal(SIGINT, onkilled);
  signal(SIGWINCH, onresize);
  signal(SIGCONT, onresize);
  
  getsize();
  WRITE(1, PROBE_DISPLAY_SIZE);
again:
  while (!killed) {
    if (resized) {
      printf("SIGWINCH ");
      getsize();
      resized = false;
    }
    if ((n = readansi(0, code, sizeof(code))) == -1) {
      if (errno == EINTR) continue;
      printf("ERROR: READ: %s\r\n", strerror(errno));
      exit(1);
    }
    e = ansi_to_unikey(code, n);
    if (e != -1 && e >= kKeyFirst) {
        if (unikeyname(e))
            printf("%s\r\n", unikeyname(e));
        else printf("kUnknown<%04x>\r\n", e);
        goto again;
    }
    //printf("%`'.*s ", n, code);
    e = 0;
    while (e < n && code[e] == 27) {
        printf("ESC ");
        e++;
    }
    if (n > e) {
        if (code[e] < ' ')
            printf("^%c ", CTRL(code[e]));
        else printf("%.*s ", n-e, code+e);
    }
    if (code[0] == CTRL('C') || code[0] == CTRL('D')) break;
    //setlocale(LC_ALL, "");
    if (code[0] == CTRL('A')) {
        int i;
        char buf[16];
        FILE *fp = fopen("unitestsh.txt", "r");

        printf("\r\n");
        while ((i = getc(fp)) != EOF) {
            int r = stream_to_rune(i & 255);
            //if (r > 0)
            if (r && r != -1) {
                if (runetostr(buf, r))
                    printf("%s", buf);
                if (r == '\n') printf("\r");
            }
        }
        fclose(fp);
#if 1
        printf("\r\n");
        for (i=128; i<512; i++) {
            //printf("%lc ", i);
            if (runetostr(buf, i))
                printf("%s ", buf);
        }
#endif
    }
    if (ansi_dsr(code, n, &yn, &xn) > 0) {
        printf("inband signalling says terminal size is %d×%d\r\n", xn, yn);
    } else {
        n = ansi_to_unimouse(code, n, &x, &y, &k, &e);
        if (n != -1) {
            printf("mouse %s at %dx%d, %s", describemouseevent(e), x, y,
                unikeyname(n));
            if (e) {
                if (k & kCtrl)  printf("+kCtrl");
                if (k & kAlt)   printf("+kAlt");
                if (k & kShift) printf("+kShift");
            }
        }
        printf("\r\n");
    }
  }
  return 0;
}
