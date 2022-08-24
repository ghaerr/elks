/*
 * ANSI to Unicode keyboard mapping
 * Maps VT and ANSI keyboard sequences to unicode private use range.
 *
 * July 2022 Greg Haerr
 */
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include "unikey.h"

#define ANSI_UTF8       0       /* =1 to decode UTF-9 in readansi() */
#define CHECK_MOUSE     1       /* =1 to validate ANSI mouse sequences */
#define DEBUG           1       /* =1 for keyname() */
#define ESC             27
#define unreachable

static char scroll_reverse = 0; /* report reversed scroll wheel direction */
int kDoubleClickTime = 200;

#if DEBUG
#pragma GCC diagnostic ignored "-Wmissing-braces"
struct unikeyname unikeynames[] = {
    kBackSpace,         "kBackSpace",
    kTab,               "kTab",
    kEscKey,            "kEscKey",
    kDel,               "kDel",
    kUpArrow,           "kUpArrow",
    kDownArrow,         "kDownArrow",
    kLeftArrow,         "kLeftArrow",
    kRightArrow,        "kRightArrow",
    kInsert,            "kInsert",
    kDelete,            "kDelete",
    kHome,              "kHome",
    kEnd,               "kEnd",
    kPageUp,            "kPageUp",
    kPageDown,          "kPageDown",
    kMouseLeftDown,     "kMouseLeftDown",
    kMouseMiddleDown,   "kMouseMiddleDown",
    kMouseRightDown,    "kMouseRightDown",
    kMouseLeftUp,       "kMouseLeftUp",
    kMouseMiddleUp,     "kMouseMiddleUp",
    kMouseRightUp,      "kMouseRightUp",
    kMouseLeftDrag,     "kMouseLeftDrag",
    kMouseMiddleDrag,   "kMouseMiddleDrag",
    kMouseRightDrag,    "kMouseRightDrag",
    kMouseWheelUp,      "kMouseWheelUp",
    kMouseWheelDown,    "kMouseWheelDown",
    kMouseMotion,       "kMouseMotion",
    kMouseLeftDoubleClick,"kMouseLeftDoubleClick",
    kF1,                "kF1",
    kF2,                "kF2",
    kF3,                "kF3",
    kF4,                "kF4",
    kF5,                "kF5",
    kF6,                "kF6",
    kF7,                "kF7",
    kF8,                "kF8",
    kF9,                "kF9",
    kF10,               "kF10",
    kF11,               "kF11",
    kF12,               "kF12",
    kCtrlF1,            "kCtrlF1",
    kCtrlF2,            "kCtrlF2",
    kCtrlF3,            "kCtrlF3",
    kCtrlF4,            "kCtrlF4",
    kCtrlF5,            "kCtrlF5",
    kCtrlF6,            "kCtrlF6",
    kCtrlF7,            "kCtrlF7",
    kCtrlF8,            "kCtrlF8",
    kCtrlF9,            "kCtrlF9",
    kCtrlF10,           "kCtrlF10",
    kCtrlF11,           "kCtrlF11",
    kCtrlF12,           "kCtrlF12",
    kAltF1,             "kAltF1",
    kAltF2,             "kAltF2",
    kAltF3,             "kAltF3",
    kAltF4,             "kAltF4",
    kAltF5,             "kAltF5",
    kAltF6,             "kAltF6",
    kAltF7,             "kAltF7",
    kAltF8,             "kAltF8",
    kAltF9,             "kAltF9",
    kAltF10,            "kAltF10",
    kAltF11,            "kAltF11",
    kAltF12,            "kAltF12",
    kAltA,              "kAltA",
    kAltB,              "kAltB",
    kAltC,              "kAltC",
    kAltD,              "kAltD",
    kAltE,              "kAltE",
    kAltF,              "kAltF",
    kAltG,              "kAltG",
    kAltH,              "kAltH",
    kAltI,              "kAltI",
    kAltJ,              "kAltJ",
    kAltK,              "kAltK",
    kAltL,              "kAltL",
    kAltM,              "kAltM",
    kAltN,              "kAltN",
    kAltO,              "kAltO",
    kAltP,              "kAltP",
    kAltQ,              "kAltQ",
    kAltR,              "kAltR",
    kAltS,              "kAltS",
    kAltT,              "kAltT",
    kAltU,              "kAltU",
    kAltV,              "kAltV",
    kAltW,              "kAltW",
    kAltX,              "kAltX",
    kAltY,              "kAltY",
    kAltZ,              "kAltZ",
    kAlt0,              "kAlt0",
    kAlt1,              "kAlt1",
    kAlt2,              "kAlt2",
    kAlt3,              "kAlt3",
    kAlt4,              "kAlt4",
    kAlt5,              "kAlt5",
    kAlt6,              "kAlt6",
    kAlt7,              "kAlt7",
    kAlt8,              "kAlt8",
    kAlt9,              "kAlt9",
    0, 0
};

