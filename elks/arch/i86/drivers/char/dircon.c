/**************************************
 * Direct video memory display driver *
 * Saku Airila 1996                   *
 **************************************/

#include <linuxmt/types.h>
#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/fs.h>
#include <linuxmt/major.h>
#include <linuxmt/sched.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#ifdef CONFIG_CONSOLE_DIRECT

/* public interface of dircon.c: */

/* void con_charout (char Ch); */
/* void Console_set_vc (int N); */
/* int Console_write (struct tty * tty); */
/* void Console_release (struct inode * inode, struct file * file); */
/* int Console_open (struct inode * inode, struct file * file); */
/* struct tty_ops dircon_ops; */
/* void init_console(void); */

/****************************************************
 * This should come from the users configuration... *
 ****************************************************/
/*
 * Actual number of virtual consoles may be less than this
 */
#define MAX_CONS 3

#define CONSOLE_NAME "console"

#define WAIT_MV1_PARM 1
#define WAIT_MV2_PARM 2
#define WAIT_P_PARM 3
#define WAIT_Q_PARM 4

#define MAX_ATTR 3

#define A_DEFAULT 0x07
#define A_BOLD 0x08
#define A_BLINK 0x80

/* The InUse field was being written, but not read so I commented all
 * references out. If you need the field, put it back in.
 */
typedef
struct ConsoleTag
{
   int cx, cy;
   int Ord;
#ifdef CONFIG_DCON_VT52
   int InCmd;
#ifdef CONFIG_DCON_ANSI
   int InAnsi;
   int AnsiLen;
   int sx, sy;  /* Saved values */
#endif
   int WaitParm, PTmp;
#endif
   unsigned Seg, Off;
#ifdef CONFIG_DCON_VT52
   unsigned char Attr;
#ifdef CONFIG_DCON_ANSI
   unsigned char AnsiStr[25];
#endif
#endif
/*   char InUse;*/
} Console;

static int Width, Height, MaxRow, MaxCol;
static int CCBase;
static int NumConsoles = MAX_CONS;
static unsigned VideoSeg, PageSize;
static Console Con[ MAX_CONS ];
static Console * Visible;

/* from keyboard.c */
extern int Current_VCminor;

#ifdef CONFIG_DCON_VT52
static unsigned AttrArry[] = {
   A_DEFAULT,
   A_BOLD,
   A_BLINK
   };
#endif

#ifdef CONFIG_DCON_ANSI
static unsigned colortable[] = {
  0,1,2,3,4,5,6,7
};
#endif

static void ScrollUp();
static void PositionCursor();
static void WriteChar();
static void ClearRange();
#ifdef CONFIG_DCON_VT52
static void ScrollDown();
static void CommandChar();
#ifdef CONFIG_DCON_ANSI
static void HandleAnsi();
#endif
static void WaitParameter();
static void Console_gotoxy();
#endif

extern int peekb();
extern int peekw();
extern void pokeb();
extern void pokew();
extern void outb();

extern int AddQueue(); /* From xt_key.c */
extern int GetQueue();

void con_charout( Ch )
char Ch;
{
   WriteChar( Visible, Ch );
   PositionCursor( Visible );
}

#define islower(c) (c>96&&c<123)   /* FIXME - Should be elsewhere */
#define isupper(c) (c>64&&c<91)
#define toupper(c) (islower(c)?(c)^0x20:(c))

void WriteChar( C, Ch )
register Console * C;
char Ch;
{
int CursorOfs;
#ifdef CONFIG_DCON_ANSI
    if( C->InAnsi )
    {
      C->AnsiStr[C->AnsiLen++]=Ch;
      if( isupper(Ch) || islower(Ch) )
      {
        C->AnsiStr[C->AnsiLen++]=Ch;
        C->AnsiStr[C->AnsiLen++]='\0';
        HandleAnsi(C,Ch);
        C->InCmd=C->InAnsi=0;
      }
      return;
    }
#endif
#ifdef CONFIG_DCON_VT52
    if( C->InCmd )
    {
#ifdef CONFIG_DCON_ANSI
      if( Ch == '[' )
      {
        C->AnsiLen = 0;
	C->InAnsi = 1;
        return;
      }
#endif
      CommandChar( C, Ch );
      return;
   }
   if( Ch == 27 )
   {
      C->InCmd = 1;
      return;
   }
#endif
   if( Ch == '\b' )
   {
      if( C->cx > 0 )
      {
         C->cx--;
         WriteChar( C, ' ' );
         C->cx--;
         PositionCursor( C );
      }
      return;
   }
   if( Ch == '\n' )
   {
      C->cy++;
      C->cx = 0;
   }
   else if( Ch == '\r' )
      C->cx = 0;
   else
   {
      CursorOfs = ( C->cx << 1 ) + ( ( Width * C->cy ) << 1 );
      pokeb( C->Seg, C->Off + CursorOfs, Ch );
#ifdef CONFIG_DCON_VT52
      pokeb( C->Seg, C->Off + CursorOfs + 1, C->Attr );
#else
      pokeb( C->Seg, C->Off + CursorOfs + 1, A_DEFAULT );
#endif
#if 0
      if (C == Visible)
      {
         pokeb( VideoSeg, CursorOfs, Ch );
#ifdef CONFIG_DCON_VT52
         pokeb( VideoSeg, CursorOfs + 1, C->Attr );
#else
         pokeb( VideoSeg, CursorOfs + 1, A_DEFAULT );
#endif
      }
#endif
      C->cx++;
   }
   if( C->cx > MaxCol )
#if 1 /* dodgy line wrap */
      C->cx = 0, C->cy++;
#else
      C->cx = MaxCol;
#endif
   if( C->cy >= Height )
   {
      ScrollUp( C, 1, Height );
      C->cy--;
   }
}

