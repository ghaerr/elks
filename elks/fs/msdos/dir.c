/*
 *  linux/fs/msdos/dir.c
 *
 *  Written 1992 by Werner Almesberger
 *
 *  MS-DOS directory handling functions
 */
/* ported from linx-0.97 by zouys, Oct 21st, 2010 */
/* neated from linx-2.0.34 fangzs, Nov 23rd, 2010 */

#include <linuxmt/fs.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>
#include <linuxmt/string.h>
#include <linuxmt/debug.h>

#include <arch/segment.h>

static size_t msdos_dir_read(struct inode *dir, struct file *filp, char *buf, size_t count)
{
    return -EISDIR;
}

static int msdos_readdir(struct inode *dir, struct file *filp,
			 char *dirbuf, filldir_t filldir);

static struct file_operations msdos_dir_operations = {
	NULL,			/* lseek - default */
	msdos_dir_read,		/* read */
	NULL,			/* write - bad */
	msdos_readdir,		/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* no special open code */
	NULL			/* no special release code */
};

/*
 * directories can handle most operations...
 */
struct inode_operations msdos_dir_inode_operations = {
	&msdos_dir_operations,	/* default directory file-ops */
	msdos_create,		/* create */
	msdos_lookup,		/* lookup */
	NULL,			/* link */
	msdos_unlink,		/* unlink */
	NULL,			/* symlink */
	msdos_mkdir,		/* mkdir */
	msdos_rmdir,		/* rmdir */
	NULL,			/* mknod */
	NULL,			/* readlink */
	NULL,			/* follow_link */
	NULL,			/* getblk */
	NULL			/* truncate */
};

static int FATPROC
unicode_to_ascii(char *ascii, unsigned char *uni)
{
	char *op = ascii;

	while(*uni && *uni!=0xff){
		*op++ = *uni;
		uni += 2;
	}
	*op = 0;
	return (op - ascii);
}

/*
 * Read next directory entry into name/namelen
 *	returns 1 on short filename
 *	returns 2 on long filename
 */
