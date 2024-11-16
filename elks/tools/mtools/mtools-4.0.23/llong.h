#ifndef MTOOLS_LLONG_H
#define MTOOLS_LLONG_H

/*  Copyright 1999,2001-2004,2007-2009 Alain Knaff.
 *  This file is part of mtools.
 *
 *  Mtools is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Mtools is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Mtools.  If not, see <http://www.gnu.org/licenses/>.
 */

#if 1


#ifdef HAVE_OFF_T_64
/* if off_t is already 64 bits, be happy, and don't worry about the
 * loff_t and llseek stuff */
# define MT_OFF_T off_t
# if SIZEOF_SIZE_T == 4
/* Some systems (NetBSD) apparently have 64 bit off_t, but 32 bit size_t ... */
#  define MT_SIZE_T off_t
# else
#  define MT_SIZE_T size_t
# endif
#endif

#ifndef MT_OFF_T
# if defined(HAVE_LLSEEK) || defined(HAVE_LSEEK64)
/* we have llseek. Now, what's its type called? loff_t or offset_t ? */
#  ifdef HAVE_LOFF_T
#   define MT_OFF_T loff_t
/* use the same type for size. Better to get signedness wrong than width */
#   define MT_SIZE_T loff_t
#  else
#   ifdef HAVE_OFFSET_T
#    define MT_OFF_T offset_t
/* use the same type for size. Better to get signedness wrong than width */
#    define MT_SIZE_T offset_t
#   endif
#  endif
# endif
#endif

#ifndef MT_OFF_T
/* we still don't have a suitable mt_off_t type...*/
# ifdef HAVE_LONG_LONG
/* ... first try long long ... */
#  define MT_OFF_T long long
#  define MT_SIZE_T unsigned long long
# else
#  ifdef HAVE_OFF64_T
#   define MT_OFF_T off64_t
#   define MT_SIZE_T off64_t
#  else
/* ... and if that fails, fall back on good ole' off_t */
#   define MT_OFF_T off_t
#   define MT_SIZE_T size_t
#  endif
# endif
#endif

typedef MT_OFF_T mt_off_t;
typedef MT_SIZE_T mt_size_t;

#else
/* testing: meant to flag dubious assignments between 32 bit length types
 * and 64 bit ones */
typedef struct {
	unsigned int lo;
	int high;
} *mt_off_t;

typedef struct {
	unsigned int lo;
	unsigned int high;
} *mt_size_t;

#endif

#define min(a,b) ((a) < (b) ? (a) : (b))
#define MAX_OFF_T_B(bits) \
	((((mt_off_t) 1 << min(bits-1, sizeof(mt_off_t)*8 - 2)) -1) << 1 | 1)

#if defined(HAVE_LLSEEK) || defined(HAVE_LSEEK64)
# define SEEK_BITS 63
#else
# define SEEK_BITS (sizeof(off_t) * 8 - 1)
#endif

extern const mt_off_t max_off_t_31;
extern const mt_off_t max_off_t_41;
extern const mt_off_t max_off_t_seek;

extern off_t truncBytes32(mt_off_t off);
extern int fileTooBig(mt_off_t off);

int mt_lseek(int fd, mt_off_t where, int whence);

unsigned int log_2(int);

#endif
