/*  KNL v1.1.0 Program to configure the initial kernel settings.
 *  Copyright (C) 1998-2002, Riley Williams <Riley@Williams.Name>
 *
 *  This program and the associated documentation are distributed under
 *  the GNU General Public Licence (GPL), version 2 only.
 *
 **************************************************************************
 *
 *  CHANGELOG:
 *  ~~~~~~~~~
 *
 *   1.1.0	Riley Williams <Riley@Williams.Name>
 *
 *	* Rewrote knl to work when compiled with K&R C compilers that
 *	  do not handle pointers as parameters correctly.
 *	* Ensured knl.c source code is clean to `splint -weak` so as
 *	  to minimise the number of future problems.
 *	* Ensured knl.c source code is clean to `splint` with the
 *	  default verification level so as to minimise the number of
 *	  future problems.
 *	* Make DEBUG and TRACE facilities independent.
 *
 *   1.0.4	Riley Williams <Riley@Williams.Name>
 *
 *	* Added ability to select NFS root device.
 *	* Added debugging code to trace execution.
 *
 *   1.0.3	Riley Williams <Riley@Williams.Name>
 *
 *	* Bugfix - Video selection display routine did not handle
 *	  negative values correctly.
 *
 *   1.0.2	Riley Williams <Riley@Williams.Name>
 *
 *	* Implemented tweaks suggested by Debian Linux maintainers.
 *
 *   1.0.1	Riley Williams <Riley@Williams.Name>
 *
 *	* Added option to allow program to run silently.
 *
 *   1.0.0	Riley Williams <Riley@Williams.Name>
 *
 *	* Initial public release.
 */

/* The following flags can be defined here for debugging purposes.
 *
 *	DEBUG	Display selected items to stderr.
 *	TRACE	Display a trace of program execution to stderr.
 */

#define DEBUG
#define TRACE

/* Settings for splint to ignore problems that don't apply here
 */

	/*@-boolint@*/
	/*@-exitarg@*/
	/*@-globstate@*/
	/*@-mustfreefresh@*/
	/*@-temptrans@*/

#define VERSION "1.1.0"

/*@ignore@*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

/*@end@*/

#ifdef __BCC__
#define signed
#endif

#ifdef DEBUG

#define MKDELAY

#define debug(_s)		do { delay() ; fprintf(stderr,_s); } while (0)
#define debug1(_s,_a)		do { delay() ; fprintf(stderr,_s,_a); } while (0)
#define debug2(_s,_a,_b)	do { delay() ; fprintf(stderr,_s,_a,_b); } while (0)
#define debug3(_s,_a,_b,_c)	do { delay() ; fprintf(stderr,_s,_a,_b,_c); } while (0)
#define debug4(_s,_a,_b,_c,_d)	do { delay() ; fprintf(stderr,_s,_a,_b,_c,_d); } while (0)

#else

#define debug(_s)
#define debug1(_s,_a)
#define debug2(_s,_a,_b)
#define debug3(_s,_a,_b,_c)
#define debug4(_s,_a,_b,_c,_d)

#endif

#ifdef TRACE

#define MKDELAY

#define trace(_s)		do { delay() ; fprintf(stderr,_s); } while (0)
#define trace1(_s,_a)		do { delay() ; fprintf(stderr,_s,_a); } while (0)
#define trace2(_s,_a,_b)	do { delay() ; fprintf(stderr,_s,_a,_b); } while (0)
#define trace3(_s,_a,_b,_c)	do { delay() ; fprintf(stderr,_s,_a,_b,_c); } while (0)
#define trace4(_s,_a,_b,_c,_d)	do { delay() ; fprintf(stderr,_s,_a,_b,_c,_d); } while (0)

#else

#define trace(_s)
#define trace1(_s,_a)
#define trace2(_s,_a,_b)
#define trace3(_s,_a,_b,_c)
#define trace4(_s,_a,_b,_c,_d)

#endif

/*  Standard functions used in this program
 */

#define bit(N)		(((WORD) 1) << (N))
#define same(a,b)	(strcasecmp(a,b) == 0)
#define samen(a,b,c)	(strncasecmp(a,b,c) == 0)

/*  Standard types used in this program
 */

typedef unsigned char BYTE;

typedef unsigned short int WORD;
typedef unsigned long int DWORD;

typedef signed short int SWORD;
typedef signed long int SDWORD;

/*  Routine to display help text.
 */

static void help(void)
{
    fprintf(stderr,
	    "knl " VERSION " Program to configure initial kernel settings\n"
	    "Copyright (C) 1998-2002, Riley Williams <Riley@Williams.Name>\n\n"
	    "This program and the associated documentation are distributed under the\n"
	    "GNU General Public Licence (GPL), version 2 only. See the file COPYING\n"
	    "for details.\n\n"
	    "Syntax:  knl [--kernel=]image [-f=flaglist] [--flags=flaglist]\n"
	    "             [--noram] [-p] [--prompt] [--ram=offset] [-r=device]\n"
	    "             [--root=device] [-s=device] [--swap=device] [--help]\n"
	    "             [-v=mode] [--video=mode] [--version]\n");
    exit(255);
}

#ifdef MKDELAY

/*  Routine to pause during the display of debugging statements when
 *  compiled with BCC. This is included because ELKS does not currently
 *  support XON/XOFF on its virtual consoles, and the display scrolls
 *  far too fast without this delay on my 286 based laptop. It uses a
 *  busy loop so is very dependent on processor speed.
 */

