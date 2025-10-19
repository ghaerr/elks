#ifndef EXT_H
#define EXT_H
/*
 * Copyright (C) 1995, 1996, 1997 Wolfgang Solfrank
 * Copyright (c) 1995 Martin Husemann
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Martin Husemann
 *	and Wolfgang Solfrank.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *	$NetBSD: ext.h,v 1.6 2000/04/25 23:02:51 jdolecek Exp $
 * $FreeBSD: src/sbin/fsck_msdosfs/ext.h,v 1.10.20.1 2009/04/15 03:14:26 kensmith Exp $
 */

#include <sys/types.h>
#include <limits.h>

#define MAXPATHLEN  PATH_MAX
#define	LOSTDIR     "LOST.DIR"

typedef unsigned int    u_int;      /* FIXME possibly change to 32-bit for ELKS */
typedef unsigned char   u_char;
typedef unsigned long   u_int32_t;
typedef unsigned long   U32;

typedef off_t           loff_t;
#define lseek64         lseek

#define pwarn printf
#define pfatal printf

#include "dosfs.h"

/*
 * Options:
 */
extern int alwaysno;	/* assume "no" for all questions */
extern int alwaysyes;	/* assume "yes" for all questions */
extern int preen;	/* we are preening */
extern int rdonly;	/* device is opened read only (supersedes above) */
extern int skipclean;	/* skip clean file systems if preening */

extern struct dosDirEntry *rootDir;

#if defined(__COMPACT__) || defined(__LARGE__)
void *hmalloc(unsigned long size);
void *hcalloc(unsigned long count, size_t size);
#define MAXCLUSTERS     31752
#else
#define hmalloc(n)      malloc(n)
#define hcalloc(c, s)   calloc(c, s)
#define MAXCLUSTERS     3969
#endif

/*
 * function declarations
 */
int ask(int, const char *, ...);

/*
 * Check the dirty flag.  If the file system is clean, then return 1.
 * Otherwise, return 0 (this includes the case of FAT12 file systems --
 * they have no dirty flag, so they must be assumed to be unclean).
 */
int checkdirty(int, struct bootblock *);

/*
 * Check file system given as arg
 */
int checkfilesys(const char *);

/*
 * Return values of various functions
 */
#define	FSOK		0		/* Check was OK */
#define	FSBOOTMOD	1		/* Boot block was modified */
#define	FSDIRMOD	2		/* Some directory was modified */
#define	FSFATMOD	4		/* The FAT was modified */
#define	FSERROR		8		/* Some unrecovered error remains */
#define	FSFATAL		16		/* Some unrecoverable error occured */
#define FSDIRTY		32		/* File system is dirty */
#define FSFIXFAT	64		/* Fix file system FAT */

/*
 * read a boot block in a machine independend fashion and translate
 * it into our struct bootblock.
 */
int readboot(int, struct bootblock *);

/*
 * Correct the FSInfo block.
 */
int writefsinfo(int, struct bootblock *);

/*
 * Read one of the FAT copies and return a pointer to the new
 * allocated array holding our description of it.
 */
int readfat(int, struct bootblock *, int, struct fatEntry **);

/*
 * Check two FAT copies for consistency and merge changes into the
 * first if neccessary.
 */
int comparefat(struct bootblock *, struct fatEntry __huge *, struct fatEntry __huge *, int);

/*
 * Check a FAT
 */
int checkfat(struct bootblock *, struct fatEntry __huge *);

/*
 * Write back FAT entries
 */
int writefat(int, struct bootblock *, struct fatEntry __huge *, int);

/*
 * Read a directory
 */
int resetDosDirSection(struct bootblock *, struct fatEntry __huge *);
void finishDosDirSection(void);
int handleDirTree(int, struct bootblock *, struct fatEntry *);

/*
 * Cross-check routines run after everything is completely in memory
 */
/*
 * Check for lost cluster chains
 */
int checklost(int, struct bootblock *, struct fatEntry __huge *);
/*
 * Try to reconnect a lost cluster chain
 */
int reconnect(int, struct bootblock *, struct fatEntry __huge *, cl_t);
void finishlf(void);

/*
 * Small helper functions
 */
/*
 * Return the type of a reserved cluster as text
 */
char *rsrvdcltype(cl_t);

/*
 * Clear a cluster chain in a FAT
 */
void clearchain(struct bootblock *, struct fatEntry __huge *, cl_t);

#endif