void ScrollUp( C, st, en )
register Console * C;
int st, en;
{
unsigned rdofs, wrofs;
int cnt;
   if( st >= en || st < 1 )
      return;
   rdofs = ( Width << 1 ) * st;
   wrofs = rdofs - ( Width << 1 );
   cnt = Width * ( en - st );
   far_memmove( C->Seg, C->Off + rdofs, C->Seg, C->Off + wrofs, cnt << 1 );
   en--;
   ClearRange( C, 0, en, Width, en );
}

#ifdef CONFIG_DCON_VT52
void ScrollDown( C, st, en )
register Console * C;
int st, en;
{
unsigned rdofs, wrofs;
int cnt;
   if( st > en || en > Height )
      return;
   rdofs = ( Width << 1 ) * st;
   wrofs = rdofs + ( Width << 1 );
   cnt = Width * ( en - st );
   far_memmove( C->Seg, C->Off + rdofs, C->Seg, C->Off + wrofs, cnt << 1 );
   ClearRange( C, 0, st, Width, st );
}
#endif

void PositionCursor( C )
register Console * C;
{
int Pos;
   if( C != Visible )
      return;
   Pos = C->cx + Width * C->cy + (C->Ord * PageSize >> 1);
   outb( 14, CCBase );
   outb( ( unsigned char )( ( Pos >> 8 ) & 0xFF ), CCBase + 1 );
   outb( 15, CCBase );
   outb( ( unsigned char )( Pos & 0xFF ), CCBase + 1 );
}

void ClearRange( C, x, y, xx, yy )
register Console * C;
int x, y, xx, yy;
{
unsigned st, en, ClrW, ofs;
   st = ( x << 1 ) + y * ( Width << 1 );
   en = ( xx << 1 ) + yy * ( Width << 1 );
   ClrW = A_DEFAULT << 8 + 0x20;
   for( ofs = st; ofs < en; ofs += 2 )
      pokew( C->Seg, C->Off + ofs, ClrW );
}

#ifdef CONFIG_DCON_VT52
void CommandChar( C, Ch )
register Console * C;
char Ch;
{
   if( C->WaitParm )
   {
      WaitParameter( C, Ch );
      return;
   }
   switch( Ch )
   {
      case 'H' : /* home */
         C->cx = C->cy = 0;
         break;
      case 'A' : /* up */
         if( C->cy )
            C->cy--;
         break;
      case 'B' : /* down */
         if( C->cy < MaxRow )
            C->cy++;
         break;
      case 'C' : /* right */
         if( C->cx < MaxCol )
            C->cx++;
         break;
      case 'D' : /* left */
         if( C->cx )
            C->cx--;
         break;
      case 'K' : /* clear to eol */
         ClearRange( C, C->cx, C->cy, Width, C->cy );
         break;
      case 'J' : /* clear to eoscreen */
         ClearRange( C, 0, 0, Width, Height );
         break;
      case 'I' : /* linefeed reverse */
         ScrollDown( C, 0, MaxRow );
         break;
#if 0
      case 'Z' : /* identify */
/* Problem here */
         AddQueue( 27 );
         AddQueue( 'Z' );
         break;
#endif
      case 'P' : /* NOT VT52. insert/delete line */
         C->WaitParm = WAIT_P_PARM;
         return;
      case 'Q' : /* NOT VT52. My own additions. Attributes... */
         C->WaitParm = WAIT_Q_PARM;
         return;
      case 'Y' : /* cursor move */
         C->WaitParm = WAIT_MV1_PARM;
         return;
   }
   C->InCmd = 0;
}

