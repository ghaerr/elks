/*  KNL v1.0.4 Program to configure the initial kernel settings.
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
 *   1.0.4	Riley Williams <Riley@Williams.Name>
 *
 *	* Tweaked to assume ELKS devices if compiled using BCC. This
 *	  has the result that /dev/bdaX and /dev/bdbX are available,
 *        but /dev/hd[a-d]X have different node numbers.
 *	* Added debugging code to get it to work under ELKS.
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

#define VERSION "1.0.4"

#include <stdio.h>
#include <stdlib.h>

#include <ctype.h>
#include <string.h>

#ifdef __BCC__
#define signed
#endif

#define DEBUG
#define TRACE

#ifdef DEBUG

#define debug(s)			fprintf(stderr,s)
#define debug1(s,a)			fprintf(stderr,s,a)
#define debug2(s,a,b)			fprintf(stderr,s,a,b)
#define debug3(s,a,b,c)			fprintf(stderr,s,a,b,c)
#define debug4(s,a,b,c,d)		fprintf(stderr,s,a,b,c,d)
#define debug5(s,a,b,c,d,e)		fprintf(stderr,s,a,b,c,d,e)
#define debug6(s,a,b,c,d,e,f)		fprintf(stderr,s,a,b,c,d,e,f)
#define debug7(s,a,b,c,d,e,f,g)		fprintf(stderr,s,a,b,c,d,e,f,g)
#define debug8(s,a,b,c,d,e,f,g,h)	fprintf(stderr,s,a,b,c,d,e,f,g,h)
#define debug9(s,a,b,c,d,e,f,g,h,i)	fprintf(stderr,s,a,b,c,d,e,f,g,h,i)

#else

#define debug(s)
#define debug1(s,a)
#define debug2(s,a,b)
#define debug3(s,a,b,c)
#define debug4(s,a,b,c,d)
#define debug5(s,a,b,c,d,e)
#define debug6(s,a,b,c,d,e,f)
#define debug7(s,a,b,c,d,e,f,g)
#define debug8(s,a,b,c,d,e,f,g,h)
#define debug9(s,a,b,c,d,e,f,g,h,i)

#endif

#ifdef TRACE

#define trace	debug
#define trace1	debug1
#define trace2	debug2
#define trace3	debug3
#define trace4	debug4
#define trace5	debug5
#define trace6	debug6
#define trace7	debug7
#define trace8	debug8
#define trace9	debug9

#else

#define trace
#define trace1
#define trace2
#define trace3
#define trace4
#define trace5
#define trace6
#define trace7
#define trace8
#define trace9

#endif

/*  Standard types used in this program
 */

typedef unsigned char BOOLEAN;

#define FALSE	0
#define TRUE	(!FALSE)

typedef unsigned char BYTE;

typedef unsigned short int WORD;
typedef unsigned long int DWORD;

typedef signed short int SWORD;
typedef signed long int SDWORD;

/*  Routine to display help text.
 */

void help(void)
{
    fprintf( stderr,
	     "knl " VERSION " Program to configure initial kernel settings\n"
	     "Copyright (C) 1998-2002, Riley Williams <Riley@Williams.Name>\n\n"
	     "This program and the associated documentation are distributed under the\n"
	     "GNU General Public Licence (GPL), version 2 only. See the file COPYING\n"
	     "for details.\n\n"
	     "Syntax:  knl [--kernel=]image [-f=flaglist] [--flags=flaglist]\n"
	     "             [--noram] [-p] [--prompt] [--ram=offset] [-r=device]\n"
	     "             [--root=device] [-s=device] [--swap=device] [--help]\n"
	     "             [-v=mode] [--video=mode] [--version]\n" );
    exit( 255 );
}

/*  Routine to decode a value.
 */