char *unikeyname(int k)
{
    int i;
    static char name[24];

    for (i = 0; unikeynames[i].key; i++) {
        if (unikeynames[i].key == k) {
            strcpy(name, unikeynames[i].name);
            return name;
        }
    }
    return NULL;
}
#endif

static int small_atoi(const char *s)
{
    int n = 0;

    while (*s == ' ' || *s == '\t')
        s++;
    while ((unsigned) (*s - '0') <= 9u)
        n = n * 10 + *s++ - '0';
    return n;
}

/*
 * Check and convert from single ANSI or UTF-8 keyboard sequence
 * to unicode key or char value, including single byte ASCII values
 * (0x00 <= ASCII <= 0x7f).
 * Will also decode UTF-8 into single UCS-2 character.
 * Returns -1 if not ASCII or ANSI keyboard sequence (e.g. mouse or DSR).
 */
int ansi_to_unikey(char *buf, int n)
{
    if (n == 0)
        return -1;              /* invalid conversion */
    if (n == 1) {
        int c = buf[0] & 255;       /* ASCII range */
        return (c == 127)? 8: c;    /* map DEL -> BS */
    }
    if (buf[0] == ESC) {
        if (buf[1] == '[') {
            if (n == 3) {                           /* xterm sequences */
                switch (buf[2]) {                   /* ESC [ A etc */
                case 'A':   return kUpArrow;
                case 'B':   return kDownArrow;
                case 'C':   return kRightArrow;
                case 'D':   return kLeftArrow;
                case 'F':   return kEnd;
                case 'H':   return kHome;
                }
            } else if (n == 4 && buf[2] == '1') {   /* ESC [ 1 P etc */
                switch (buf[3]) {
                case 'P':   return kF1;
                case 'Q':   return kF2;
                case 'R':   return kF3;
                case 'S':   return kF4;
                }
            }
            if (n > 3 && buf[n-1] == '~') {         /* vt sequences */
                switch (small_atoi(buf+2)) {
                case 1:     return kHome;
                case 2:     return kInsert;
                case 3:     return kDelete;
                case 4:     return kEnd;
                case 5:     return kPageUp;
                case 6:     return kPageDown;
                case 7:     return kHome;
                case 8:     return kEnd;
                case 11:    return kF1;
                case 12:    return kF2;
                case 13:    return kF3;
                case 14:    return kF4;
                case 15:    return kF5;
                case 17:    return kF6;
                case 18:    return kF7;
                case 19:    return kF8;
                case 20:    return kF9;
                case 21:    return kF10;
                case 23:    return kF11;
                case 24:    return kF12;
                }
            }
        }

        /* allow multi-keystroke sequences using ESC + character -> Alt/Fn key */
        if (n == 2) {
            if (buf[1] >= 'a' && buf[1] <= 'z')     /* ESC {a-z} -> ALT-{A-Z} */
                return buf[1] - 'a' + kAltA;

            if (buf[1] >= '1' && buf[1] <= '9')     /* ESC {1-9} -> Fn{1-9} */
                return buf[1] - '1' + kF1;
            if (buf[1] == '0')
                return kF10;
        }
    }
    return -1;
}