void WaitParameter( C, Ch )
register Console * C;
char Ch;
{
   switch( C->WaitParm )
   {
      case WAIT_MV1_PARM :
         C->PTmp = Ch - ' ';
         C->WaitParm = WAIT_MV2_PARM;
         return;
      case WAIT_MV2_PARM :
         Console_gotoxy( C, Ch - ' ', C->PTmp );
         break;
      case WAIT_P_PARM :
         switch( Ch )
         {
            case 'f' : /* insert line at crsr */
               ScrollDown( C, C->cy, MaxRow );
               break;
            case 'd' : /* delete line at crsr */
               ScrollUp( C, C->cy + 1, Height );
               break;
         }
         break;
      case WAIT_Q_PARM :
         Ch -= ' ';
         if( Ch > 0 && Ch < MAX_ATTR )
            C->Attr |= AttrArry[ Ch ];
         if( !Ch )
            C->Attr = A_DEFAULT;
         break;
      default :
         break;
   }
#ifdef CONFIG_DCON_VT52
   C->WaitParm = C->InCmd = 0;
#endif
}
#endif

#ifdef CONFIG_DCON_ANSI
void HandleAnsi( C, Ch )
register Console * C;
char Ch;
{
  int x,y,c,np;
  unsigned char num[5],*p;
  switch(Ch) {
    case 's': /* Save the current location */
              C->sx=C->cx;
              C->sy=C->cy;
              break;
    case 'u': /* Restore saved location */
              C->cx=C->sx;
              C->cy=C->sy;
              break;
    case 'K': /* Clear to EOL */
              ClearRange( C, C->cx, C->cy, Width, C->cy );
              break;
    case 'A': /* Move up n lines */
              y=atoi(C->AnsiStr);
              if( C->cy-y < 0 )
                C->cy=0;
              else
                C->cy-=y;
              break;
    case 'B': /* Move down n lines */
              y=atoi(C->AnsiStr);
              if( C->cy+y > MaxRow )
                C->cy=MaxRow;
              else
                C->cy+=y;
              break;
    case 'C': /* Move right n characters */
              x=atoi(C->AnsiStr);
              if( C->cx+x > MaxCol )
                C->cx=MaxCol;
              else
                C->cx+=x;
              break;
    case 'D': /* Move left n characters */
              x=atoi(C->AnsiStr);
              if( C->cx-x < 0 )
                C->cx=0;
              else
                C->cx-=x;
              break;
    case 'm': /* Now what we've all been waiting for.. COLOR
               * this case 'm' part is rewritten by MBit
               * the rest of the kicking ass ANSI stuff is written
               * by Davey...
               */
             
             p=C->AnsiStr;
             while (*p != 'm') {
                 if (*p == ';') {p++; continue;};
                 if (*p == '0') {C->Attr=A_DEFAULT; p++; continue;};
                 if (*p == '1') {C->Attr|=A_BOLD; p++; continue;};
                 if (*p == '5') {C->Attr|=A_BLINK; p++; continue;};
                 if (*p == '8') {C->Attr=0; p++; continue;};
                 if ((*p == '3') && (*(p+1) != ';' && *(p+1) != 'm')) {
                     p++;
                     switch (*p) {
                        case '0': C->Attr&=0xf8; C->Attr|=0x0; break;
                        case '1': C->Attr&=0xf8; C->Attr|=0x1; break;
                        case '2': C->Attr&=0xf8; C->Attr|=0x2; break;
                        case '3': C->Attr&=0xf8; C->Attr|=0x3; break;
                        case '4': C->Attr&=0xf8; C->Attr|=0x4; break;
                        case '5': C->Attr&=0xf8; C->Attr|=0x5; break;
                        case '6': C->Attr&=0xf8; C->Attr|=0x6; break;
                        case '7': C->Attr&=0xf8; C->Attr|=0x7; break;
                        default: C->Attr=A_DEFAULT; break;
                     }
                     p++;
                     continue;
                 }
                 if (*p == '3') {C->Attr=A_DEFAULT; break;};
                 if ((*p == '4') && (*(p+1) != ';' && *(p+1) != 'm')) {
                     p++;
                     switch (*p) {
                        case '0': C->Attr&=0x8f; C->Attr|=0x0; break;
                        case '1': C->Attr&=0x8f; C->Attr|=0x10; break;
                        case '2': C->Attr&=0x8f; C->Attr|=0x20; break;
                        case '3': C->Attr&=0x8f; C->Attr|=0x30; break;
                        case '4': C->Attr&=0x8f; C->Attr|=0x40; break;
                        case '5': C->Attr&=0x8f; C->Attr|=0x50; break;
                        case '6': C->Attr&=0x8f; C->Attr|=0x60; break;
                        case '7': C->Attr&=0x8f; C->Attr|=0x70; break;
                        default: C->Attr=A_DEFAULT; break;
                     }
                 p++;
                 continue;
                 }
                 C->Attr=A_DEFAULT;
                 break;
             }	
             /* C->Attr = 0x92; TEST should print blinking blue bg and green fg*/ 
  }
}
#endif