DWORD GetValue( char *Ptr, DWORD Max, BYTE *Valid, char OmitOK )
{
    char *Next;
    SDWORD Value = 0;

    trace4("TRACE: GetValue( \"%s\", $lu, %p, %c )\n",
	   Ptr, Max, Valid, OmitOK );
    if (*Ptr) {
	Value = strtol(Ptr,&Next,10);
	if ( (*Next=='\0') && (Value>=0) && (Value<=Max) ) {
	    if ( (Value>0) || (OmitOK!='Y') )
		*Valid = 'Y';
	    else
		*Valid = 'N';
	} else
	    *Valid = 'N';
    } else
	*Valid = OmitOK;
    trace3("TRACE: GetValue returning %ld as %lu with Valid = %c\n",
	   Value, (DWORD) Value, (char) (*Valid));
    return (DWORD) Value;
}

#define GetByte(Ptr,Max,Valid,OK)	(BYTE) GetValue(Ptr,Max,Valid,OK)
#define GetWord(Ptr,Max,Valid,OK)	(WORD) GetValue(Ptr,Max,Valid,OK)

/*  Routine to get arbitrary major and minor numbers.
 */

void GetMajorMinor( char *Ptr, BYTE *Major, BYTE *Minor, BYTE *Valid )
{
    char *Gap, Sep;

    trace4("TRACE: GetMajorMinor( \"%s\", %p, %p, %p )\n",
	   Ptr, Major, Minor, Valid);
    if (*Ptr) {
	if ((Gap = index(Ptr,'.')) != NULL) {
	    Sep = *Gap;
	    *Gap++ = '\0';
	    *Major = GetByte(Ptr,255,Valid,'N');
	    if (*Valid == 'Y')
		*Minor = GetByte(Gap,255,Valid,'N');
	    *--Gap = Sep;
	} else
	    *Valid = 'N';
    } else
	*Valid = 'N';
    trace3("TRACE: GetMajorMinor returned (%d,%d) with Valid = %c\n",
	   (int) (*Major), (int) (*Minor), (char) (*Valid));
}