int FATPROC msdos_get_entry_long(
	struct inode *dir,		/* scan this directory*/
	off_t *pos,				/* starting at this offset*/
	struct buffer_head **bh,/* using this buffer*/
	char *name,				/* return entry name*/
	int *namelen,			/* return entry length*/
	off_t *dirpos,			/* return entry directory position*/
	ino_t *ino)				/* return entry inode*/
{
	struct msdos_dir_entry *de;
	off_t oldpos = *pos;	/* location of next directory entry start*/
	int is_long;
	unsigned char alias_checksum = 0;
	ASYNCIO_REENTRANT unsigned short unicodename[26+1];	/* Limited to two long entries */

	if ((int)*pos & (sizeof(struct msdos_dir_entry) - 1)) return -ENOENT;
	is_long = 0;
	*ino = msdos_get_entry(dir,pos,bh,&de);
	debug_fat("get_entry_long block %lu\n", EBH(*bh)->b_blocknr);
	while (*ino != (ino_t)-1L) {
		if (de->name[0] == 0) {		/* empty  entry and stop reading*/
			break;
		} else if ((unsigned char)de->name[0] == DELETED_FLAG) {	/* empty entry*/
			is_long = 0;
			oldpos = *pos;
		} else if (de->attr ==  ATTR_EXT) {		/* long filename entry*/
			int slot = 0;
			register struct msdos_dir_slot *ds = (struct msdos_dir_slot *) de;

			if (ds->id & LAST_LONG_ENTRY) {		/* last entry is first*/
				slot = ds->id & ~LAST_LONG_ENTRY;
				is_long = 1;
				alias_checksum = ds->alias_checksum;
			}

			unicodename[26] = 0;		        /* nul terminate unicode buffer*/
			while (slot > 0) {					/* read entries until slot 0*/
				if (ds->attr != ATTR_EXT ||
					(ds->id & ~LAST_LONG_ENTRY) != slot ||
					 ds->alias_checksum != alias_checksum) {
					is_long = 0;
					break;
				}
				/*
				 * Only save last two slot contents as limited to 14 char filenames
				 */
				slot--;
				if (slot < 2) {
					int offset = slot * 13;		/* 13 chars/entry*/
					memcpy(&unicodename[offset], ds->name0_4, 10);
					offset += 5;
					memcpy(&unicodename[offset], ds->name5_10, 12);
					offset += 6;
					memcpy(&unicodename[offset], ds->name11_12, 4);
					offset += 2;
					if (ds->id & LAST_LONG_ENTRY)
						unicodename[offset] = 0;
				}
				if (slot > 0) {
					*ino = msdos_get_entry(dir,pos,bh,&de);
					if (*ino == (ino_t)-1L) {
						is_long = 0;
						break;
					}
					ds = (struct msdos_dir_slot *)de;
				}
			}
		} else if (de->name[0] && !(de->attr & ATTR_VOLUME)) {	/* short filename entry*/
			int i,i2,last;
			int long_len = 0;
			unsigned char c;
			ASYNCIO_REENTRANT char longname[14];

			if (is_long) {
				unsigned char sum;
#pragma GCC diagnostic ignored "-Waggressive-loop-optimizations"
				for (i = sum = 0; i < MSDOS_NAME; i++) {
					sum = ((sum&1)<<7) | ((sum&0xfe)>>1);
					sum += de->name[i];
				}

				if (sum != alias_checksum)
					is_long = 0;

				unicodename[14] = 0;		/* truncate name to 14 characters*/
				long_len = unicode_to_ascii(longname, (unsigned char *)unicodename);
				/* FIXME should handle:
				 *	leading and trailing spaces ignored
				 *	trailing periods ignored
				 */
			}

			for (i = last = 0; i < 8; i++) {
				if (!(c = de->name[i])) break;
				if (c == 0x05) c = 0xE5;	/* 05 -> E5 as same as deleted entry*/
				if (c != ' ') last = i+1;
				name[i] = tolower(c);
			}
			i = last;
			name[i] = '.';
			i++;
			for (i2 = 0; i2 < 3; i2++) {
				if (!(c = de->ext[i2])) break;
				name[i] = tolower(c);
				i++;
				if (c != ' ') last = i;
			}
			if ((i = last) != 0) {
				if (!strcmp(de->name,MSDOS_DOT))
					*ino = dir->i_ino;
				else if (!strcmp(de->name,MSDOS_DOTDOT))
					*ino = msdos_parent_ino(dir,0);

				debug_fat("dir: '%s' attr %x size %lx\n", de->name, de->attr, de->size);
				if (is_long) {
					*namelen = long_len;
					*dirpos = oldpos;
					memcpy(name, longname, long_len);
					return 2;
				} else {
					*namelen = i;
					*dirpos = oldpos;
					return 1;
				}
			}
			is_long = 0;
		} else {
			is_long = 0;
			oldpos = *pos;
		}
		*ino = msdos_get_entry(dir,pos,bh,&de);

	} /* while (*ino != -1) */

	return 0;
}

/* Read a complete directory entry for a (specified) file,
 * submit a message to the callback function,
 * return a value of 0 or an error code.
 */
static int msdos_readdir(struct inode *dir, struct file *filp, char *dirbuf,
	filldir_t filldir)
{
	struct buffer_head *bh = NULL;
	ino_t ino;
	off_t dirpos;
	int res, namelen;
	ASYNCIO_REENTRANT char name[14];

	if (!dir || !S_ISDIR(dir->i_mode)) return -EBADF;
	if (dir->i_ino == MSDOS_ROOT_INO) {
		/* Fake . and .. for the root directory*/
		if ((int)filp->f_pos < 2) {
			/* Tricky: returns "." or ".." depending on namelen*/
			size_t namlen = (size_t)++filp->f_pos;
			return filldir(dirbuf, (char *)"..", namlen, filp->f_pos, (ino_t)MSDOS_ROOT_INO);
		}
		if ((int)filp->f_pos == 2)	/* reset to 0 after '..' for real directory offset*/
			filp->f_pos = 0;
	}

	res = msdos_get_entry_long(dir, &filp->f_pos, &bh, name, &namelen, &dirpos, &ino);
	if (res > 0)
		filldir(dirbuf, name, namelen, dirpos, ino);
	if (bh)
		unmap_brelse(bh);
	if (res < 0)
		return res;
	return 0;
}
