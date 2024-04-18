/* shared console routines for Direct and BIOS consoles - #include in console drivers*/

static void WriteChar(register Console * C, int c)
{
    /* check for graphics lock */
    while (glock) {
        if (glock == C)
            return;
        sleep_on(&glock_wait);
    }
    C->fsm(C, c);
}

void Console_conout(dev_t dev, int Ch)
{
    Console *C = &Con[MINOR(dev)];

    if (Ch == '\n')
        WriteChar(C, '\r');
    WriteChar(C, Ch);
    PositionCursor(C);
}

void Console_conin(unsigned char Key)
{
    register struct tty *ttyp = &ttys[Current_VCminor];

    if (!tty_intcheck(ttyp, Key))
        chq_addch(&ttyp->inq, Key);
}

#ifdef CONFIG_EMUL_ANSI
static void Console_gotoxy(register Console * C, int x, int y)
{
    register int xp = x;

    C->cx = (xp >= MaxCol) ? MaxCol : (xp < 0) ? 0 : xp;
    xp = y;
    C->cy = (xp >= MaxRow) ? MaxRow : (xp < 0) ? 0 : xp;
    C->XN = 0;
}

static int parm1(register unsigned char *buf)
{
    int n;

    if (!(n = atoi((char *)buf)))
        n = 1;
    return n;
}

static int parm2(register unsigned char *buf)
{
    while (*buf != ';' && *buf)
        buf++;
    if (*buf)
        buf++;
    return parm1(buf);
}

static void itoaQueue(int i)
{
   unsigned char a[6];
   unsigned char *b = a + sizeof(a) - 1;

   *b = 0;
   do {
      *--b = '0' + (i % 10);
      i /= 10;
   } while (i);
   while (*b)
        Console_conin(*b++);
}

/* reverse map table ANSI -> ega      blk red grn yel blu mag cyn wht */
static unsigned char ega_color[16] = {  0,  4,  2,  6,  1,  5,  3,  7,
                                        8, 12, 10, 14,  9, 13, 11, 15 };

/* ESC [ processing */
static void AnsiCmd(register Console * C, int c)
{
    int n;

    /* ANSI param gathering and processing */
    if (C->parmptr < &C->params[MAXPARMS - 1])
        *C->parmptr++ = (unsigned char) c;
    if (!isalpha(c)) {
        return;
    }
    *C->parmptr = '\0';

    switch (c) {
    case 's':                   /* Save the current location */
        C->savex = C->cx;
        C->savey = C->cy;
        break;
    case 'u':                   /* Restore saved location */
        C->cx = C->savex;
        C->cy = C->savey;
        break;
    case 'A':                   /* Move up n lines */
        C->cy -= parm1(C->params);
        if (C->cy < 0)
            C->cy = 0;
        break;
    case 'B':                   /* Move down n lines */
        C->cy += parm1(C->params);
        if (C->cy > MaxRow)
            C->cy = MaxRow;
        break;
    case 'd':                   /* Vertical position absolute */
        C->cy = parm1(C->params) - 1;
        if (C->cy > MaxRow)
            C->cy = MaxRow;
        break;
    case 'C':                   /* Move right n characters */
        C->cx += parm1(C->params);
        if (C->cx > MaxCol)
            C->cx = MaxCol;
        break;
    case 'G':                   /* Horizontal position absolute */
        C->cx = parm1(C->params) - 1;
        if (C->cx > MaxCol)
            C->cx = MaxCol;
        break;
    case 'D':                   /* Move left n characters */
        C->cx -= parm1(C->params);
        if (C->cx < 0)
            C->cx = 0;
        break;
    case 'H':                   /* cursor move */
        Console_gotoxy(C, parm2(C->params) - 1, parm1(C->params) - 1);
        break;
    case 'J':                   /* clear screen */
        n = atoi((char *)C->params);
        if (n == 0) {           /* to bottom */
            ClearRange(C, C->cx, C->cy, MaxCol, C->cy);
            if (C->cy < MaxRow)
                ClearRange(C, 0, C->cy, MaxCol, MaxRow);
        } else if (n == 2)      /* all*/
            ClearRange(C, 0, 0, MaxCol, MaxRow);
        break;
    case 'K':                   /* clear line */
        n = atoi((char *)C->params);
        if (n == 0)             /* to EOL */
            ClearRange(C, C->cx, C->cy, MaxCol, C->cy);
        else if (n == 1)        /* to BOL */
            ClearRange(C, 0, C->cy, C->cx, C->cy);
        else if (n == 2)        /* all */
            ClearRange(C, 0, C->cy, MaxCol, C->cy);
        break;
    case 'L':                   /* insert line */
        ScrollDown(C, C->cy);
        break;
    case 'M':                   /* remove line */
        ScrollUp(C, C->cy);
        break;
    case 'm':                   /* ansi color */
      {
        char *p = (char *)C->params;
        do {
          n = atoi(p);
          if (n >= 30 && n <= 37) {             /* set fg color */
            C->attr &= 0xf0;                    /* don't save bright bit */
            C->attr |= ega_color[n-30];
          }
          else if (n >= 40 && n <= 47) {
            C->attr &= 0x8f;                    /* save blink bit */
            C->attr |= ega_color[n-40] << 4;
          }
          else if (n >= 90 && n <= 97) {        /* set bright fg color */
            C->attr &= 0xf0;                    /* don't save bright bit */
            C->attr |= ega_color[n-90+8];
          }
          else switch (n) {
            case 1:
                C->attr |= A_BOLD;
                break;
            case 4:
                n = C->attr;
                C->attr &= ~(A_DEFAULT | A_REVERSE);
                C->attr |= A_UNDERLINE;
                break;
            case 5:
                C->attr |= A_BLINK;
                break;
            case 7:
                n = C->attr;
                C->attr &= ~(A_DEFAULT | A_REVERSE);
                C->attr |= ((n >> 4) & 0x07) | ((n << 4) & 0x70);
                break;
            case 8:
                C->attr = A_BLANK;
                break;
            case 39:
            case 49:
            case 0:
            default:
                C->attr = A_DEFAULT;
                break;
          }
          while (*p >= '0' && *p <= '9')
            p++;
        } while (*p++ == ';');
      }
      break;
    case 'n':
        if (parm1(C->params) == 6) {            /* device status report*/
            //FIXME sequence can be interrupted by kbd input
            Console_conin(ESC);
            Console_conin('[');
            itoaQueue(C->cy+1);
            Console_conin(';');
            itoaQueue(C->cx+1);
            Console_conin('R');
        }
        break;
    case 'h':                   /* cursor on */
    case 'l':                   /* cursor off */
        if (C->params[0] == '?' && atoi((const char *)C->params+1) == 25) {
            DisplayCursor(c == 'h');
        }
        break;
    }
    C->fsm = std_char;
}