/* state machine conversion of UTF-8 byte sequence to Rune */
/* returns Rune or 0 on no data yet or -1 on invalid data */
int stream_to_rune(unsigned int ch)
{
    static enum { kAscii, kUtf8 } state = kAscii;
    static int a, n;

    switch (state) {
    case kAscii:
    default:
        if (ch >= 0300) {
            a = ThomPikeByte(ch);
            n = ThomPikeLen(ch) - 1;
            state = kUtf8;
            return 0;       /* no data yet */
        }
        break;
    case kUtf8:
        if (!ThomPikeCont(ch)) {
            state = kAscii;
            return -1;      /* bad data */
        }
        a = ThomPikeMerge(a, ch);
        if (!--n) {
            state = kAscii;
            return a;
        }
        return 0;           /* no data yet */
    }

    return ch;
}

/*-*- mode:c;indent-tabs-mode:nil;c-basic-offset:2;tab-width:8;coding:utf-8 -*-│
│vi: set net ft=c ts=2 sts=2 sw=2 fenc=utf-8                                :vi│
╞══════════════════════════════════════════════════════════════════════════════╡
│ Copyright 2020 Justine Alexandra Roberts Tunney                              │
│                                                                              │
│ Permission to use, copy, modify, and/or distribute this software for         │
│ any purpose with or without fee is hereby granted, provided that the         │
│ above copyright notice and this permission notice appear in all copies.      │
│                                                                              │
│ THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL                │
│ WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED                │
│ WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE             │
│ AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL         │
│ DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR        │
│ PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER               │
│ TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR             │
│ PERFORMANCE OF THIS SOFTWARE.                                                │
╚─────────────────────────────────────────────────────────────────────────────*/
//#include "libc/calls/calls.h"
//#include "libc/calls/internal.h"
//#include "libc/str/thompike.h"
//#include "libc/sysv/errfuns.h"

/**
 * Reads single keystroke or control sequence from character device.
 *
 * When reading ANSI UTF-8 text streams, characters and control codes
 * are oftentimes encoded as multi-byte sequences. This function knows
 * how long each sequence is, so that each read consumes a single thing
 * from the underlying file descriptor, e.g.
 *
 *     "a"               ALFA
 *     "\316\261"        ALPHA
 *     "\e[38;5;202m"    ORANGERED
 *     "\e[A"            UP
 *     "\e\e[A"          ALT-UP
 *     "\001"            CTRL-ALFA
 *     "\e\001"          ALT-CTRL-ALFA
 *     "\eOP"            PF1
 *     "\000"            NUL
 *     "\e]rm -rf /\e\\" OSC
 *     "\302\233A"       UP
 *     "\300\200"        NUL
 *
 * This routine generalizes to ascii, utf-8, chorded modifier keys,
 * function keys, color codes, c0/c1 control codes, cursor movement,
 * mouse movement, etc.
 *
 * Userspace buffering isn't required, since ANSI escape sequences and
 * UTF-8 are decoded without peeking. Noncanonical overlong encodings
 * can cause the stream to go out of sync. This function recovers such
 * events by ignoring continuation bytes at the beginning of each read.
 *
 * @param p is guaranteed to receive a NUL terminator if n>0
 * @return number of bytes read (helps differentiate "\0" vs. "")
 * @see examples/ttyinfo.c
 * @see ANSI X3.64-1979
 * @see ISO/IEC 6429
 * @see FIPS-86
 * @see ECMA-48
 */
