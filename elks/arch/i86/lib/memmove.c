/*******************************************************************
 * memmove function for far data. The number of bytes transferred  *
 * must be <= 32767 ( ie. fit in an signed int ). That's because   *
 * of ( you guessed it... ) the braindead segmented architecture   *
 * Saku Airila 1996                                                *
 *******************************************************************/

#include <linuxmt/mm.h>

void blt_forth(unsigned,unsigned,unsigned,unsigned,unsigned);

void far_memmove(unsigned sseg, unsigned soff, unsigned dseg, unsigned doff,
		 unsigned bytes)
{
    if (bytes) {
	sseg += (soff >> 4);
	soff &= 0xf;
	dseg += (doff >> 4);
	doff &= 0xf;

	if ((sseg < dseg) || ((sseg == dseg) && (soff < doff))) {
	    --bytes;
	    blt_forth(soff+bytes, sseg, doff+bytes, dseg, bytes+1);
	} else {
	    fmemcpy(dseg, doff, sseg, soff, bytes );
	}
    }
}

