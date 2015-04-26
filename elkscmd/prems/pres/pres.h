/*
 * PREM/MALPREM compression/decompression (c) RL 1990-2015
 *
 * Definition for the applications dealing with the format
 *
 * Compression is applied to blocks of fixed length BLON
 * (the last one can be shorter)
 *
 * The beginning of a compressed block is aligned to a byte boundary
 *
 * Compressed (possibly unchanged) block format:
 *  - magic pattern byte                             (ВМARK)
 *    containing a compressed/uncompressed flag bit  (PREMITA)
 *    and a last/not-last flag bit                   (LASTA)
 *  - bitwise inversion of the same byte
 *  - block number - two bytes in LITTLE ENDIAN order
 *    (при BLON=8192 the maximum data length will be 512MB)
 *    - in the last block - the uncompressed block data length, 2 bytes
 *      the last block length is 0..BLON-1
 *  - data
 *
 * $Header: pres.h,v 1.1 90/12/18 16:22:56 pro Exp $
 * $Log:	pres.h,v $
 * Revision 1.1  90/12/18  16:22:56  pro
 * Initial revision
 * 
 */

#ifdef lint
#define void int
#endif

#ifndef BLON
#define BLON    8192          /* Processing block length            */
#endif
#define BMARK   0100          /* Block begin mark                   */
#define PREMITA 1             /* Compressed flag                    */
#define LASTA   2             /* Last block flag                    */