int readansi(int fd, char *p, int n) {
  //wint_t x = 0;
  int rc;
  int e, i, j;
  unsigned char c;
  enum { kAscii, kUtf8, kEsc, kCsi1, kCsi2, kSs, kNf, kStr, kStr2, kDone } t;
  e = errno;
  t = kAscii;
  i = j = 0;
  if (n) p[0] = 0;
  do {
    for (;;) {
      if (n) {
        /* TODO: possibly allow timeout after single ESC to return ESC */
        rc = read(fd, &c, 1);
      } else {
        errno = EFAULT; //read(fd, 0, 0);
        rc = -1;
      }
      if (rc == -1 && errno == EINTR) {
        if (!i) {
          return -1;
        }
      } else if (rc == -1) {
        return -1;
      } else if (!rc) {
        if (!i) {
          errno = e;
          return 0;
        } else {
          errno = EILSEQ;
          return -1;
        }
      } else {
        break;
      }
    }
    if (i + 1 < n) {
      p[i] = c;
      p[i + 1] = 0;
    } else if (i < n) {
      p[i] = 0;
    }
    ++i;
    switch (t) {
    Whoopsie:
      if (n) p[0] = c;
      t = kAscii;
      i = 1;
        /* fallthrough */
      case kAscii:
        if (c < 0200) {
          if (c == '\e') {
            t = kEsc;
          } else {
            t = kDone;
          }
        } else if (c >= 0300) {
#if ANSI_UTF8
          t = kUtf8;
          x = ThomPikeByte(c);
          j = ThomPikeLen(c) - 1;
#else
          t = kDone;
#endif
        } else {
          /* ignore overlong sequences */
        }
        break;
#if ANSI_UTF8
      case kUtf8:
        if ((c & 0300) == 0200) {
          x = ThomPikeMerge(x, c);
          if (!--j) {
            switch (x) {
              case '\e':
                t = kEsc; /* parsed but not canonicalized */
                break;
              case 0x9b:
                t = kCsi1; /* unusual but legal */
                break;
              case 0x8e:
              case 0x8f:
                t = kSs; /* unusual but legal */
                break;
              case 0x90: /* DCS (Device Control String) */
              case 0x98: /* SOS (Start of String) */
              case 0x9d: /* OSC (Operating System Command) */
              case 0x9e: /* PM  (Privacy Message) */
              case 0x9f: /* APC (Application Program Command) */
                t = kStr;
                break;
              default:
                t = kDone;
                break;
            }
          }
        } else {
          goto Whoopsie; /* ignore underlong sequences if not eof */
        }
        break;
#endif
      case kEsc:
        if (0x20 <= c && c <= 0x2f) { /* Nf */
          /*
           * Almost no one uses ANSI Nf sequences
           * They overlaps with alt+graphic keystrokes
           * We care more about being able to type alt-/
           */
          if (c == ' ' || c == '#') {
            t = kNf;
          } else {
            t = kDone;
          }
        } else if (0x30 <= c && c <= 0x3f) { /* Fp */
          t = kDone;
        } else if (0x20 <= c && c <= 0x5F) { /* Fe */
          switch (c) {
            case '[':
              t = kCsi1;
              break;
            case 'N': /* SS2 */
            case 'O': /* SS3 */
              t = kSs;
              break;
            case 'P': /* DCS (Device Control String) */
            case 'X': /* SOS (Start of String) */
            case ']': /* DCS (Operating System Command) */
            case '^': /* PM  (Privacy Message) */
            case '_': /* DCS (Application Program Command) */
              t = kStr;
              break;
            default:
              t = kDone;
              break;
          }
        } else if (0x60 <= c && c <= 0x7e) { /* Fs */
          t = kDone;
        } else if (c == '\e') {
          if (i < 3) {
            t = kEsc; /* alt chording */
          } else {
            t = kDone; /* esc mashing */
            i = 1;
          }
        } else {
          t = kDone;
        }
        break;
      case kSs:
        t = kDone;
        break;
      case kNf:
        if (0x30 <= c && c <= 0x7e) {
          t = kDone;
        } else if (!(0x20 <= c && c <= 0x2f)) {
          goto Whoopsie;
        }
        break;
      case kCsi1:
        if (0x20 <= c && c <= 0x2f) {
          t = kCsi2;
        } else if (c == '[' && (i == 3 || (i == 4 && p[1] == '\e'))) {
          /* linux function keys */
        } else if (0x40 <= c && c <= 0x7e) {
          t = kDone;
        } else if (!(0x30 <= c && c <= 0x3f)) {
          goto Whoopsie;
        }
        break;
      case kCsi2:
        if (0x40 <= c && c <= 0x7e) {
          t = kDone;
        } else if (!(0x20 <= c && c <= 0x2f)) {
          goto Whoopsie;
        }
        break;
      case kStr:
        switch (c) {
          case '\a':
            t = kDone;
            break;
          case '\e': /* ESC */
          case 0302: /* C1 (UTF-8) */
            t = kStr2;
            break;
          default:
            break;
        }
        break;
      case kStr2:
        switch (c) {
          case '\a':
            t = kDone;
            break;
          case '\\': /* ST (ASCII) */
          case 0234: /* ST (UTF-8) */
            t = kDone;
            break;
          default:
            t = kStr;
            break;
        }
        break;
      default:
        unreachable;
    }
  } while (t != kDone);
  errno = e;
  return i;
}

