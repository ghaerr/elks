/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  WHEN YOU FIGURE OUT WHAT THIS FILE DOES, PLEASE
*               DESCRIBE IT HERE!
*
****************************************************************************/


/* definitions used throughout fcenable */
#include <stdio.h>
#include "bool.h"
#include "watcom.h"

#define MAX_OBJECT_REC_SIZE 4096

#define IOERROR         ((size_t)-1)

typedef unsigned char   byte;
typedef unsigned short  ushort;

typedef unsigned short  omf_index;

// these are the results returned from ReadRec.

typedef enum {
    ERROR = -1,
    OK = 0,
    ENDFILE,
    ENDMODULE,
    LIBRARY,
    OBJECT,
    ENDLIBRARY
} rec_status;

typedef struct name_list    NAME_LIST;
typedef struct exclude_list EXCLUDE_LIST;

typedef struct name_list {
    NAME_LIST   *next;
    omf_index   lnameidx;       // index of lname record which equals name
    char        name[1];
} name_list;

typedef struct exclude_list {
    EXCLUDE_LIST *  next;
    omf_index       lnameidx;   // index of segment lname record
    omf_index       segidx;     // index of segment
    unsigned long   start_off;  // starting offset
    unsigned long   end_off;    // ending offset;
    char            name[1];
} exclude_list;

extern long         PageLen;
extern FILE         *InFile;
extern FILE         *OutFile;
extern name_list    *ClassList;
extern name_list    *SegList;
extern exclude_list *ExcludeList;

// fcenable.c
extern void         LinkList( void **, void * );
extern void         FreeList( void * );
extern void         Warning( const char * );
extern void         Error( const char * );
extern void         IOError( const char * );
extern size_t       QRead( FILE *, void *, size_t );
extern size_t       QWrite( FILE *, const void *, size_t );
extern int          QSeek( FILE *, long, int );

// mem.c
extern void         MemInit( void );
extern void         MemFini( void );
extern void         *MemAlloc( size_t );
extern void         MemFree( void * );

// records.c
extern void         *InitRecStuff( void );
extern void         FileCleanup( void );
extern void         CleanRecStuff( void );
extern void         FinalCleanup( void );
extern void         ProcessRec( void );
extern omf_index    GetIndex( const byte ** );
extern rec_status   ReadRec( void );
extern void         BuildRecord( const void *, size_t );
extern void         FlushBuffer( void );
extern void         IndexRecord( omf_index );