/* ANSI emulator - ESC seen */
static void esc_char(register Console * C, int c)
{
    /* Parse CSI sequence */
    C->parmptr = C->params;
    switch (c) {
    case '[':
        C->fsm = AnsiCmd;
        return;
    case '7':           /* DEC save cursor pos */
        C->savex = C->cx;
        C->savey = C->cy;
        break;
    case '8':           /* DEC restore cursor pos */
        C->cx = C->savex;
        C->cy = C->savey;
        break;
    }
    C->fsm = std_char;
}
#endif

/* Normal character processing */
static void std_char(register Console * C, int c)
{
    switch(c) {
    case BEL:
        bell();
        break;
    case '\b':
        C->XN = 0;
        if (C->cx > 0) {
            --C->cx;
#ifdef CONFIG_CONSOLE_BIOS
            PositionCursor(C);
#endif
            VideoWrite(C, ' ');
        }
        break;
    case '\t':
        C->cx = (C->cx | 0x07) + 1;
        goto linewrap;
    case '\n':
        C->XN = 0;
        ++C->cy;
        break;
    case '\r':
        C->XN = 0;
        C->cx = 0;
        break;

#ifdef CONFIG_EMUL_ANSI
    case ESC:
        C->fsm = esc_char;
        break;
#endif

    default:
        if (C->XN) {
            C->XN = 0;
            C->cx = 0;
            C->cy++;
            if (C->cy > MaxRow) {
                ScrollUp(C, 0);
                C->cy = MaxRow;
            }
        }
#ifdef CONFIG_CONSOLE_BIOS
        PositionCursor(C);
#endif
        if (c != 0) {
                VideoWrite(C, c);
                C->cx++;
        }
      linewrap:
        if (C->cx > MaxCol) {
            C->XN = 1;
            C->cx = MaxCol;
        }
    }
    if (C->cy > MaxRow) {
        ScrollUp(C, 0);
        C->cy = MaxRow;
    }
}

static int Console_ioctl(struct tty *tty, int cmd, char *arg)
{
    register Console *C = &Con[tty->minor];

    switch (cmd) {
    case DCGET_GRAPH:
        if (!glock) {
            glock = C;
            return 0;
        }
        return -EBUSY;
    case DCREL_GRAPH:
        if (glock == C) {
            glock = NULL;
            wake_up(&glock_wait);
            return 0;
        }
        break;
    case DCSET_KRAW:
        if (glock == C) {
            kraw = 1;
            return 0;
        }
        break;
    case DCREL_KRAW:
        if ((glock == C) && (kraw == 1)) {
            kraw = 0;
            return 1;
        }
        break;
    case KIOCSOUND:
        {
            unsigned period = (unsigned) arg;
            if (period)
                soundp(period);
            else
                nosound();
        }
        return 0;
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        return 0;
    }

    return -EINVAL;
}

static int Console_write(register struct tty *tty)
{
    register Console *C = &Con[tty->minor];
    int cnt = 0;

    while ((tty->outq.len > 0) && !glock) {
        WriteChar(C, tty_outproc(tty));
        cnt++;
    }
    if (C == Visible)
        PositionCursor(C);
    return cnt;
}

static void Console_release(struct tty *tty)
{
    ttystd_release(tty);
}

static int Console_open(register struct tty *tty)
{
    if ((int)tty->minor >= NumConsoles)
        return -ENODEV;
    return ttystd_open(tty);
}