static int startswith(const char *s, const char *prefix) {
  for (;;) {
    if (!*prefix) return 1;
    if (!*s) return 0;
    if (*s++ != *prefix++) return 0;
  }
}

static int getparm(char *buf, int n)
{
    while (n-- > 0) {
        while (*buf != ';' && *buf)
                buf++;
        if (*buf)
                buf++;
    }
    return small_atoi(buf);
}

/* xterm ansi mouse sequence status bits ESC [ < status;y;x {M|m} */
#define ANSI_MOUSE_BUTTON_MASK   0x03
#define ANSI_MOUSE_BUTTON_L      0x00
#define ANSI_MOUSE_BUTTON_M      0x01
#define ANSI_MOUSE_BUTTON_R      0x02
#define AMSI_MOUSE_MOD_MASK      0x1c
#define ANSI_MOUSE_SHIFT         0x04
#define ANSI_MOUSE_ALT           0x08
#define ANSI_MOUSE_CTRL          0x10
#define ANSI_MOUSE_MOTION        0x20
#define ANSI_MOUSE_WHEEL_MASK    0x40
#define AMSI_MOUSE_WHEEL_DOWN    0x01   /* otherwise up */
#define ANSI_MOUSE_BUTTON_UP     0x80   /* otherwise down */

const char *describemouseevent(int e) {
  static char buf[32];
  sprintf(buf, "%02x", e);
  if (e & 0x10) strcat(buf, " ctrl");
  if (e & 0x08) strcat(buf, " alt");
  if (e & 0x04) strcat(buf, " shift");
  if (e & 0x40) {
    strcat(buf, " wheel");
    if (e & 0x01) {
      strcat(buf, " down");
    } else {
      strcat(buf, " up");
    }
  } else {
    switch (e & 3) {
      case 0:
        strcat(buf, " left");
        break;
      case 1:
        strcat(buf, " middle");
        break;
      case 2:
        strcat(buf, " right");
        break;
      default:
        break;
    }
    if (e & 0x20) {
      strcat(buf, " motion");
    } else if (e & 0x80) {
      strcat(buf, " up");
    } else {
      strcat(buf, " down");
    }
  }
  return buf;
}

