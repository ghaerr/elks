/*
 *	Buffer operations
 */

#include <linuxmt/types.h>
#include <arch/bitops.h>
#include <linuxmt/fs.h>

void brelse(register struct buffer_head *bh)
{
    if (bh)
	__brelse(bh);
}