/* Routine to convert a disk reference into the relevant node numbers.
 * Conversion map:
 *
 * /dev/? name  Major  Minor  Notes
 * ~~~~~~~~~~~  ~~~~~  ~~~~~  ~~~~~
 *   Boot          0      0   Use system boot device.
 *   NFS	   0	255   Select NFS Boot.
 *
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
 *   hdbX          5      Y   X < 64, Y = X + 192         (ELKS)
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

void GetDisk( char *Name, WORD *Disk, BYTE *Valid )
{
    char *Alpha = "abcdefghijklmnopqrstuvwxyz", *Ptr = NULL;
    BYTE Major = 0;
    BYTE Minor = 0;
    BYTE Partition = 0;

    trace3("TRACE: GetDisk( \"%s\", %p, %p )\n",Name,Disk,Valid);
    if (!strcasecmp(Name,"Boot")) {
	*Disk = 0;
	*Valid = 'Y';
	trace("TRACE: Returning 0 with Valid = Y\n");
	return;
    }
    if (!strncasecmp(Name,"/dev/",5)) {
	Name += 5;
	trace("TRACE: Skipping leading /dev/\n");
    }
    *Valid = 'N';
    switch (tolower(*Name)) {
	case 'a':
	    if (!strncasecmp(Name,"aztcd",5)) {
		Major = 29;
		Minor = GetByte(Name+5,255,Valid,'Y');
	    }
	    break;
#ifdef __BCC__
	case 'b':
	    if (!strncasecmp(Name,"bda",3)) {
		Major = 3;
		Minor = GetByte(Name+3,64,Valid,'Y');
	    }
	    break;
#endif
	case 'c':
	    if (!strncasecmp(Name,"cdouble",7)) {
		Major = 19;
		Minor = GetByte(Name+7,127,Valid,'N') + 128;
	    }
	    break;
	case 'd':
	    if (!strncasecmp(Name,"double",6)) {
		Major = 19;
		Minor = GetByte(Name+6,127,Valid,'N');
	    }
	    break;
	case 'f':
	    switch (tolower(Name[1])) {
		case 'd':
		    if (Name[2] == '\0') {
			Major = 2;
			Minor = GetByte(Name+2,3,Valid,'N');
		    }
		    break;
		case 'l':
		    if (!strncasecmp(Name,"flash",5)) {
			Major = 31;
			Minor = GetByte(Name+5,7,Valid,'N') + 16;
		    }
		default:
		    break;
	    }
	    break;
	case 'g':
	    if (!strncasecmp(Name,"gscd",4)) {
		Major = 16;
		Minor = GetByte(Name+4,255,Valid,'Y');
	    }
	    break;
	case 'h':
	    switch (tolower(Name[1])) {
		case 'd':
		    Partition = GetByte(Name+3,63,Valid,'Y');
		    switch (tolower(Name[2]-1)|1) {
			case 'a':
#ifdef __BCC__
			    Major = 5;
#else
			    Major = 3;
#endif
			    break;
			case 'c':
#ifdef __BCC__
			    Major = 5;
			    Partition += 128;
#else
			    Major = 22;
#endif
			    break;
			case 'e':
			    Major = 33;
			    break;
			case 'g':
			    Major = 34;
			    break;
			default:
			    *Valid = 'N';
			    break;
		    }
		    Minor = 64 * ((tolower(Name[2])+1)&1) + Partition;
		    break;
		case 'i':
		    if (!strncasecmp(Name,"hitcd",5)) {
			Major = 20;
			Minor = GetByte(Name+5,255,Valid,'Y');
		    }
		default:
		    break;
	    }
	    break;
	case 'm':
	    switch (tolower(Name[1])) {
		case 'c':
		    if (!strncasecmp(Name,"mcd",3)) {
			Major = 23;
			Minor = GetByte(Name+3,255,Valid,'Y');
		    }
		    break;
		case 'o':
		    if (!strncasecmp(Name,"Mode-",5))
			GetMajorMinor(Name+5,&Major,&Minor,Valid);
		    break;
		default:
		    break;
	    }
	    break;
	case 'o':
	    if (!strncasecmp(Name,"optcd",5)) {
		Major = 17;
		Minor = GetByte(Name+5,255,Valid,'Y');
	    }
	    break;
	case 'r':
	    switch (tolower(Name[1])) {
		case 'a':
		    if (!strncasecmp(Name,"ram",3)) {
			Major = 1;
			if (!strncasecmp(Name+3,"dis",3)) {
			    if (Name[7]=='\0' && index("ck",tolower(Name[6]))!=NULL) {
				*Valid = 'Y';
				Minor = 0;
			    } else
				*Valid = 'N';
			} else
			    Minor = GetByte(Name+3,7,Valid,'M');
		    }
		    break;
		case 'f':
		    if (!strncasecmp(Name,"rflash",6)) {
			Major = 31;
			Minor = GetByte(Name+6,7,Valid,'N') + 24;
		    }
		case 'o':
		    if (!strncasecmp(Name,"rom",3)) {
			Major = 31;
			Minor = GetByte(Name+3,7,Valid,'N');
		    }
		    break;
		case 'r':
		    if (!strncasecmp(Name,"rrom",4)) {
			Major = 31;
			Minor = GetByte(Name+4,7,Valid,'N') + 8;
		    }
		default:
		    break;
	    }
	    break;
	case 's':
	    switch (tolower(Name[1])) {
		case 'c':
		    if (!strncasecmp(Name,"scd",3)) {
			Major = 11;
			Minor = GetByte(Name+3,255,Valid,'Y');
		    }
		    break;
		case 'd':
		    Partition = GetByte(Name+3,15,Valid,'Y');
		    Ptr = index(Alpha,tolower(Name[2]));
		    if (*Valid == 'Y' && Ptr != NULL && (Ptr-Alpha) < 16) {
			Major = 8;
			Minor = 16 * (Ptr - Alpha) + Partition;
		    } else
			*Valid = 'N';
		    break;
		case 'j':
		    if (!strncasecmp(Name,"sjcd",4)) {
			Major = 18;
			Minor = GetByte(Name+4,255,Valid,'Y');
		    }
		    break;
		case 'o':
		    if (!strncasecmp(Name,"sonycd",6)) {
			Major = 15;
			Minor = GetByte(Name+6,255,Valid,'Y');
		    }
		default:
		    break;
	    }
	    break;
	case 'x':
	    if (tolower(Name[1])=='d' && index("ab",tolower(Name[2]))!=NULL) {
		Major = 13;
		Minor = GetByte(Name+3,63,Valid,'Y');
		if (tolower(Name[2])=='b')
		    Minor += 64;
	    }
	    break;
	default:
	    break;
    }
    if (*Valid == 'Y')
	*Disk = (Major << 8) + Minor;
    trace2("TRACE: GetDisk returning %X with Valid = %c\n",
	   (int) (*Disk), *Valid);
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

void GetFlags(char *Ptr, SDWORD *Flags, BYTE *Valid)
{
    SDWORD Result = 0, Value = 0;
    char *Next;

    trace3("TRACE: GetFlags( \"%s\", %p, %p )\n",Ptr,Flags,Valid);
    *Valid = 'Y';
    if (strcasecmp(Ptr,"None"))
	while (Ptr != NULL) {
	    Next = index(Ptr,',');
	    if (Next != NULL)
		*Next++ = '\0';
	    switch (toupper(*Ptr)) {
		case 'R':
		    if (!strcasecmp(Ptr,"RO"))
			Value |= 1;
		    else
			*Valid = 'N';
		    break;
		case 'X':
		    Result = GetByte(Ptr+1,15,Valid,'N');
		    if (*Valid == 'Y')
			Value |= (1 << Result);
		    else
			*Valid = 'N';
		    break;
		default:
		    *Valid = 'N';
	    }
	    Ptr = Next;
	}
    }
    if (*Valid == 'Y')
	*Flags = Value;
    trace2("TRACE: GetFlags returned %ld with Valid = %c\n",
	   (long) (*Flags), Valid);
}

/* Routine to analyse the "--ram=" option and set the offset to the start
 * of the ramdisk image. A maximum of 8,191 blocks is enforced.
 */