/* convert ansi mouse event to unikey and modifiers */
static int mouse_to_unikey(int e, int *modkeys)
{
  int status = 0;
  int k;

  if (e & 0x10) status |= kCtrl;
  if (e & 0x08) status |= kAlt;
  if (e & 0x04) status |= kShift;
  if (e & 0x40) {           /* wheel */
    if (e & 0x01) {
      k = scroll_reverse? kMouseWheelUp: kMouseWheelDown;
    } else {
      k = scroll_reverse? kMouseWheelDown: kMouseWheelUp;
    }
  } else {
    if (e & 0x20) {         /* motion */
      e &= 3;
      k = !e? kMouseLeftDrag: e==1? kMouseMiddleDrag: e==2? kMouseRightDrag: kMouseMotion;
    } else if (e & 0x80) {  /* up */
      e &= 3;
      k = !e? kMouseLeftUp: e==1? kMouseMiddleUp: e==2? kMouseRightUp: 0;
    } else {                /* down */
      e &= 3;
      k = !e? kMouseLeftDown: e==1? kMouseMiddleDown: e==2? kMouseRightDown: 0;
    }
  }
  *modkeys = status;
  return k;
}

int isdigit(int c)
{
    return (unsigned) (c - '0') < 10u;
}

/*
 * Check and convert ANSI mouse sequence to unicode mouse event. 
 * Format: ESC [ < status;y;x {m|M}
 * Returns -1 if not ANSI mouse sequence.
 */
int ansi_to_unimouse(char *buf, int n, int *x, int *y, int *modkeys, int *status)
{
    char *p;
    int k;
    struct timeval t;
    long tick;
    static long lasttick;
    static int lastkey;

    if (!startswith(buf, "\e[<") || (buf[n-1] != 'm' && buf[n-1] != 'M'))
        return -1;
    p = buf + 3;
    if (!isdigit(*p)) return -1;

    *status = getparm(p, 0);
    *x = getparm(p, 1) - 1;
    *y = getparm(p, 2) - 1;

#if CHECK_MOUSE
    if (*status > 93) return -1;
    if (*x < 0) return -1;
    if (*y < 0) return -1;
    if (getparm(p, 3) != 0) return -1;
#endif

    *status |= (buf[n-1] == 'm') << 7;
    k = mouse_to_unikey(*status, modkeys);

    if (k == kMouseLeftDown || k == kMouseLeftUp) {
        gettimeofday(&t, NULL);
        tick = t.tv_sec * 1000 + t.tv_usec / 1000;
        if (k == kMouseLeftDown && lastkey == kMouseLeftUp
            && (tick - lasttick) < kDoubleClickTime) {
            k = kMouseLeftDoubleClick;
        }
        lasttick = tick;
    }
    lastkey = k;

    //printf("mouse %s at %d×%d, %s\r\n",describemouseevent(*status), *x, *y, keyname(k));
    return k;
}

/*
 * Check and respond to ANSI DSR (device status report).
 * Format: ESC [ rows; cols R
 * Returns -1 if not ANSI DSR.
 */
int ansi_dsr(char *buf, int n, int *cols, int *rows)
{
    char *p;

    if (!startswith(buf, "\e[") || buf[n-1] != 'R')
        return -1;
    p = buf + 2;
    *rows = getparm(p, 0);
    *cols = getparm(p, 1);
    //printf("DSR terminal size is %dx%d\r\n", *cols, *rows);
    return 1;
}

#define WRITE(FD, SLIT)             write(FD, SLIT, strlen(SLIT))
#define PROBE_DISPLAY_SIZE          "\e7\e[9979;9979H\e[6n\e8"

/* probe display size - only uses DSR for now, for ELKS/UNIX compatibility */
int tty_getsize(int *cols, int *rows)
{
    int n, x, y;
    char buf[32];

    WRITE(1, PROBE_DISPLAY_SIZE);
    if ((n = readansi(0, buf, sizeof(buf))) != -1) {
        if (ansi_dsr(buf, n, &x, &y) > 0) {
            *cols = x;
            *rows = y;
            //printf("inband signaling says terminal size is %d×%d\r\n", x, y);
            return 1;
        }
    }
    return -1;
}