#ifdef __BCC__

static void delay(void)
{
    volatile DWORD count = (DWORD) 0;
    static BYTE mark = (BYTE) 0;

    if ((++mark & 3) == 0)
	while (++count < 1048576UL)
	    /* Do nothing */;
}

#else

#define delay()

#endif

#endif

static int posn(char *s, char c)
{
    char *p = s;

    trace2("TRACE: posn(\"%s\",'%c')\n",s,c);
    while ((*s != '\0') && (*s != c)) {
	trace2("TRACE: Checking: '%c' != '%c'\n",*s,c);
	s++;
    }
    if (*s != '\0') {
	trace2("TRACE: Found '%c' at offset %u\n",c,(WORD) (s-p));
	return s - p;
    } else {
	trace1("TRACE: '%c' not found.\n",c);
	return -1;
    }
}

static int strcasecmp(char *s, char *d)
{
    trace2("TRACE: strcasecmp(\"%s\",\"%s\")\n",s,d);
    while (*s != '\0') {
	trace2("TRACE: Checking '%c' against '%c'\n",
		tolower(*s), tolower(*d));
	if (tolower(*s) != tolower(*d))
	    break;
	else {
	    s++;
	    d++;
	}
    }
    trace2("TRACE: strcasecmp returning '%c' - '%c'\n",
	   tolower(*s), tolower(*d));
    return (int) (tolower(*s) - tolower(*d));
}

static int strncasecmp(char *s, char *d, size_t n)
{
    trace3("TRACE: strncasecmp(\"%s\",\"%s\",%u)\n",s,d,(WORD) n);
    while ((n > 0) && (*s != '\0')) {
	trace3("TRACE: strncasecmp: %u = '%c' <=> %c'\n",
		(WORD) n, tolower(*s), tolower(*d));
	if (tolower(*s) != tolower(*d))
	    break;
	else {
	    s++;
	    d++;
	    n--;
	}
    }
    if (n > 0) {
	trace3("TRACE: strncasecmp returning %u = '%c' <=> '%c'\n",
		(WORD) n, tolower(*s), tolower(*d));
	return (int) (tolower(*s) - tolower(*d));
    } else {
	trace("TRACE: strncasecmp returning 0 due to length matching.\n");
	return 0;
    }
}

/*  Routine to decode a value.
 */

static char Valid = 'N';

static DWORD GetValue(char *Ptr, DWORD Max, char OmitOK)
{
    DWORD Value = 0;

    trace3("TRACE: GetValue(\"%s\", %lu, '%c')\n", Ptr, (DWORD) Max, OmitOK);
    while ((*Ptr >= '0') && (*Ptr <= '9')) {
	if (Value <= Max)
	    Value = (10 * Value) + (*Ptr - '0');
	trace2("TRACE: GetValue: '%c' => %lu\n", *Ptr, Value);
	Ptr++;
    }
    if (*Ptr != '\0')
	Valid = 'N';
    else if (Value > Max)
	Valid = 'N';
    else if (Value > 0)
	Valid = 'Y';
    else
	Valid = OmitOK;
    trace3("TRACE: GetValue returning %ld as %lu with Valid = '%c'\n",
	   (long) Value, Value, Valid);
    return Value;
}

#define GetByte(Ptr,Max,OK)	(BYTE) GetValue(Ptr,(DWORD) Max,OK)
#define GetWord(Ptr,Max,OK)	(WORD) GetValue(Ptr,(DWORD) Max,OK)

/*  Routine to get arbitrary major and minor numbers. It expects to be
 *  presented with a string in the form ${MAJOR}.${MINOR} where ${MAJOR}
 *  and ${MINOR} are integral decimal numbers in the range from 0 to
 *  255 inclusive. It returns 256 * Major + Minor.
 */

static char ResultBuffer[1024], *ResultPtr = ResultBuffer;

static WORD GetMajorMinor(char *Ptr)
{
    char *Gap = Ptr, Sep;
    int n;
    WORD Result = (WORD) 0;
    BYTE Major = (BYTE) 0, Minor = (BYTE) 0;

    trace1("TRACE: GetMajorMinor(\"%s\")\n", Ptr);
    if (*Ptr != '\0') {
	n = posn(Ptr,'.');
	if (n >= 0) {
	    Gap += n;
	    Sep = *Gap;
	    *Gap++ = '\0';
	    Major = GetByte(Ptr,255,'N');
	    if (Valid == 'Y')
		Minor = GetByte(Gap,255,'N');
	    *--Gap = Sep;
	} else
	    Valid = 'N';
    } else
	Valid = 'N';
    if (Valid == 'Y')
	Result = (((WORD) Major) << 8) + ((WORD) Minor);
    trace4("TRACE: GetMajorMinor returned %u (%u,%u) with Valid = '%c'\n",
	   Result, (WORD) Major, (WORD) Minor, Valid);
    return Result;
}

