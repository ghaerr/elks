/*
 *	Tables from the Minix book, as thats all I have on XT keyboard controllers
 *	They need be loadable, this doesn't look good on my finnish kbd.
 */

/***************************************************************
 * Added primitive buffering, and function stubs for vfs calls *
 * Removed vfs funcs, they belong better to the console driver *
 * Saku Airila 1996                                            *
 ***************************************************************/

/***************************************************************
* Changed code to work with belgian keyboard                   *
* Stefaan (Stefke) Van Dooren 1998                             *
****************************************************************/

#include <linuxmt/sched.h>
#include <linuxmt/types.h>
#include <arch/io.h>
#include <arch/keyboard.h>
#include <linuxmt/errno.h>
#include <linuxmt/fs.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/config.h>
#include <linuxmt/chqueue.h>
#include <linuxmt/ntty.h>

#ifdef CONFIG_CONSOLE_DIRECT

extern struct tty ttys[];

#define ESC 27
#define KB_SIZE 64

#ifdef CONFIG_DEFAULT_KEYMAP

static unsigned char xtkb_scan[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','[',']',015,0202,'a','s',
	'd','f','g','h','j','k','l',';',
	'\'',0140,0200,0134,'z','x','c','v',
	'b','n','m',',','.','/',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_shifted[]=
{
	0,033,'!','@','#','$','%','^',
	'&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','{','}',015,0202,'A','S',
	'D','F','G','H','J','K','L',':',
	042,'~',0200,'|','Z','X','C','V',
	'B','N','M','<','>','?',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};

static unsigned char xtkb_scan_ctrl_alt[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','[',']',015,0202,'a','s',
	'd','f','g','h','j','k','l',';',
	'\'',0140,0200,0134,'z','x','c','v',
	'b','n','m',',','.','/',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

/*********************************
 * Quick add. Probably not good. *
 * SA                            *
 *********************************/

static unsigned char xtkb_scan_caps[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','[',']',015,0202,'A','S',
	'D','F','G','H','J','K','L',':',
	042,'~',0200,'|','Z','X','C','V',
	'B','N','M',',','.','/',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else
#if defined(CONFIG_BE_KEYMAP) || defined(CONFIG_FR_KEYMAP)
static unsigned char xtkb_scan[]=
{
	0,033,'&','é','\"','\'','(','§',
	'è','!','ç','à',')','-','\b','\t',
	'a','z','e','r','t','y','u','i',
	'o','p','^','$',015,0202,'q','s',
	'd','f','g','h','j','k','l','m',
	'ù','²','µ',0134,'w','x','c','v',
	'b','n',',',';',':','=',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_shifted[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','°','_','\b','\t',
	'A','Z','E','R','T','Y','U','I',
	'O','P','¨','*',015,0202,'Q','S',
	'D','F','G','H','J','K','L','M',
	'%','²',0200,'£','W','X','C','V',
	'B','N','?','.','/','+',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};

static unsigned char xtkb_scan_ctrl_alt[]=
{
	0,033,'|','@','#','\'','(','^',
	'è','!','{','}',')','-','\b','\t',
	'a','z','e','r','t','y','u','i',
	'o','p','[',']',015,0202,'q','s',
	'd','f','g','h','j','k','l','m',
	'´','²','`',0134,'w','x','c','v',
	'b','n',',',';',':','~',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177

};

static unsigned char xtkb_scan_caps[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','°','_','\b','\t',
	'A','Z','E','R','T','Y','U','I',
	'O','P','¨','*',015,0202,'Q','S',
	'D','F','G','H','J','K','L','M',
	'%','²',0200,'£','W','X','C','V',
	'B','N','?','.','/','+',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else
#ifdef CONFIG_UK_KEYMAP
static unsigned char xtkb_scan[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','[',']',015,0202,'a','s',
	'd','f','g','h','j','k','l',';',
	'\'','#',0200,0134,'z','x','c','v',
	'b','n','m',',','.','/',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_shifted[]=
{
	0,033,'!','"','£','$','%','^',
	'&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','{','}',015,0202,'A','S',
	'D','F','G','H','J','K','L',':',
	042,'~',0200,'|','Z','X','C','V',
	'B','N','M','<','>','?',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};

static unsigned char xtkb_scan_ctrl_alt[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','[',']',015,0202,'a','s',
	'd','f','g','h','j','k','l',';',
	'\'',0140,0200,0134,'z','x','c','v',
	'b','n','m',',','.','/',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_caps[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','[',']',015,0202,'A','S',
	'D','F','G','H','J','K','L',':',
	042,'~',0200,'|','Z','X','C','V',
	'B','N','M',',','.','/',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else
#ifdef CONFIG_DE_KEYMAP
/**************************************************************
 * German Keyboard adapted from the spanish layout            *
 * by Klaus Syttkus (no steady e-mail address right now)      *
 * Some characters might not display correctly on your screen *
 * and wont do so on the elks console...                      *
 **************************************************************/

static unsigned char xtkb_scan[]=
{
	0, 033, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', 'ß', '\'', '\b', '\t',
	'q', 'w', 'e', 'r', 't', 'z', 'u', 'i',
	'o', 'p', 'ü', '+', 015, 0202, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', 'ö',
	'ä', '^', 0200, '#', 'y', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '-', 0201, '*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177,0,0,'<'
};

static unsigned char xtkb_scan_shifted[]=
{
	0,033,'!','\"','§','$','%','&',
	'/','(',')','=','?',0140,'\b','\t',
	'Q','W','E','R','T','Z','U','I',
	'O','P','Ü','*',015,0202,'A','S',
	'D','F','G','H','J','K','L','Ö',
	'Ä','°',0200,'\'','Y','X','C','V',
	'B','N','M',';',':','_',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177,0,0,'>'
};

static unsigned char xtkb_scan_ctrl_alt[]=
{
	0,033,'1',0xB2,0xB3,'4','5','6',
	'{','[',']','}','\\','\'','\b','\t',
	'@','w','e','r','t','z','u','i',
	'o','p','ü','~',015,0202,0xa0,'s',
	'd','f','g','h','j','k','l','ö',
	'ä','^',0200,'#','y','x','c','v',
	'b','n',0xB5,',','.','-',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177,0,0,'|'
};

static unsigned char xtkb_scan_caps[84]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'Q','W','E','R','T','Z','U','I',
	'O','P','Ü','+',015,0202,'A','S',
	'D','F','G','H','J','K','L','Ö',
	'Ä',0x80,0200,'<','Y','X','C','V',
	'B','N','M',',','.','-',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else
#ifdef CONFIG_ES_KEYMAP
/* Spanish Keyboard add by Nacho Martin. imartin@cie.uva.es */

static unsigned char xtkb_scan[84]=
{
	0, 033, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', '-', '=', '\b', '\t',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
	'o', 'p', '\'', '`', 015, 0202, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', 0xa4,
	';', 135, 0200, '<', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '\'', 0201, '*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_shifted[84]=
{
	0,033,173,168,'#','$','%','/',
	'&','*','(',')','_','+','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','\"','^',015,0202,'A','S',
	'D','F','G','H','J','K','L',0xa5,
	':',0x80,0200,'>','Z','X','C','V',
	'B','N','M','?','!','\"',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};

static unsigned char xtkb_scan_ctrl_alt[84]=
{
	0,033,'|','@','·','~','5','6',
	'7','8','{','}','-','=','\b','\t',
	'q','w',0x82,'r','t','y',0xa3,0xa1,
	0xa2,'p','[',']',015,0202,0xa0,'s',
	'd','f',0x81,'h','j','k','l','ñ',
	':',0x87,0200,'\\','z','x','c','v',
	'b','n','m',',','.','\'',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_caps[84]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','\'','`',015,0202,'A','S',
	'D','F','G','H','J','K','L',0xa5,
	';',0x80,0200,'<','Z','X','C','V',
	'B','N','M',',','.','\'',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else
#ifdef CONFIG_SE_KEYMAP
/**************************************************************
 * Swedish keyboard adapted from the German layout by Per     *
 * Olofsson (MagerValp@cling.gu.se). This works fine on my    *
 * laptop (Toshiba T1200). YMMV.                              *
 **************************************************************/

static unsigned char xtkb_scan[]=
{
	0, 033, '1', '2', '3', '4', '5', '6',
	'7', '8', '9', '0', '+', '\'', '\b', '\t',
	'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
	'o', 'p', 'å', '~', 015, 0202, 'a', 's',
	'd', 'f', 'g', 'h', 'j', 'k', 'l', 'ö',
	'ä', '\'', 0200, '<', 'z', 'x', 'c', 'v',
	'b', 'n', 'm', ',', '.', '-', 0201, '*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177,0,0,'<'
};

static unsigned char xtkb_scan_shifted[]=
{
	0,033,'!','\"','#','$','%','&',
	'/','(',')','=','?',0140,'\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','Å','^',015,0202,'A','S',
	'D','F','G','H','J','K','L','Ö',
	'Ä','*',0200,'>','Z','X','C','V',
	'B','N','M',';',':','_',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177,0,0,'>'
};

static unsigned char xtkb_scan_ctrl_alt[]=
{
	0,033,'1','@','£','4','5','6',
	'{','[',']','}','\\','\'','\b','\t',
	'q','w','e','r','t','y','u','i',
	'o','p','å','~',015,0202,0xa0,'s',
	'd','f','g','h','j','k','l','ö',
	'ä','\'',0200,'|','z','x','c','v',
	'b','n',0xB5,',','.','-',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177,0,0,'|'
};

static unsigned char xtkb_scan_caps[84]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','-','=','\b','\t',
	'Q','W','E','R','T','Y','U','I',
	'O','P','Å','~',015,0202,'A','S',
	'D','F','G','H','J','K','L','Ö',
	'Ä',0x80,0200,'<','Z','X','C','V',
	'B','N','M',',','.','-',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else

#ifdef CONFIG_DV_KEYMAP
/************************************************************************
*  First attempt at Dvorak keyboard layout . Peter Vachuska . July 2000 *
************************************************************************/
static unsigned char xtkb_scan[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','[',']','\b','\t',
	'\'',',','.','p','y','f','g','c',
	'r','l','/','=',015,0202,'a','o',
	'e','u','i','d','h','t','n','s',
	'-',0140,0200,0134,';','q','j','k',
	'x','b','m','w','v','z',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_shifted[]=
{
	0,033,'!','@','#','$','%','^',
	'&','*','(',')','{','}','\b','\t',
	042,'<','>','P','Y','F','G','C',
	'R','L','?','+',015,0202,'A','O',
	'E','U','I','D','H','T','N','S',
	'_','~',0200,'|',':','Q','J','K',
	'X','B','M','W','V','Z',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};

static unsigned char xtkb_scan_ctrl_alt[]=
{
	0,033,'!','@','#','$','%','^',
	'&','*','(',')','[',']','\b','\t',
	'\'',',','.','P','Y','F','G','C',
	'R','L','/','=',015,0202,'A','O',
	'E','U','I','D','H','T','N','S',
	'-',0140,0200,0134,';','Q','J','K',
	'X','B','M','W','V','Z',0201,'*',
	0203,' ',0204,0241,0242,0243,0244,0245,
	0246,0247,0250,0251,0252,0205,0210,0267,
	0270,0271,0211,0264,0265,0266,0214,0261,
	0262,0263,'O',0177
};

static unsigned char xtkb_scan_caps[]=
{
	0,033,'1','2','3','4','5','6',
	'7','8','9','0','[',']','\b','\t',
	'\'',',','.','P','Y','F','G','C',
	'R','L','/','=',015,0202,'A','O',
	'E','U','I','D','H','T','N','S',
	'-',0140,0200,0134,';','Q','J','K',
	'X','B','M','W','V','Z',0201,'*',
	0203,' ',0204,0221,0222,0223,0224,0225,
	0226,0227,0230,0231,0232,0204,0213,'7',
	'8','9',0211,'4','5','6',0214,'1',
	'2','3','0',0177
};
#else

#include"keymap.h"
#endif /* CONFIG_DV_KEYMAP */
#endif /* CONFIG_SE_KEYMAP */
#endif /* CONFIG_ES_KEYMAP */
#endif /* CONFIG_DE_KEYMAP */
#endif /* CONFIG_UK_KEYMAP */
#endif /* CONFIG_BE_KEYMAP || CONFIG_FR_KEYMAP */
#endif /* CONFIG_DEFAULT_KEYMAP */

int Current_VCminor = 0;
int kraw = 0;

/*
 *	Keyboard state - the poor little keyboard controller hasnt
 *	got the brains to remember itself.
 */
/*********************************************
 * Changed this a lot. Made it work, too. ;) *
 * SA 1996                                   *
 *********************************************/
#define LSHIFT 1
#define RSHIFT 2
#define CTRL 4
#define ALT 8
#define CAPS 16
#define NUM 32
#define ALT_GR 64

#define ANYSHIFT 3 /* [LR]SHIFT */

/****************************************************
 * Queue for input received but not yet read by the *
 * application.                                     *
 * SA 1996                                          *
 * There needs to be many buffers if we implement   *
 * virtual consoles...                              *
 ****************************************************/

static void SetLeds();

void AddQueue();
int GetQueue();
int KeyboardInit();

/* Not for long... */
void xtk_init()
{
   KeyboardInit();
}

int KeyboardInit()
{
}

/*
 *	XT style keyboard I/O is almost civilised compared
 *	with the monstrosity AT keyboards became.
 */
 
void keyboard_irq(irq,regs,dev_id)
int irq;
struct pt_regs *regs;
void *dev_id;
{
	int code;
	int mode;
	int IsRelease;
	int key;
	int E0 = 0;
	static unsigned ModeState = 0;
	static int E0Prefix = 0;

	code=inb_p(KBD_IO);
	mode=inb_p(KBD_CTL);
	outb_p(mode | 0x80,KBD_CTL);  /* Necessary for the XT. */
	outb_p(mode,KBD_CTL);

	if (kraw)
	{
		AddQueue(code);
		return;
	}
	if( code == 0xE0 ) /* Remember this has been received */
	{
		E0Prefix = 1;
		return;
	}
	if( E0Prefix )
	{
		E0 = 1;
		E0Prefix = 0;
	}
	IsRelease = code & 0x80;
	switch( code & 0x7F )
	{
		case 29 :
        		IsRelease ? ( ModeState &= ~CTRL ) : ( ModeState |= CTRL );
        		return;
		case 42 :
			IsRelease ? ( ModeState &= ~LSHIFT ) : ( ModeState |= LSHIFT );
			return;
		case 54 :
			IsRelease ? ( ModeState &= ~RSHIFT ) : ( ModeState |= RSHIFT );
			return;
		case 56 : 
#if defined(CONFIG_DE_KEYMAP) || defined(CONFIG_SE_KEYMAP)
			if ( E0 == 0 ) {
				IsRelease ? ( ModeState &= ~ALT ) : ( ModeState |= ALT ) ;
			}
			else {
				IsRelease ? ( ModeState &= ~ALT_GR ) : ( ModeState |= ALT_GR ) ;
			};
#else
			IsRelease ? ( ModeState &= ~ALT ) : ( ModeState |= ALT ) ;
#endif
			return;
		case 58 :
			ModeState ^= IsRelease ? 0 : CAPS;
			return;
		case 69 :
			ModeState ^= IsRelease ? 0 : NUM;
			return;
		default :
        		if( IsRelease )
				return;
			break;
	}

	/*	Handle CTRL-ALT-DEL	*/

	if ((code == 0x53) && (ModeState & CTRL) && (ModeState & ALT))
		ctrl_alt_del();

	/*
	 *	Pick the right keymap
	 */
	if( ModeState & CAPS && !( ModeState & ANYSHIFT ) )
		key = xtkb_scan_caps[ code ];
	else if( ModeState & ANYSHIFT && !( ModeState & CAPS ) )
		key = xtkb_scan_shifted[ code ];

	/* added for belgian keyboard (Stefke) */

	else if ((ModeState & CTRL) && (ModeState & ALT))
		key = xtkb_scan_ctrl_alt[ code ];

	/* end belgian					*/

	/* added for German keyboard (Klaus Syttkus) */

	else if (ModeState & ALT_GR)
		key = xtkb_scan_ctrl_alt[code];
	/* end German */
	else
		key = xtkb_scan[ code ];

	if( ModeState & CTRL && code < 14 && !(ModeState & ALT))
		key = xtkb_scan_shifted[ code ];
	if( code < 70 && ModeState & NUM )
		key = xtkb_scan_shifted[ code ];
	/*
	 *	Apply special modifiers
	 */
	if( ModeState & ALT && !(ModeState & CTRL))  /* Changed to support CTRL-ALT */
		key |= 0x80; /* META-.. */
	if( !key ) /* non meta-@ is 64 */
		key = '@';
	if( ModeState & CTRL && !(ModeState & ALT))   /* Changed to support CTRL-ALT */
		key &= 0x1F; /* CTRL-.. */
	if( code < 0x45 && code > 0x3A ) /* F1 .. F10 */
	{
#ifdef CONFIG_CONSOLE_DIRECT
		if( ModeState & ALT )
		{
			Console_set_vc( code - 0x3B );
			return;
		}
#endif
		AddQueue( ESC );
		AddQueue( code - 0x3B + 'a' );
		return;
	}
	if( E0 ) /* Is extended scancode */
		switch( code )
		{
			case 0x48 :  /* Arrow up */
				AddQueue( ESC );
				AddQueue( 'A' );
				return;
			case 0x50 :  /* Arrow down */
				AddQueue( ESC );
				AddQueue( 'B' );
				return;
			case 0x4D :  /* Arrow right */
				AddQueue( ESC );
				AddQueue( 'C' );
				return;
			case 0x4B :  /* Arrow left */
				AddQueue( ESC );
				AddQueue( 'D' );
				return;
			case 0x1c :  /* keypad enter */
				AddQueue( '\n' );
				return;
	}
	if (key == '\r')
		key = '\n';
	AddQueue( key );
}

/* Ack.  We can't add a character until the queue's ready */

void AddQueue( Key )
unsigned char Key;
{
	register struct tty * ttyp = &ttys[Current_VCminor];
#if 0
	if ((ttyp->termios.c_lflag & ISIG) && (ttyp->pgrp)) {
		int sig = 0;
		if (Key == ttyp->termios.c_cc[VINTR]) {
			sig = SIGINT;
		}
		if (Key == ttyp->termios.c_cc[VSUSP]) {
			sig = SIGTSTP;
		}
		if (sig) {
			kill_pg(ttyp->pgrp, sig, 1);
			return;
		}
	}
	if (ttyp->inq.size != 0) {
		chq_addch(&ttyp->inq, Key, 0);
	}
#else
	if (!tty_intcheck(ttyp, Key) && (ttyp->inq.size != 0)) {
		chq_addch(&ttyp->inq, Key, 0);
	}
#endif
	return;
}

/*
 *      Busy wait for a keypress in kernel state for bootup/debug.
 */

int wait_for_keypress()
{
	isti();
	return chq_getch(&ttys[0].inq, 0, 1);
}

#endif /* CONFIG_CONSOLE_DIRECT */