void GetRam( char *Ptr, WORD *MaxSize, BYTE *RamOK, BYTE *Valid )
{
    WORD Result;

    trace4("TRACE: GetRAM( \"%p\", %p, %p, %p )\n",Ptr,MaxSize,RamOK, Valid);
    Result = GetWord(Ptr,8191,Valid,'N');
    if (*Valid == 'Y') {
	*RamOK = 'Y';
	*MaxSize = (WORD) Result;
    }
    trace3("TRACE: GetWord returned %u / %c with Valid = %c\n",
	   *MaxSize, *RamOK, *Valid);
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

void GetVideo( char *Ptr, WORD *Video, BYTE *VideoOK, BYTE *Valid )
{
    WORD Result;

    trace4("TRACE: GetVideo( \"%s\", %p, %p, %p )\n", Ptr, Video, VideoOK,
	   Valid);
    switch (toupper(*Ptr)) {
	case 'A':
	    if (!strcasecmp(Ptr,"Ask")) {
		*Valid = *VideoOK = 'Y';
		*Video = (WORD) (-3);
	    }
	    break;
	case 'E':
	    if (!strcasecmp(Ptr,"EVGA")) {
		*Valid = *VideoOK = 'Y';
		*Video = (WORD) (-2);
	    }
	    break;
	case 'V':
	    if (!strcasecmp(Ptr,"VGA")) {
		*Valid = *VideoOK = 'Y';
		*Video = (WORD) (-1);
	    }
	    break;
	default:
	    Result = GetWord(Ptr,65549,Valid,'N');
	    if (*Valid == 'Y') {
		*VideoOK = 'Y';
		*Video = (WORD) Result;
	    }
	    break;
    }
    trace3("TRACE: GetVideo returned %d / %c with Valid = %c\n",
	   (int) (*Video), *VideoOK, *Valid);
}

/*  Routine to decode a disk number into a disk name.
 */

char *SetDisk( WORD Value )
{
    char Result[32];
    BYTE Major = Value >> 8;
    BYTE Minor = Value & 255;

    trace3("TRACE: SetDisk( %X ) = (%d,%d)\n", Value, Major, Minor);
    *Result = '\0';
    switch (Major) {
	case 0:
	    switch (Minor) {
		case 0:
		    strcpy( Result, "Boot" );
		    break;
		case 255:
		    strcpy( Result, "NFS" );
		default:
		    break;
	    }
	    break;
	case 1:
	    if (Minor < 8)
		sprintf( Result, "/dev/ram%u", Minor );
	    break;
	case 2:
	    strcpy( Result, "/dev/fd0" );
	    Result[7] += Minor % 4;
	    if (Minor > 4)
		sprintf( Result+8, "* (Mode-4.%u)", Minor );
	    break;
	case 3:
	    if (Minor < 128) {
#ifdef __BCC__
		sprintf( Result, "/dev/bda%u", Minor % 64 );
#else
		sprintf( Result, "/dev/hda%u", Minor % 64 );
#endif
		if (Minor & 64)
		    Result[7]++;
#ifdef __BCC__
		if (Minor & 128)
		    Result[7] += 2;
#endif
		if (Minor % 64 == 0)
		    Result[8] = '\0';
	    }
	    break;
	case 8:
	    sprintf( Result, "/dev/sda%u", Minor % 16 );
	    Result[7] += Minor / 16;
	    if (Minor % 16 == 0)
		Result[8] = '\0';
	    break;
	case 11:
	    sprintf( Result, "/dev/scd%u", Minor );
	    if (!Minor)
		Result[8] = '\0';
	    break;
	case 13:
	    if (Minor < 128) {
		sprintf( Result, "/dev/xda%u", Minor % 64 );
		if (Minor & 64)
		    Result[7]++;
		if (Minor % 64 == 0)
		    Result[8] = '\0';
	    }
	    break;
	case 15:
	    sprintf( Result, "/dev/sonycd%u", Minor );
	    if (!Minor)
		Result[11] = '\0';
	    break;
	case 16:
	    sprintf( Result, "/dev/gscd%u", Minor );
	    if (!Minor)
		Result[9] = '\0';
	    break;
	case 17:
	    sprintf( Result, "/dev/optcd%u", Minor );
	    if (!Minor)
		Result[10] = '\0';
	    break;
	case 18:
	    sprintf( Result, "/dev/sjcd%u", Minor );
	    if (!Minor)
		Result[9] = '\0';
	    break;
	case 19:
	    if (Minor > 128)
		sprintf( Result, "/dev/cdouble%u", Minor-128 );
	    else
		sprintf( Result, "/dev/double%u", Minor );
	    break;
	case 20:
	    sprintf( Result, "/dev/hitcd%u", Minor );
	    if (!Minor)
		Result[10] = '\0';
	    break;
#ifndef __BCC__
	case 22:
	    if (Minor < 128) {
		sprintf( Result, "/dev/hdc%u", Minor % 64 );
		if (Minor & 64)
		    Result[7]++;
		if (Minor % 64 == 0)
		    Result[8] = '\0';
	    }
	    break;
#endif
	case 23:
	    sprintf( Result, "/dev/mcd%u", Minor );
	    if (!Minor)
		Result[8] = '\0';
	    break;
	case 29:
	    sprintf( Result, "/dev/aztcd%u", Minor );
	    if (!Minor)
		Result[10] = '\0';
	    break;
	case 31:
	    switch (Minor >> 3) {
		case 0:
		    sprintf( Result, "/dev/rom%u", Minor );
		    break;
		case 1:
		    sprintf( Result, "/dev/rrom%u", Minor );
		    break;
		case 2:
		    sprintf( Result, "/dev/flash%u", Minor );
		    break;
		case 3:
		    sprintf( Result, "/dev/rflash%u", Minor );
		default:
		    break;
	    }
	    break;
	case 33:
	    if (Minor < 128) {
		sprintf( Result, "/dev/hde%u", Minor % 64 );
		if (Minor & 64)
		    Result[7]++;
		if (Minor % 64 == 0)
		    Result[8] = '\0';
	    }
	    break;
	case 34:
	    if (Minor < 128) {
		sprintf( Result, "/dev/hdg%u", Minor % 64 );
		if (Minor & 64)
		    Result[7]++;
		if (Minor % 64 == 0)
		    Result[8] = '\0';
	    }
	default:
	    break;
    }
    if (*Result != '\0')
	sprintf( Result, "Mode-%u.%u", Major, Minor );
    trace1("TRACE: SetDisk returned \"%s\"\n", Result);
    return strdup( Result );
}

/* Routine to decode the flag word into a string.
 */

char *SetFlags( WORD Flags )
{
    char Result[256];
    BYTE i;

    trace1("TRACE: SetFlags( %X )\n", Flags);
    *Result = '\0';
    for ( i=16; i--; )
	if (Flags & (1<<i) )
	    switch (i) {
		case 0:
		    strcat( Result, ",RO" );
		    break;
		default:
		    sprintf( Result, "%s,X%u", Result, i );
		    break;
	    }
    if (*Result == '\0')
	strcpy( Result, " None" );
    trace1("TRACE: SetFlags returned \"%s\"\n", Result+1);
    return strdup(Result+1);
}

/* Routine to decode a video mode into a string.
 */

char *SetVideo( WORD Value )
{
    char Buffer[32], *Result;

    trace1("TRACE: SetVideo( %d )\n", Value);
    if (Value > 65499) {
	Value -= 32768;
	Value = 32768 - Value;
	switch (Value) {
	    case 0:
		Result = "0 (Normally 80x25)";
		break;
	    case 1:
		Result = "VGA";
		break;
	    case 2:
		Result = "XVGA";
		break;
	    case 3:
		Result = "Ask";
		break;
	    default:
		Result = Buffer;
		sprintf( Result, "Undefined (-%u)", Value );
		break;
	}
    } else {
	Result = Buffer;
	sprintf( Result, "%u", Value );
    }
    trace1("TRACE: SetVideo returned \"%s\"\n", Result);
    return Result;
}

/* File buffer
 */

#define BufStart	0x1F0
#define BufSize		8

WORD Buffer[BufSize] = { 1, 2, 3, 4, 5, 6, 7, 8 };

#define BufFlags	Buffer[1]
#define BufSwap		Buffer[3]
#define BufRAM		Buffer[4]
#define BufVideo	Buffer[5]
#define BufRoot		Buffer[6]
#define BufSignature	Buffer[7]

/****************/
/* Main program */
/****************/

int main(int c, char **v)
{
    char *Image, *Ptr, *Value;
    FILE *fp;
    SDWORD Flags = -1;
    DWORD i = 0;
    WORD RamOffset = 0;
    WORD RootDev = 0;
    WORD SwapDev = 0;
    WORD VideoMode = 0;
    BYTE Display = 'Y';
    BYTE RamOK = 'N';
    BYTE RamPrompt = 'N';
    BYTE Valid = 'N';
    BYTE VideoOK = 'N';

    if (c == 1)
	help();
    else {
	Image = NULL;
	for (i=1; i<c; i++) {
	    Ptr = v[i];
	    trace1("TRACE: Analysing \"%s\", Ptr);
	    if (*Ptr != '-') {
		Image = Ptr;
		Valid = 'Y';
		debug1("DEBUG: Kernel image: %s\n", Ptr);
	    } else if (*(++Ptr) != '-') {
		trace("TRACE: Analysing short options.\n");
		Valid = 'N';
		switch (tolower(*Ptr++)) {
		    case 'f':
			if (*Ptr == '=') {
			    GetFlags( ++Ptr, &Flags, &Valid );
			    if (Valid == 'Y') {
				debug1("DEBUG: Flags set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid flags: %s\n", Ptr);
			    }
			}
			break;
		    case 'p':
			if (!*Ptr) {
			    RamOK = RamPrompt = Valid = 'Y';
			    debug("DEBUG: Initial RamDisk enabled and prompting selected.\n");
			}
			break;
		    case 'r':
			if (*Ptr == '=') {
			    GetDisk( ++Ptr, &RootDev, &Valid );
			    if (Valid == 'Y') {
				debug1("DEBUG: Root device set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid root device: %s\n", Ptr);
			    }
			}
			break;
		    case 's':
			if (*Ptr == '=') {
			    GetDisk( ++Ptr, &SwapDev, &Valid );
			    if (Valid == 'Y') {
				debug1("DEBUG: Swap device set to %s\n", Ptr);
			    } else {
				debug1("DEBUG: Invalid swap device: %s\n", Ptr);
			    }
			}
			break;
		    case 'v':
			if (*Ptr == '=') {
			    GetVideo( ++Ptr, &VideoMode, &VideoOK, &Valid );
			    if (Valid == 'Y') {
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
		Valid = 'N';
		if ( (Value = index(Ptr,'=')) != NULL)
		    *Value++ = '\0';
		switch (* ++Ptr) {
		    case 'f':
			if (!strcmp(Ptr,"flags")) {
			    GetFlags(Value, &Flags, &Valid);
			    if (Valid == 'Y') {
				debug1("DEBUG: Flags set to %s\n", Value);
			    } else {
				debug1("DEBUG: Invalid flags: %s\n", Value);
			    }
			}
			break;
		    case 'h':
			if (!strcmp(Ptr,"help") && (Value == NULL))
			    help();
			break;
		    case 'k':
			if (!strcmp(Ptr,"kernel") && (Value != NULL)) {
			    Image = Value;
			    Valid = 'Y';
			    debug1("DEBUG: Kernel image: %s\n", Value);
			}
			break;
		    case 'n':
			if (!strcmp(Ptr,"noram") && (Value == NULL)) {
			    RamOffset = 0;
			    RamOK = 'N';
			    RamPrompt = Valid = 'Y';
			    debug("DEBUG: Initial RamDisk disabled.\n");
			}
			break;
		    case 'p':
			if (!strcmp(Ptr,"prompt") && (Value == NULL)) {
			    RamOK = RamPrompt = Valid = 'Y';
			    debug("DEBUG: Initial RamDisk enabled and prompting selected.\n");
			}
			break;
		    case 'r':
			if (Value != NULL)
			    switch (Ptr[1]) {
				case 'a':
				    if (!strcmp(Ptr,"ram")) {
					GetRam(Value, &RamOffset, &RamOK, &Valid);
					if (Valid == 'Y') {
					    debug1("DEBUG: Initial RamDisk enabled with offset set to %s\n", Value);
					} else {
					    debug1("DEBUG: Invalid initial RamDisk offset: %s\n", Value);
					}
				    }
				    break;
				case 'o':
				    if (!strcmp(Ptr,"root")) {
					GetDisk(Value, &RootDev, &Valid);
					if (Valid == 'Y') {
					    debug1("DEBUG: Root device set to %s\n", Value);
					} else {
					    debug1("DEBUG: Invalid root device: %s\n", Value);
					}
				    }
				    break;
			    }
			break;
		    case 's':
			if (!strcmp(Ptr,"swap") && (Value != NULL)) {
			    GetDisk(Value, &SwapDev, &Valid);
			    if (Valid == 'Y') {
				debug1("DEBUG: Swap device set to %s\n", Value);
			    } else {
				debug1("DEBUG: Invalid swap device: %s\n", Value);
			    }
			}
			break;
		    case 'v':
			switch (Ptr[1]) {
			    case 'e':
				if (!strcasecmp(Ptr,"version") && (Value == NULL)) {
				    fprintf( stderr, "knl v%s", VERSION );
				    exit( 255 );
				}
				break;
			    case 'i':
				if (!strcmp(Ptr,"video") && (Value != NULL)) {
				    GetVideo(Value, &VideoMode, &VideoOK, &Valid);
				    if (Valid == 'Y') {
					debug1("DEBUG: Video mode set to %s\n", Value);
				    } else {
					debug1("DEBUG: Invalid video mode: %s\n", Value);
				    }
				}
				break;
			}
			break;
		}
		if (Valid == 'Y') {
		    if (strcmp(Ptr,"kernel"))
			Display = 'N';
		} else
		    if (Value != NULL)
			*--Value = '=';
	    }
	    if (Valid == 'N')
		fprintf(stderr, "knl: Ignoring invalid option: %s\n", v[i]);
	}
	if (Image == NULL) {
	    fprintf( stderr, "knl: (1) Kernel image file not specified.\n" );
	    exit( 1 );
	}
	if ( (fp = fopen(Image,"r")) == NULL) {
	    fprintf( stderr, "knl: (2) Kernel image file not found.\n" );
	    exit( 2 );
	}
	fseek( fp, (long) BufStart, SEEK_SET );
	fread( Buffer, sizeof(WORD), BufSize, fp );
	if (BufSignature != 0xAA55) {
	    fprintf( stderr, "knl: (3) File is not a kernel image.\n" );
	    exit( 3 );
	}
	if (Display == 'Y') {
	    Flags = BufFlags;
	    RamOffset = BufRAM & 0x0FFF;
	    RamPrompt = (BufRAM & 0x8000) ? 'Y' : 'N';
	    RamOK = (BufRAM & 0x4000) ? 'Y' : 'N';
	    VideoMode = BufVideo;
	    RootDev = BufRoot;
	    SwapDev = BufSwap;
	    printf( "\nKernel image configuration:\n\n"
		    "    Image:       %s\n\n"
		    "    Root Dev:    %s\n"
		    "    Swap Dev:    %s\n\n"
		    "    Flags:       %s\n"
		    "    Video Mode:  %s\n\n",
		    Image, SetDisk(RootDev), SetDisk(SwapDev),
		    SetFlags(Flags), SetVideo(VideoMode) );
	    if (RamOK == 'N')
		printf( "    Ram Disk:    No\n" );
	    else
		printf( "    Ram Disk:    Yes\n"
			"        Offset:  %u\n"
			"        Prompt:  %c\n",
			RamOffset, RamPrompt );
	} else {
	    fclose( fp );
	    if ( (fp = fopen(Image,"rb+")) == NULL) {
		fprintf( stderr,
			 "knl: (4) Kernel image file not updatable.\n" );
		exit( 4 );
	    }
	    if (Flags != -1)
		BufFlags = (WORD) Flags;
	    if (RamOK == 'Y')
		BufRAM = ( (RamOK=='Y') ? 0x4000 : 0) |
			 ( (RamPrompt='Y') ? 0x8000 : 0) | RamOffset;
	    if (RootDev != 0)
		BufRoot = RootDev;
	    if (SwapDev != 0)
		BufSwap = SwapDev;
	    if (VideoOK == 'Y')
		BufVideo = VideoMode;
	    fseek( fp, (long) BufStart, SEEK_SET );
	    if (fwrite(Buffer,sizeof(WORD),BufSize,fp) != BufSize) {
		fprintf( stderr, "knl: (5) Kernel image not updated.\n" );
		exit( 5 );
	    }
	}
	fclose( fp );
    }
    return 0;
}
