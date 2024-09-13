#ifndef _INTTYPES_H
#define _INTTYPES_H
/* For application compatibility, not used in ELKS C Library */

/* Copyright (C) 1997-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* Modified for Dev86 by Jody Bruchon <jody@jodybruchon.com>
   Note that Dev86 does not support 64-bit data types! */

/*
 *	ISO C99: 7.8 Format conversion of integer types	<inttypes.h>
 */

/* Get the type definitions.  */
#include <stdint.h>

/* Macros for printing format specifiers.  */

/* Decimal notation.  */
# define PRId8		"d"
# define PRId16		"d"
# define PRId32		"d"

# define PRIdLEAST8	"d"
# define PRIdLEAST16	"d"
# define PRIdLEAST32	"d"

# define PRIdFAST8	"d"
# define PRIdFAST16	"d"
# define PRIdFAST32	"d"


# define PRIi8		"i"
# define PRIi16		"i"
# define PRIi32		"i"

# define PRIiLEAST8	"i"
# define PRIiLEAST16	"i"
# define PRIiLEAST32	"i"

# define PRIiFAST8	"i"
# define PRIiFAST16	"i"
# define PRIiFAST32	"i"

/* Octal notation.  */
# define PRIo8		"o"
# define PRIo16		"o"
# define PRIo32		"o"

# define PRIoLEAST8	"o"
# define PRIoLEAST16	"o"
# define PRIoLEAST32	"o"

# define PRIoFAST8	"o"
# define PRIoFAST16	"o"
# define PRIoFAST32	"o"

/* Unsigned integers.  */
# define PRIu8		"u"
# define PRIu16		"u"
# define PRIu32		"u"

# define PRIuLEAST8	"u"
# define PRIuLEAST16	"u"
# define PRIuLEAST32	"u"

# define PRIuFAST8	"u"
# define PRIuFAST16	"u"
# define PRIuFAST32	"u"

/* lowercase hexadecimal notation.  */
# define PRIx8		"x"
# define PRIx16		"x"
# define PRIx32		"x"

# define PRIxLEAST8	"x"
# define PRIxLEAST16	"x"
# define PRIxLEAST32	"x"

# define PRIxFAST8	"x"
# define PRIxFAST16	"x"
# define PRIxFAST32	"x"

/* UPPERCASE hexadecimal notation.  */
# define PRIX8		"X"
# define PRIX16		"X"
# define PRIX32		"X"

# define PRIXLEAST8	"X"
# define PRIXLEAST16	"X"
# define PRIXLEAST32	"X"

# define PRIXFAST8	"X"
# define PRIXFAST16	"X"
# define PRIXFAST32	"X"


/* Macros for printing `intmax_t' and `uintmax_t'.  */
# define PRIdMAX	"d"
# define PRIiMAX	"i"
# define PRIoMAX	"o"
# define PRIuMAX	"u"
# define PRIxMAX	"x"
# define PRIXMAX	"X"


/* Macros for printing `intptr_t' and `uintptr_t'.  */
# define PRIdPTR	"d"
# define PRIiPTR	"i"
# define PRIoPTR	"o"
# define PRIuPTR	"u"
# define PRIxPTR	"x"
# define PRIXPTR	"X"


/* Macros for scanning format specifiers.  */

/* Signed decimal notation.  */
# define SCNd8		"hhd"
# define SCNd16		"hd"
# define SCNd32		"d"

# define SCNdLEAST8	"hhd"
# define SCNdLEAST16	"hd"
# define SCNdLEAST32	"d"

# define SCNdFAST8	"hhd"
# define SCNdFAST16	"d"
# define SCNdFAST32	"d"

/* Signed decimal notation.  */
# define SCNi8		"hhi"
# define SCNi16		"hi"
# define SCNi32		"i"

# define SCNiLEAST8	"hhi"
# define SCNiLEAST16	"hi"
# define SCNiLEAST32	"i"

# define SCNiFAST8	"hhi"
# define SCNiFAST16	"i"
# define SCNiFAST32	"i"

/* Unsigned decimal notation.  */
# define SCNu8		"hhu"
# define SCNu16		"hu"
# define SCNu32		"u"

# define SCNuLEAST8	"hhu"
# define SCNuLEAST16	"hu"
# define SCNuLEAST32	"u"

# define SCNuFAST8	"hhu"
# define SCNuFAST16	"u"
# define SCNuFAST32	"u"

/* Octal notation.  */
# define SCNo8		"hho"
# define SCNo16		"ho"
# define SCNo32		"o"

# define SCNoLEAST8	"hho"
# define SCNoLEAST16	"ho"
# define SCNoLEAST32	"o"

# define SCNoFAST8	"hho"
# define SCNoFAST16	"o"
# define SCNoFAST32	"o"

/* Hexadecimal notation.  */
# define SCNx8		"hhx"
# define SCNx16		"hx"
# define SCNx32		"x"

# define SCNxLEAST8	"hhx"
# define SCNxLEAST16	"hx"
# define SCNxLEAST32	"x"

# define SCNxFAST8	"hhx"
# define SCNxFAST16	"x"
# define SCNxFAST32	"x"


/* Macros for scanning `intmax_t' and `uintmax_t'.  */
# define SCNdMAX	"d"
# define SCNiMAX	"i"
# define SCNoMAX	"o"
# define SCNuMAX	"u"
# define SCNxMAX	"x"

/* Macros for scaning `intptr_t' and `uintptr_t'.  */
# define SCNdPTR	"d"
# define SCNiPTR	"i"
# define SCNoPTR	"o"
# define SCNuPTR	"u"
# define SCNxPTR	"x"


#endif