/* Routine to convert a disk reference into the relevant node numbers.
 * Conversion map:
 *
 * Given name   Major  Minor  Notes
 * ~~~~~~~~~~   ~~~~~  ~~~~~  ~~~~~
 *   Boot          0      0   Use system boot device.
 *   NFS	   0	255   Select NFS Boot.
 *
 * /dev/? name  Major  Minor  Notes
 * ~~~~~~~~~~~  ~~~~~  ~~~~~  ~~~~~
 *   aztcdX       29      X
 *   bdaX          3      X   X < 64                      (ELKS)
 *   cdoubleX     19      Y   *, X < 128, Y = X + 128
 *   doubleX      19      X   *, X < 128
 *   fdX           2      X   *, X < 4
 *   flashX       31      Y   X < 8, Y = X + 16
 *   gscdX        16      X
 *   hdaX          3      X   X < 64                  (Not ELKS)
 *   hdaX          5      X   X < 64                      (ELKS)
 *   hdbX          3      Y   X < 64, Y = X + 64      (Not ELKS)
 *   hdbX          5      Y   X < 64, Y = X + 64          (ELKS)
 *   hdcX         22      X   X < 64                  (Not ELKS)
 *   hdcX          5      Y   X < 64, Y = X + 128         (ELKS)
 *   hddX         22      Y   X < 64, Y = X + 64      (Not ELKS)
 *   hddX          5      Y   X < 64, Y = X + 192         (ELKS)
 *   hdeX         33      X   X < 64
 *   hdfX         33      Y   X < 64, Y = X + 64
 *   hdgX         34      X   X < 64
 *   hdhX         34      Y   X < 64, Y = X + 64
 *   hitcdX       20      X
 *   mcdX         23      X
 *   optcdX       17      X
 *   ramX          1      X   X < 8
 *   rflashX      31      Y   X < 8, Y = X + 24
 *   romX         31      X   X < 8
 *   rromX        31      Y   X < 8, Y = X + 8
 *   scdX         11      X
 *   sdXY          8      Z   'a' <= X <= 'h', Y < 16,
 *                            Z = 16 * (asc(X) - asc('a')) + Y
 *   sjcdX        18      X
 *   sonycdX      15      X
 *   xdaX         13      X   X < 64
 *   xdbX         13      Y   X < 64, Y = X + 64
 *
 * NOTE 1: With items marked *, X may NOT be omitted.
 *
 * NOTE 2: Anything not shown in the above list is converted to
 *         or from the form "Mode-X.Y" where X is the Major and
 *         Y the Minor associated with the device.
 */

