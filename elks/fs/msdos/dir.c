/*
 *  linux/fs/msdos/dir.c
 *
 *  Written 1992 by Werner Almesberger
 *
 *  MS-DOS directory handling functions
 */
/* ported from linx-0.97 by zouys, Oct 21st, 2010 */
/* neated from linx-2.0.34 fangzs, Nov 23rd, 2010 */

#include <features.h>
#include <arch/segment.h>

#include <linuxmt/fs.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/errno.h>
#include <linuxmt/stat.h>

static int msdos_dir_read(inode, filp, buf, count)
     struct inode *inode;
     struct file *filp;
     char *buf;
     int count;
{
    return -EISDIR;
}

static int msdos_readdir(struct inode *inode,
			 struct file *filp,
			 char *dirent, filldir_t filldir);

/*@-type@*/

static struct file_operations msdos_dir_operations = {
	NULL,			/* lseek - default */
	msdos_dir_read,		/* read */
	NULL,			/* write - bad */
	msdos_readdir,		/* readdir */
	NULL,			/* select - default */
	NULL,			/* ioctl - default */
	NULL,			/* no special open code */
	NULL,			/* no special release code */
#ifdef BLOAT_FS
	NULL			/* default fsync */
#endif
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
#ifdef USE_GETBLK
	NULL,			/* getblk */
#endif
	NULL,			/* truncate */
#ifdef BLOAT_FS
	NULL			/* permission */
#endif
};

/*@+type@*/

static int
uni16_to_x8(unsigned char *ascii, register unsigned char *uni)
{
	register unsigned char *op = ascii;

	while(*uni && *uni!=0xff){
		*op++ = *uni;
		uni += 2;
	}
	*op = 0;
	return (op - ascii);
}

/* Read a complete directory entry for a (specified) file,
 * submit a message to the callback function,
 * return a value of 0 or an error code.
 */
int msdos_readdir(
	struct inode *inode,
	register struct file *filp,
	char *dirent,
	filldir_t filldir)
{
	ino_t ino;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	unsigned long oldpos = filp->f_pos; /* The location of the next starting directory entry */
	int is_long;                        /* Whether it is complete, and its long file name matches the long file name */
	unsigned char alias_checksum = 0; /* Make compiler warning go away */
	unsigned char long_slots = 0;
	unsigned char unicode[52];          /* Limit to two long entries */
	if (!inode || !S_ISDIR(inode->i_mode)) return -EBADF;
	if (inode->i_ino == MSDOS_ROOT_INO) {
		/* Fake . and .. for the root directory. */
		if ((int)filp->f_pos < 2) {
			/* Tricky: returns "." or ".." depending on f_pos */
			return filldir(dirent, "..", (int)filp->f_pos, ++filp->f_pos, (long) MSDOS_ROOT_INO);
		}

		if ((int)filp->f_pos == 2) *(int *)&filp->f_pos = 0;
	}

	if ((int)filp->f_pos & (sizeof(struct msdos_dir_entry)-1)) return -ENOENT;
	bh = NULL;
	is_long = 0;
	ino = msdos_get_entry(inode,&filp->f_pos,&bh,&de);
	while (ino != (ino_t)(-1L)) {
		/* Check for long filename entry */
		if (de->name[0] == (__s8) DELETED_FLAG) {
			is_long = 0;
			oldpos = filp->f_pos;
		} else if (de->attr ==  ATTR_EXT) {
			register struct msdos_dir_slot *ds;
			int offset;
			unsigned char slot = 0;

			ds = (struct msdos_dir_slot *) de;
			if (ds->id & 0x40) {
				long_slots = slot = ds->id & ~0x40;
				is_long = 1;
				alias_checksum = ds->alias_checksum;
			}

			while (slot > 0) {
				if (ds->attr !=  ATTR_EXT ||
				    (ds->id & ~0x40) != slot ||
				    ds->alias_checksum != alias_checksum) {
					is_long = 0;
					break;
				}
				slot--;
				offset = slot * 26;
				memcpy(&unicode[offset], ds->name0_4, 10);
				offset += 10;
				memcpy(&unicode[offset], ds->name5_10, 12);
				offset += 12;
				memcpy(&unicode[offset], ds->name11_12, 4);
				offset += 4;

				if (ds->id & 0x40) {
					*(short *)&unicode[offset] = 0;
				}
				if (slot > 0) {
					ino = msdos_get_entry(inode,&filp->f_pos,&bh,&de);
					if (ino == -1L) {
						is_long = 0;
						break;
					}
					ds = (struct msdos_dir_slot *) de;
				}
			}
		} else if (de->name[0] && !(de->attr & ATTR_VOLUME)) {
			int i,i2,last;
			char longname[14];
			unsigned char long_len = 0; /* Make compiler warning go away */
			char bufname[13], c;
			register char *ptname = bufname;

			if (is_long) {
				unsigned char sum;
				for (sum = 0, i = 0; i < 11; i++) {
					sum = (((sum&1)<<7)|((sum&0xfe)>>1));
//					asm("rorb [bp-111], 1");    /* Note the offset */
					sum += de->name[i];
				}

				if (sum != alias_checksum) {
					is_long = 0;
				}
				long_len = uni16_to_x8(longname, unicode);
			}

			for (i = last = 0; i < 8; i++) {
				if (!(c = de->name[i])) break;
				/* see namei.c, msdos_format_name */
				if (c == 0x05) c = 0xE5;
				if (c != ' ') last = i+1;
				ptname[i] = tolower(c);
			}
			i = last;
			ptname[i] = '.';
			i++;
			for (i2 = 0; i2 < 3; i2++) {
				if (!(c = de->ext[i2])) break;
				ptname[i] = tolower(c);
				i++;
				if (c != ' ') last = i;
			}
			if ((i = last) != 0) {
				if (!strcmp(de->name,MSDOS_DOT))
					ino = inode->i_ino;
				else if (!strcmp(de->name,MSDOS_DOTDOT))
					ino = msdos_parent_ino(inode,0);

				if (!is_long) {
					filldir(dirent, bufname, i, oldpos, (long) ino);
					break;
				}
				else {
#ifdef CONFIG_UMSDOS_FS
					if (inode->i_ino == MSDOS_ROOT_INO && filldir && 
					    long_len == 3 && !strncmp(longname,"dev",3)) {
						MSDOS_SB(inode->i_sb)->dev_ino = ino;
					}
#endif
					filldir(dirent, longname, long_len, oldpos, (long) ino);
					break;
				}
				oldpos = filp->f_pos;
			}
			is_long = 0;
		} else {
			is_long = 0;
			oldpos = filp->f_pos;
		}
		ino = msdos_get_entry(inode,&filp->f_pos,&bh,&de);

	} /* while (ino != -1) */

	if (bh)
		unmap_brelse(bh);
	return 0;
}