/* This also tells the keyboard driver which tty to direct it's output to...
 * CAUTION: It *WILL* break if the console driver doesn't get tty0-X. 
 */

void Console_set_vc( N )
int N;
{
  unsigned int offset;

  if( N < 0 || N >= NumConsoles )
    return;
  if( Visible == &Con[ N ] )
    return;
  Visible = &Con[ N ];
#if 0
  far_memmove(
	      Visible->Seg, Visible->Off,
	      VideoSeg, 0,
	      ( Width * Height ) << 1 );
#endif
  offset=N*PageSize>>1;
  outw((offset & 0xff00) | 0x0c, CCBase);
  outw(((offset & 0xff) << 8) | 0x0d, CCBase);
  PositionCursor( Visible );
  Current_VCminor = N;
}

#ifdef CONFIG_DCON_VT52
void Console_gotoxy( C, x, y )
register Console * C;
int x, y;
{
   if( x >= MaxCol )
      x = MaxCol;
   if( y >= MaxRow )
      y = MaxRow;
   if( x < 0 )
      x = 0;
   if( y < 0 )
      y = 0;
   C->cx = x;
   C->cy = y;
   PositionCursor( C );
}
#endif

int Console_write(tty)
register struct tty * tty;
{
	int cnt = 0, chi;
	unsigned char ch;
	Console * C = &Con[tty->minor];
	while (tty->outq.len != 0) { 
		chq_getch(&tty->outq, &ch, 0);
#if 0
		if (ch == '\n') WriteChar(C, '\r');
#endif
		WriteChar( C, ch);
		cnt++;
	};
	PositionCursor( C );
	return cnt;
}

void Console_release( inode, file )
struct inode *inode;
struct file *file;
{
#if 0
int minor = MINOR( inode->i_rdev );
   if( minor < NumConsoles )
      Con[ minor ].InUse = 0;
#endif
}

int Console_open( inode, file )
struct inode *inode;
struct file *file;
{
int minor = MINOR(inode->i_rdev);
   if( minor >= NumConsoles )
      return -ENODEV;

/*   Con[ minor ].InUse = 1; */
return 0;
}

struct tty_ops dircon_ops = {
	Console_open,
	Console_release, /* Do not remove this, or it crashes */
	Console_write,
	NULL,
	NULL,
};

void init_console()
{
   unsigned int i;

   MaxCol = ( Width = peekb( 0x40, 0x4a ) ) - 1;
   /* Trust this. Cga does not support peeking at 0x40:0x84. */
   MaxRow = ( Height = 25 ) - 1;
   CCBase = peekw( 0x40, 0x63 );
   PageSize = peekw( 0x40, 0x4C );
   if( peekb( 0x40, 0x49 ) == 7 ) {
     VideoSeg = 0xB000;
     NumConsoles = 1;
   }else
     VideoSeg = 0xb800;
 
   for( i = 0; i < NumConsoles; i++ )
   {
#ifdef CONFIG_DCON_VT52
      Con[ i ].cx = Con[ i ].cy = Con[ i ].InCmd = 0;
      Con[ i ].WaitParm = 0;
      Con[ i ].Attr = A_DEFAULT;
#else
      Con[ i ].cx = Con[ i ].cy = 0;
#endif
      Con[ i ].Seg = VideoSeg + ( PageSize >> 4 ) * i;
      Con[ i ].Off = 0;
      Con[ i ].Ord = i;
/*      Con[ i ].InUse = 0; */
      if (i != 0) {
        ClearRange( &Con[ i ], 0, 0, Width, Height );
      }
   }
   Con[ 0 ].cx = peekb( 0x40, 0x50 );
   Con[ 0 ].cy = peekb( 0x40, 0x51 );
/*   Con[ 0 ].InUse = 1; */
   Visible = &Con[ 0 ];
#ifdef CONFIG_DCON_VT52
   printk(
         "Console: Direct %dx%d emulating vt52 (%d virtual consoles)\n",
         Width,
         Height,
         NumConsoles );
#else
   printk(
         "Console: Direct %dx%d dumb (%d virtual consoles)\n",
         Width,
         Height,
         NumConsoles );
#endif
}

#endif