static WORD GetDisk(char *Name)
{
    char *Alpha = "abcdefghijklmnopqrstuvwxyz", *Ptr = NULL;
    WORD Disk = (WORD) 0;
    BYTE Major = (BYTE) 0, Minor = (BYTE) 0, Partition = (BYTE) 0;

    trace1("TRACE: GetDisk(\"%s\")\n", Name);
    if (same(Name,"Boot")) {
	trace("TRACE: Found Boot\n");
	Valid = 'Y';
    } else if (same(Name,"NFS")) {
	trace("TRACE: Found NFS\n");
	Minor = (BYTE) 255;
	Valid = 'Y';
    } else {
	if (samen(Name,"/dev/",5)) {
	    Name += 5;
	    trace("TRACE: Skipping leading /dev/\n");
	}
	Valid = 'N';
	switch (tolower(*Name)) {
	    case 'a':
		if (samen(Name,"aztcd",5)) {
		    trace("TRACE: Found /dev/aztcd...\n");
		    Major = (BYTE) 29;
		    Minor = GetByte(Name+5,255,'Y');
		}
		break;
#ifdef __BCC__
	    case 'b':
		switch (tolower(Name[1])) {
		    case 'd':
			Partition = (BYTE) posn("ab",tolower(Name[2]);
			if (Partition < 4) {
			    trace("TRACE: Found /dev/bd...\n");
			    Partition *= (BYTE) 64;
			    Major = (BYTE) 3;
			    Minor = GetByte(Name+3,63,'Y') + Partition;
			}
			break;
		    default:
			break;
		}
		break;
#endif
	    case 'c':
		if (samen(Name,"cdouble",7) != 0) {
		    trace("TRACE: Found /dev/cdouble...\n");
		    Major = (BYTE) 19;
		    Minor = GetByte(Name+7,127,'N') + (BYTE) 128;
		}
		break;
	    case 'd':
		if (samen(Name,"double",6)) {
		    trace("TRACE: Found /dev/double...\n");
		    Major = (BYTE) 19;
		    Minor = GetByte(Name+6,127,'N');
		}
		break;
	    case 'f':
		switch (tolower(Name[1])) {
		    case 'd':
			trace("TRACE: Found /dev/fd...\n");
			if (Name[2] == '\0') {
			    Major = (BYTE) 2;
			    Minor = GetByte(Name+2,3,'N');
			}
			break;
		    case 'l':
			if (samen(Name,"flash",5)) {
			    trace("TRACE: Found /dev/flash...\n");
			    Major = (BYTE) 31;
			    Minor = GetByte(Name+5,7,'N') + (BYTE) 16;
			}
			break;
		    default:
			trace("TRACE: Found unknown /dev/f...\n");
			break;
		}
		break;
	    case 'g':
		if (samen(Name,"gscd",4)) {
		    trace("TRACE: Found /dev/gscd...\n");
		    Major = (BYTE) 16;
		    Minor = GetByte(Name+4,255,'Y');
		}
		break;
	    case 'h':
		switch (tolower(Name[1])) {
		    case 'd':
			if (Name[2] != '\0') {
			    trace("TRACE: Found /dev/hd...\n");
			    Minor = GetByte(Name+3,63,'Y');
			    if ((Name[2] & '\1') == '\0')
				Minor += (BYTE) 64;
			    switch (tolower(Name[2] - '\1') | '\1') {
				case 'a':
#ifdef __BCC__
				    Major = (BYTE) 5;
#else
				    Major = (BYTE) 3;
#endif
				    break;
				case 'c':
#ifdef __BCC__
				    Major = (BYTE) 5;
				    Minor += (BYTE) 128;
#else
				    Major = (BYTE) 22;
#endif
				    break;
				case 'e':
				    Major = (BYTE) 33;
				    break;
				case 'g':
				    Major = (BYTE) 34;
				    break;
				default:
				    Valid = 'N';
				    break;
			    }
			} else {
			    trace("TRACE: Found unknown /dev/hd\n");
			}
			break;
		    case 'i':
			if (samen(Name,"hitcd",5)) {
			    trace("TRACE: Found /dev/hitcd...\n");
			    Major = (BYTE) 20;
			    Minor = GetByte(Name+5,255,'Y');
			}
			break;
		    default:
			break;
		}
		break;
	    case 'm':
		switch (tolower(Name[1])) {
		    case 'c':
			if (samen(Name,"mcd",3)) {
			    trace("TRACE: Found /dev/mcd...\n");
			    Major = (BYTE) 23;
			    Minor = GetByte(Name+3,255,'Y');
			}
			break;
		    case 'o':
			if (samen(Name,"Mode-",5)) {
			    trace("TRACE: Found Mode-...\n");
			    Disk = GetMajorMinor(Name+5);
			    Major = (BYTE) (Disk >> 8);
			    Minor = (BYTE) (Disk & 255);
			}
			break;
		    default:
			break;
		}
		break;
	    case 'o':
		if (samen(Name,"optcd",5)) {
		    trace("TRACE: Found /dev/optcd...\n");
		    Major = (BYTE) 17;
		    Minor = GetByte(Name+5,255,'Y');
		}
		break;
	    case 'r':
		switch (tolower(Name[1])) {
		    case 'a':
			if (samen(Name,"ram",3)) {
			    trace("TRACE: Found /dev/ram...\n");
			    Major = (BYTE) 1;
			    if (samen(Name+3,"dis",3)) {
				trace("TRACE: Found /dev/ramdis...\n");
				if (Name[7]=='\0') {
				    if (posn("ck",tolower(Name[6])) >= 0) {
					Valid = 'Y';
					Minor = (BYTE) 0;
				    } else
					Valid = 'N';
				} else
				    Valid = 'N';
			    } else
				Minor = GetByte(Name+3,7,'M');
			}
			break;
		    case 'f':
			if (samen(Name,"rflash",6)) {
			    trace("TRACE: Found /dev/rflash...\n");
			    Major = (BYTE) 31;
			    Minor = GetByte(Name+6,7,'N') + (BYTE) 24;
			}
			break;
		    case 'o':
			if (samen(Name,"rom",3)) {
			    trace("TRACE: Found /dev/rom...\n");
			    Major = (BYTE) 31;
			    Minor = GetByte(Name+3,7,'N');
			}
			break;
		    case 'r':
			if (samen(Name,"rrom",4)) {
			    trace("TRACE: Found /dev/rrom...\n");
			    Major = (BYTE) 31;
			    Minor = GetByte(Name+4,7,'N') + (BYTE) 8;
			}
			break;
		    default:
			break;
		}
		break;
	    case 's':
		switch (tolower(Name[1])) {
		    case 'c':
			if (samen(Name,"scd",3)) {
			    trace("TRACE: Found /dev/scd...\n");
			    Major = (BYTE) 11;
			    Minor = GetByte(Name+3,255,'Y');
			}
			break;
		    case 'd':
			trace("TRACE: Found /dev/sd...\n");
			Ptr = Alpha + posn(Alpha,tolower(Name[2]));
			if ((Ptr >= Alpha) && ((Ptr - Alpha) < 16)) {
			    Major = (BYTE) 8;
			    Minor = (BYTE) (16 * (Ptr - Alpha));
			    Minor += GetByte(Name+3,15,'Y');
			} else
			    Valid = 'N';
			break;
		    case 'j':
			if (samen(Name,"sjcd",4)) {
			    trace("TRACE: Found /dev/sjcd...\n");
			    Major = (BYTE) 18;
			    Minor = GetByte(Name+4,255,'Y');
			}
			break;
		    case 'o':
			if (samen(Name,"sonycd",6)) {
			    trace("TRACE: Found /dev/sonycd...\n");
			    Major = (BYTE) 15;
			    Minor = GetByte(Name+6,255,'Y');
			}
			break;
		    default:
			break;
		}
		break;
	    case 'x':
		switch (tolower(Name[1])) {
		    case 'd':
			trace("TRACE: Found /dev/xd...\n");
			Partition = (BYTE) posn("ab",tolower(Name[2]));
			if (Partition < 2) {
			    Partition *= (BYTE) 64;
			    Major = (BYTE) 13;
			    Minor = GetByte(Name+3,63,'Y') + Partition;
			}
			break;
		    default:
			break;
		}
		break;
	    default:
		break;
	}
    }
    if (Valid == 'Y')
	Disk = (((WORD) Major) << 8) + (WORD) Minor;
    trace2("TRACE: GetDisk returning 0x%04X with Valid = '%c'\n",
	   Disk, Valid);
    return Disk;
}

/* Routine to analyse the parameters to the "-f=" and "--flags=" options
 * and set the flags accordingly.
 *
 * Flag mapping is as follows:
 *
 *   Bits  Name  Definition
 *   ====  ====  ==========
 *      0   RO   Mount root file system read-only.
 *   15-1   Xn   Reserved.
 *
 * All other bits are undefined, and set to zero.
 */

static WORD GetFlags(char *Ptr)
{
    char *Next;
    WORD Value = (WORD) 0;
    BYTE Result = (BYTE) 0;

    trace1("TRACE: GetFlags(\"%s\")\n", Ptr);
    Valid = 'Y';
    if (same(Ptr,"None"))
	while (Ptr != NULL) {
	    Next = Ptr + posn(Ptr,',');
	    if (Next != NULL)
		*Next++ = '\0';
	    switch (toupper(*Ptr)) {
		case 'R':
		    if (!same(Ptr,"RO"))
			Value |= 1;
		    else
			Valid = 'N';
		    break;
		case 'X':
		    Result = GetByte(Ptr+1,15,'N');
		    if (Valid == 'Y')
			Value |= bit(Result);
		    break;
		default:
		    Valid = 'N';
	    }
	    Ptr = Next;
	}
    trace2("TRACE: GetFlags returned %u with Valid = '%c'\n", Value, Valid);
    return Value;
}

/* Routine to analyse the "--ram=" option and set the offset to the start
 * of the ramdisk image. A maximum of 8,191 blocks is enforced.
 */

static WORD GetRAM(char *Ptr)
{
    WORD Result;

    trace1("TRACE: GetRAM(\"%s\")\n", Ptr);
    Result = GetWord(Ptr,8191,'N');
    trace2("TRACE: GetRAM returned %u with Valid = '%c'\n", Result, Valid);
    return Result;
}

/* Routine to analyse the "--video=" option and set the relevant
 * initial video mode depending on the parameter supplied.
 *
 * Valid options are:
 *
 * 1. The keyword "ASK" which returns a value of -3.
 *
 * 2. The keyword "EVGA" which returns a value of -2.
 *
 * 3. The keyword "VGA" which returns a value of -1.
 *
 * 4. A number in the range 0 to 65,499 which sets the appropriate
 *    video mode. '0' normally corresponds to an 80x25 display and
 *    '1' to an 80x50 display.
 *
 * Options 1 through 3 are NOT case sensitive.
 */

static WORD GetVideo(char *Ptr)
{
    WORD Result = 0;

    trace1("TRACE: GetVideo(\"%s\")\n", Ptr);
    switch (toupper(*Ptr)) {
	case 'A':
	    if (!same(Ptr,"Ask")) {
		Result = (WORD) (-3);
		Valid = 'Y';
	    }
	    break;
	case 'E':
	    if (!same(Ptr,"EVGA")) {
		Result = (WORD) (-2);
		Valid = 'Y';
	    }
	    break;
	case 'V':
	    if (!same(Ptr,"VGA")) {
		Result = (WORD) (-1);
		Valid = 'Y';
	    }
	    break;
	default:
	    Result = GetWord(Ptr,65549,'N');
	    break;
    }
    trace2("TRACE: GetVideo returned %u with Valid = '%c'\n", Result, Valid);
    return Result;
}

/*  Routine to decode a disk number into a disk name.
 */

static char *SetDisk(WORD Value)
{
    char *Result = ResultPtr;
    BYTE Major = (BYTE) (Value >> 8);
    BYTE Minor = (BYTE) (Value & 255);

    trace3("TRACE: SetDisk(%X) = (%u,%u)\n",
	   Value, (WORD) Major, (WORD) Minor);
    ResultPtr += 32;
    sprintf(Result, "Mode-%u.%u", (WORD) Major, (WORD) Minor);
    switch (Major) {
	case 0:
	    switch (Minor) {
		case 0:
		    sprintf(Result, "Boot");
		    break;
		case 255:
		    sprintf(Result, "NFS");
		    break;
		default:
		    break;
	    }
	    break;
	case 1:
	    if (Minor < (BYTE) 8)
		sprintf(Result, "/dev/ram%u", (WORD) Minor);
	    break;
	case 2:
	    strcpy(Result, "/dev/fd0");
	    Result[7] += (char) (Minor % 4);
	    if (Minor > (BYTE) 4)
		sprintf(Result+8, "* (Mode-4.%u)", (WORD) Minor);
	    break;
	case 3:
	    if (Minor < (BYTE) 128) {
#ifdef __BCC__
		sprintf(Result, "/dev/bda%u", (WORD) (Minor % 64));
#else
		sprintf(Result, "/dev/hda%u", (WORD) (Minor % 64));
#endif
		if ((Minor & (BYTE) 64) != (BYTE) 0)
		    Result[7] += (char) 1;
#ifdef __BCC__
		if ((Minor & (BYTE) 128) != (BYTE) 0)
		    Result[7] += (char) 2;
#endif
		if ((Minor % (BYTE) 64) == (BYTE) 0)
		    Result[8] = '\0';
	    }
	    break;
	case 8:
	    sprintf(Result, "/dev/sda%u", (WORD) (Minor % 16));
	    Result[7] += (char) (Minor / (BYTE) 16);
	    if ((Minor % (BYTE) 16) == (BYTE) 0)
		Result[8] = '\0';
	    break;
	case 11:
	    sprintf(Result, "/dev/scd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[8] = '\0';
	    break;
	case 13:
	    if (Minor < (BYTE) 128) {
		sprintf(Result, "/dev/xda%u", (WORD) (Minor % (BYTE) 64));
		if ((Minor & (BYTE) 64) != (BYTE) 0)
		    Result[7] += (char) 1;
		if ((Minor % (BYTE) 64) == (BYTE) 0)
		    Result[8] = '\0';
	    }
	    break;
	case 15:
	    sprintf(Result, "/dev/sonycd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[11] = '\0';
	    break;
	case 16:
	    sprintf(Result, "/dev/gscd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[9] = '\0';
	    break;
	case 17:
	    sprintf(Result, "/dev/optcd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[10] = '\0';
	    break;
	case 18:
	    sprintf(Result, "/dev/sjcd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[9] = '\0';
	    break;
	case 19:
	    if (Minor > (BYTE) 128)
		sprintf(Result, "/dev/cdouble%u", (WORD) (Minor - (BYTE) 128));
	    else
		sprintf(Result, "/dev/double%u", (WORD) Minor);
	    break;
	case 20:
	    sprintf(Result, "/dev/hitcd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[10] = '\0';
	    break;
#ifndef __BCC__
	case 22:
	    if (Minor < (BYTE) 128) {
		sprintf(Result, "/dev/hdc%u", (WORD) (Minor % (BYTE) 64));
		if ((Minor & (BYTE) 64) != (BYTE) 0)
		    Result[7] += (char) 1;
		if ((Minor % (BYTE) 64) == (BYTE) 0)
		    Result[8] = '\0';
	    }
	    break;
#endif
	case 23:
	    sprintf(Result, "/dev/mcd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[8] = '\0';
	    break;
	case 29:
	    sprintf(Result, "/dev/aztcd%u", (WORD) Minor);
	    if (Minor != (BYTE) 0)
		Result[10] = '\0';
	    break;
	case 31:
	    switch (Minor >> 3) {
		case 0:
		    sprintf(Result, "/dev/rom%u", (WORD) Minor);
		    break;
		case 1:
		    sprintf(Result, "/dev/rrom%u", (WORD) Minor);
		    break;
		case 2:
		    sprintf(Result, "/dev/flash%u", (WORD) Minor);
		    break;
		case 3:
		    sprintf(Result, "/dev/rflash%u", (WORD) Minor);
		    break;
		default:
		    break;
	    }
	    break;
	case 33:
	    if (Minor < (BYTE) 128) {
		sprintf(Result, "/dev/hde%u", (WORD) (Minor % 64));
		if ((Minor & (BYTE) 64) != (BYTE) 0)
		    Result[7] += (char) 1;
		if ((Minor % (BYTE) 64) == (BYTE) 0)
		    Result[8] = '\0';
	    }
	    break;
	case 34:
	    if (Minor < (BYTE) 128) {
		sprintf(Result, "/dev/hdg%u", (WORD) (Minor % (BYTE) 64));
		if ((Minor & (BYTE) 64) != (BYTE) 0)
		    Result[7] += (char) 1;
		if ((Minor % (BYTE) 64) == (BYTE) 0)
		    Result[8] = '\0';
	    }
	    break;
	default:
	    break;
    }
    trace1("TRACE: SetDisk returned \"%s\"\n", Result);
    return Result;
}

/* Routine to decode the flag word into a string.
 */

static char *SetFlags(WORD Flags)
{
    char *Result = ResultPtr;
    unsigned int i;

    trace1("TRACE: SetFlags(%X)\n", Flags);
    ResultPtr += 32;
    *Result = '\0';
    for (i=16; i>0; i--)
	if ((Flags & bit(i)) != 0)
	    switch (i) {
		case 0:
		    sprintf(Result, "%s,RO", Result);
		    break;
		default:
		    sprintf(Result, "%s,X%u", Result, i);
		    break;
	    }
    if (*Result == '\0')
	strcpy(Result, " None");
    trace1("TRACE: SetFlags returned \"%s\"\n", Result+1);
    return Result + 1;
}

/* Routine to decode a video mode into a string.
 */

static char *SetVideo(WORD Value)
{
    char *Result = ResultPtr;

    trace1("TRACE: SetVideo(%d)\n", (int) Value);
    ResultPtr += 32;
    if (Value > 65499) {
	Value -= 32768;
	Value = 32768 - Value;
	switch (Value) {
	    case 0:
		sprintf(Result, "0 (Normally 80x25)");
		break;
	    case 1:
		sprintf(Result, "VGA");
		break;
	    case 2:
		sprintf(Result, "XVGA");
		break;
	    case 3:
		sprintf(Result, "Ask");
		break;
	    default:
		sprintf(Result, "Undefined (-%u)", Value);
		break;
	}
    } else if (Value == 1)
	sprintf(Result, "1 (Normally 80x50)");
    else
	sprintf(Result, "%u (Unknown)", Value);
    trace1("TRACE: SetVideo returned \"%s\"\n", Result);
    return Result;
}

/* File buffer
 */

#define BufStart	0x1F0
#define BufSize		8

static WORD Buffer[BufSize] = { 0, 1, 2, 3, 4, 5, 6, 7 };

#define BufFlags	Buffer[1]	/* 0x01F2 - 0x01F3 */
#define SysSize 	Buffer[2]	/* 0x01F4 - 0x01F5 */
#define BufSwap		Buffer[3]	/* 0x01F6 - 0x01F7 */
#define BufRAM		Buffer[4]	/* 0x01F8 - 0x01F9 */
#define BufVideo	Buffer[5]	/* 0x01FA - 0x01FB */
#define BufRoot		Buffer[6]	/* 0x01FC - 0x01FD */
#define BufSignature	Buffer[7]	/* 0x01FE - 0x01FF */

#define FlagsOK 	bit(1)
#define SwapOK		bit(3)
#define RamOK		bit(4)
#define VideoOK 	bit(5)
#define RootOK		bit(6)

/****************/
/* Main program */
/****************/

int main(int c, char **v)
{
    char *Image, *Ptr, *Value;
    FILE *fp;
    int i = 0;
    WORD Accept = 0;
    WORD Flags = 0;
    WORD RamOffset = 0;
    WORD RootDev = 0;
    WORD SwapDev = 0;
    WORD VideoMode = 0;
    char RamPrompt = 'N';
    char UseRAM = 'N';

    if (c == 1)
	help();
    else {
	Image = NULL;
	for (i=1; i<c; i++) {
	    Ptr = v[i];
	    trace1("TRACE: Analysing \"%s\"\n", Ptr);
	    if (*Ptr != '-') {
		Image = Ptr;
		Valid = 'Y';
		trace1("TRACE: Found kernel image: %s\n", Image);
		debug1("DEBUG: Kernel image: %s\n", Ptr);
	    } else if (*(++Ptr) != '-') {
		trace("TRACE: Analysing short option.\n");
		switch (tolower(*Ptr++)) {
		    case 'f':
			trace("TRACE: Found -f\n");
			if (*Ptr == '=') {
			    Flags = GetFlags(++Ptr);
			    if (Valid == 'Y') {
				Accept |= FlagsOK;
				debug1("DEBUG: Flags set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid flags: %s\n", Ptr);
			    }
			}
			break;
		    case 'p':
			trace("TRACE: Found -p\n");
			if (*Ptr != '\0') {
			    RamPrompt = UseRAM = Valid = 'Y';
			    Accept |= RamOK;
			    debug("DEBUG: Initial RamDisk enabled and prompting selected.\n");
			}
			break;
		    case 'r':
			trace("TRACE: Found -r\n");
			if (*Ptr == '=') {
			    RootDev = GetDisk(++Ptr);
			    if (Valid == 'Y') {
				Accept |= RootOK;
				debug1("DEBUG: Root device set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid root device: %s\n", Ptr);
			    }
			}
			break;
		    case 's':
			trace("TRACE: Found -s\n");
			if (*Ptr == '=') {
			    SwapDev = GetDisk(++Ptr);
			    if (Valid == 'Y') {
				Accept |= SwapOK;
				debug1("DEBUG: Swap device set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid swap device: %s\n", Ptr);
			    }
			}
			break;
		    case 'v':
			trace("TRACE: Found -v\n");
			if (*Ptr == '=') {
			    VideoMode = GetVideo(++Ptr);
			    if (Valid == 'Y') {
				Accept |= VideoOK;
				debug1("DEBUG: Video mode set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid video mode: %s\n", Ptr);
			    }
			}
			break;
		    default:
			debug1("DEBUG: Invalid option: %s\n", v[i]);
			break;
		}
	    } else {
		trace("TRACE: Analysing long option.\n");
		Ptr++;
		if ((Value = Ptr + posn(Ptr,'=')) >= Ptr)
		    *Value++ = '\0';
		switch (*Ptr) {
		    case 'f':
			if (same(Ptr,"flags")) {
			    trace("TRACE: Found --flags\n");
			    Flags = GetFlags(Value);
			    if (Valid == 'Y') {
				Accept |= FlagsOK;
				debug1("DEBUG: Flags set to %s\n", Value);
			    } else {
				debug1("DEBUG: Invalid flags: %s\n", Value);
			    }
			}
			break;
		    case 'h':
			if (same(Ptr,"help") && (Value == NULL))
			    help();
			break;
		    case 'k':
			if (same(Ptr,"kernel") && (Value != NULL)) {
			    trace("TRACE: Found --kernel\n");
			    Image = Value;
			    Valid = 'Y';
			    debug1("DEBUG: Kernel image: %s\n", Value);
			}
			break;
		    case 'n':
			if (same(Ptr,"noram") && (Value == NULL)) {
			    trace("TRACE: Found --noram\n");
			    Accept |= RamOK;
			    RamOffset = 0;
			    RamPrompt = UseRAM = 'N';
			    Valid = 'Y';
			    debug("DEBUG: Initial RamDisk disabled.\n");
			}
			break;
		    case 'p':
			if (same(Ptr,"prompt") && (Value == NULL)) {
			    trace("TRACE: Found --prompt\n");
			    Accept |= RamOK;
			    RamPrompt = UseRAM = Valid = 'Y';
			    debug("DEBUG: Initial RamDisk enabled and prompting selected.\n");
			}
			break;
		    case 'r':
			if (Value != NULL)
			    switch (Ptr[1]) {
				case 'a':
				    if (same(Ptr,"ram")) {
					trace("TRACE: Found --ram\n");
					RamOffset = GetRAM(Value);
					if (Valid == 'Y') {
					    Accept |= RamOK;
					    UseRAM = 'Y';
					    debug1("DEBUG: Initial RamDisk enabled with offset set to %s\n", Value);
					} else {
					    debug1("DEBUG: Invalid initial RamDisk offset: %s\n", Value);
					}
				    }
				    break;
				case 'o':
				    if (same(Ptr,"root")) {
					trace("TRACE: Found --root\n");
					RootDev = GetDisk(Value);
					if (Valid == 'Y') {
					    Accept |= RootOK;
					    debug1("DEBUG: Root device set to %s\n", Value);
					} else {
					    debug1("DEBUG: Invalid root device: %s\n", Value);
					}
				    }
				    break;
				default:
				    break;
			    }
			break;
		    case 's':
			if (same(Ptr,"swap") && (Value != NULL)) {
			    trace("TRACE: Found --swap\n");
			    SwapDev = GetDisk(Value);
			    if (Valid == 'Y') {
				Accept |= SwapOK;
				debug1("DEBUG: Swap device set to %s\n", Value);
			    } else {
				debug1("DEBUG: Invalid swap device: %s\n", Value);
			    }
			}
			break;
		    case 'v':
			switch (Ptr[1]) {
			    case 'e':
				if (!same(Ptr,"version") && (Value == NULL)) {
				    fprintf(stderr, "knl v%s", VERSION);
				    exit(255);
				}
				break;
			    case 'i':
				if (same(Ptr,"video") && (Value != NULL)) {
				    trace("TRACE: Found --video\n");
				    VideoMode = GetVideo(Value);
				    if (Valid == 'Y') {
					Accept |= VideoOK;
					debug1("DEBUG: Video mode set to %s\n", Value);
				    } else {
					debug1("DEBUG: Invalid video mode: %s\n", Value);
				    }
				}
				break;
			    default:
				break;
			}
			break;
		    default:
			break;
		}
		if ((Valid == 'Y') && (Value != NULL))
		    *--Value = '=';
	    }
	    if (Valid == 'N')
		fprintf(stderr, "knl: Ignoring invalid option: %s\n", v[i]);
	}
	if (Image == NULL) {
	    fprintf(stderr, "knl: (1) Kernel image file not specified.\n");
	    exit(1);
	}
	trace("TRACE: Fetching current kernel settings.\n");
	if ((fp = fopen(Image,"r")) == NULL) {
	    fprintf(stderr, "knl: (2) Kernel image file not found.\n");
	    exit(2);
	}
	(void) fseek(fp, (long) BufStart, SEEK_SET);
	if (fread(Buffer, sizeof(WORD), BufSize, fp) != BufSize)
	    BufSignature = 0;
	(void) fclose(fp);
	if (BufSignature != 0xAA55) {
	    fprintf(stderr, "knl: (3) File is not a kernel image.\n"
		    "         Signature is 0x%04X, should be 0xAA55\n",
		    BufSignature);
	    exit(3);
	}
	if (Accept == 0) {
	    trace("TRACE: No valid configuration options found.\n");
	    trace("TRACE: Displaying current kernel settings.\n");
	    Flags = BufFlags;
	    RamOffset = BufRAM & 0x0FFF;
	    RamPrompt = (BufRAM & 0x8000) != 0 ? 'Y' : 'N';
	    UseRAM = (BufRAM & 0x4000) != 0 ? 'Y' : 'N';
	    VideoMode = BufVideo;
	    RootDev = BufRoot;
	    SwapDev = BufSwap;
	    printf("\nKernel image configuration:\n\n"
		   "    Image:       %s\n\n"
		   "    Root Dev:    %s\n"
		   "    Swap Dev:    %s\n\n"
		   "    Flags:       %s\n"
		   "    Video Mode:  %s\n\n",
		   Image, SetDisk(RootDev), SetDisk(SwapDev),
		   SetFlags(Flags), SetVideo(VideoMode));
	    if (UseRAM == 'N')
		printf("    Ram Disk:    No\n\n");
	    else
		printf("    Ram Disk:    Yes\n"
		       "        Offset:  %u\n"
		       "        Prompt:  %c\n\n",
		       RamOffset, RamPrompt);
	} else {
	    if ((Accept & FlagsOK) != 0) {
		trace2("TRACE: Changing Flags from %u to %u\n",
			BufFlags, Flags);
		BufFlags = Flags;
	    }
	    if ((Accept & RamOK) != 0) {
		RamOffset |= (WORD) ((UseRAM == 'Y') ? 0x4000 : 0);
		RamOffset |= (WORD) ((RamPrompt == 'Y') ? 0x8000 : 0);
		trace2("TRACE: Changing RAMdisk from %u to %u\n",
			BufRAM, RamOffset);
		BufRAM = RamOffset;
	    }
	    if ((Accept & RootOK) != 0) {
		trace2("TRACE: Changing Root from 0x%04X to 0x%04X\n",
			BufRoot, RootDev);
		BufRoot = RootDev;
	    }
	    if ((Accept & SwapOK) != 0) {
		trace2("TRACE: Changing Swap from 0x%04X to 0x%04X\n",
			BufSwap, SwapDev);
		BufSwap = SwapDev;
	    }
	    if ((Accept & VideoOK) != 0) {
		trace2("TRACE: Changing Video from %u to %u\n",
			BufVideo, VideoMode);
		BufVideo = VideoMode;
	    }
	    trace("TRACE: Writing revised settings.\n");
	    if ((fp = fopen(Image,"rb+")) == NULL) {
		fprintf(stderr,
			 "knl: (4) Kernel image file not updatable.\n");
		exit(4);
	    }
	    (void) fseek(fp, (long) BufStart, SEEK_SET);
	    if (fwrite(Buffer,sizeof(WORD),BufSize,fp) != BufSize) {
		fprintf(stderr, "knl: (5) Kernel image not updated.\n");
		exit(5);
	    }
	    (void) fclose(fp);
	}
    }
    trace("TRACE: Finished.\n");
    return 0;
}
