/*
 *	Buffer operations
 */
 
#include <linuxmt/types.h>
#include <arch/bitops.h>
#include <linuxmt/fs.h>

extern void brelse(bh)
register struct buffer_head *bh;
{
	if(bh)
		__brelse(bh);
}
#if 0
extern void bforget(bh)
struct buffer_head *bh;
{
	if(bh)
		__bforget(bh);
}

extern struct inode *iget(sb,nr)
struct super_block *sb;
long nr;
{
	return __iget(sb,nr,1);
}
#endif
