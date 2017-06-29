/*
 *  linux-0.97/fs/msdos/namei.c
 *
 *  Written 1992 by Werner Almesberger
 */

#include <features.h>
#include <arch/segment.h>

#include <linuxmt/sched.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/kernel.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>
#include <linuxmt/mm.h>

unsigned char get_fs_byte(void *dv)
{
    unsigned char retv;

    memcpy_fromfs(&retv,dv,1);
    return retv;
}

/* Formats an MS-DOS file name. Rejects invalid names. */
/* Converted to 11 bytes of msdos 8.3 file name */
static int msdos_format_name(register const char *name,int len,char *res)
{
	register char *walk;
	unsigned char c;

	if (get_fs_byte(name) == DELETED_FLAG) return -EINVAL;
	if (get_fs_byte(name) == '.' && (len == 1 || (len == 2 &&
	    get_fs_byte(name+1) == '.'))) {
		memset(res+1,' ',10);
		while (len--) *res++ = '.';
		return 0;
	}
	c = 0; /* to make GCC happy */
	for (walk = res; len && walk-res < 8; walk++) {
	    	c = get_fs_byte(name++);
		len--;
		if (c <= ' ' || c == ':' || c == '\\') return -EINVAL;
		if (c == '.') break;
		*walk = toupper(c);
	}
	while (c != '.' && len--) c = get_fs_byte(name++);
	if (walk == res) return -EINVAL;
	if (c == '.') {
		while (walk-res < 8) *walk++ = ' ';
		while (len > 0 && walk-res < MSDOS_NAME) {
			c = get_fs_byte(name++);
			len--;
			if (c <= ' ' || c == ':' || c == '\\' || c == '.')
				return -EINVAL;
			*walk++ = toupper(c);
		}
	}
	while (walk-res < MSDOS_NAME) *walk++ = ' ';
	return 0;
}


/* Locates a directory entry. */

static int msdos_find(struct inode *dir,const char *name,int len,
    struct buffer_head **bh,struct msdos_dir_entry **de,ino_t *ino)
{
	char msdos_name[MSDOS_NAME];
	int res;

	if ((res = msdos_format_name(name,len,
	    msdos_name)) < 0) return res;
	return msdos_scan(dir,msdos_name,bh,de,ino);
}


int msdos_lookup(register struct inode *dir,const char *name,int len,
    register struct inode **result)
{
	ino_t ino;
	int res;
	struct msdos_dir_entry *de;
	struct buffer_head *bh;
	struct inode *next;
	*result = NULL;
/*	if (!dir) return -ENOENT; dir != NULL always, because reached this function dereferencing dir */
	if (!S_ISDIR(dir->i_mode)) {
		iput(dir);
		return -ENOENT;
	}
	if (len == 1 && get_fs_byte(name) == '.') {
		*result = dir;
		return 0;
	}
	if (len == 2 && get_fs_byte(name) == '.' && get_fs_byte(name+1) == '.')
	    {
		ino = msdos_parent_ino(dir,0);
		iput(dir);
		if (ino < 0) return (int)ino;
		if (!(*result = iget(dir->i_sb,ino))) return -EACCES;
		return 0;
	}
	if ((res = msdos_find(dir,name,len,&bh,&de,&ino)) < 0) {
		iput(dir);
		return res;
	}
	if (bh) unmap_brelse(bh);
/* printk("lookup: ino=%ld\r\n",ino); */
	if (!(*result = iget(dir->i_sb,ino))) {
		iput(dir);
		return -EACCES;
	}
	if ((*result)->u.msdos_i.i_busy) { /* mkdir in progress */
		iput(*result);
		iput(dir);
		return -ENOENT;
	}
	iput(dir);
	return 0;
}


/* Creates a directory entry (name is already formatted). */

static int msdos_create_entry(register struct inode *dir,char *name,int is_dir,
    register struct inode **result)
{
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	int res;
	ino_t ino;

	if ((res = msdos_scan(dir,NULL,&bh,&de,&ino)) < 0) {
		if (dir->i_ino == MSDOS_ROOT_INO) return -ENOSPC;
		if ((res = msdos_add_cluster(dir)) < 0) return res;
		if ((res = msdos_scan(dir,NULL,&bh,&de,&ino)) < 0) return res;
	}
	memcpy(de->name,name,MSDOS_NAME);
	de->attr = is_dir ? ATTR_DIR : ATTR_ARCH;
	de->start = 0;
	date_unix2dos(CURRENT_TIME,&de->time,&de->date);
	de->size = 0;
	bh->b_dirty = 1;
	if (*result = iget(dir->i_sb,ino)) msdos_read_inode(*result);
	unmap_brelse(bh);
	if (!*result) return -EIO;
	(*result)->i_mtime = 
	    CURRENT_TIME;
	(*result)->i_dirt = 1;
	return 0;
}


int msdos_create(register struct inode *dir,const char *name,int len,int mode,
	struct inode **result)
{
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	char msdos_name[MSDOS_NAME];
	ino_t ino;
	int res;
	if (!dir) return -ENOENT;
	if ((res = msdos_format_name(name,len,
	    msdos_name)) < 0) {
		iput(dir);
		return res;
	}
	lock_creation();
	if (msdos_scan(dir,msdos_name,&bh,&de,&ino) >= 0) {
		unlock_creation();
		unmap_brelse(bh);
		iput(dir);
		return -EEXIST;
 	}
	res = msdos_create_entry(dir,msdos_name,S_ISDIR(mode),result);
	unlock_creation();
	iput(dir);
	return res;
}


int msdos_mkdir(struct inode *dir,const char *name,int len,int mode)
{
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	struct inode *inode,*dot;
	char msdos_name[MSDOS_NAME];
	ino_t ino;
	int res;

	if ((res = msdos_format_name(name,len,
	    msdos_name)) < 0) {
		iput(dir);
		return res;
	}
	lock_creation();

	if (msdos_scan(dir,msdos_name,&bh,&de,&ino) >= 0) {

		unlock_creation();
		unmap_brelse(bh);
		iput(dir);
		return -EEXIST;
 	}

	if ((res = msdos_create_entry(dir,msdos_name,1,&inode)) < 0) {
		unlock_creation();
		iput(dir);
		return res;
	}

	inode->u.msdos_i.i_busy = 1; /* prevent lookups */
	if ((res = msdos_add_cluster(inode)) < 0) goto mkdir_error;
	if ((res = msdos_create_entry(inode,MSDOS_DOT,1,&dot)) < 0)
		goto mkdir_error;

	dot->i_size = inode->i_size;
	dot->u.msdos_i.i_start = inode->u.msdos_i.i_start;
	dot->i_dirt = 1;
	iput(dot);
	if ((res = msdos_create_entry(inode,MSDOS_DOTDOT,1,&dot)) < 0)
		goto mkdir_error;
	unlock_creation();
	dot->i_size = dir->i_size;
	dot->u.msdos_i.i_start = dir->u.msdos_i.i_start;
	dot->i_dirt = 1;
	inode->u.msdos_i.i_busy = 0;
	iput(dot);
	iput(inode);
	iput(dir);

	return 0;
mkdir_error:
	iput(inode);
	if (msdos_rmdir(dir,name,len) < 0) panic("rmdir in mkdir failed");
	unlock_creation();
	return res;
}


int msdos_rmdir(register struct inode *dir,const char *name,int len)
{
	int res;
	off_t pos;
	ino_t ino;
	struct buffer_head *bh,*dbh;
	struct msdos_dir_entry *de,*dde;
	register struct inode *inode;

	bh = NULL;
	inode = NULL;
	res = -EINVAL;
	if (len == 1 && get_fs_byte(name) == '.') goto rmdir_done;
	if ((res = msdos_find(dir,name,len,&bh,&de,&ino)) < 0) goto rmdir_done;
	res = -ENOENT;
	if (!(inode = iget(dir->i_sb,ino))) goto rmdir_done;
	res = -ENOTDIR;
	if (!S_ISDIR(inode->i_mode)) goto rmdir_done;
	res = -EBUSY;
	if (dir->i_dev != inode->i_dev || dir == inode) goto rmdir_done;
	if (inode->i_count > 1) goto rmdir_done;
	if (inode->u.msdos_i.i_start) { /* may be zero in mkdir */
		res = -ENOTEMPTY;
		pos = 0;
		dbh = NULL;
		while (msdos_get_entry(inode,&pos,&dbh,&dde) != -1)
			if (dde->name[0] && ((unsigned char *) dde->name)[0] !=
			    DELETED_FLAG && strncmp(dde->name,MSDOS_DOT,
			    MSDOS_NAME) && strncmp(dde->name,MSDOS_DOTDOT,
			    MSDOS_NAME)) goto rmdir_done; /* linux bug ??? */
		if (dbh) unmap_brelse(dbh);
	}
	inode->i_nlink = 0;
	dir->i_mtime = CURRENT_TIME;
	inode->i_dirt = dir->i_dirt = 1;
	de->name[0] = DELETED_FLAG;
	bh->b_dirty = 1;
	res = 0;
rmdir_done:
	unmap_brelse(bh);
	iput(dir);
	iput(inode);
	return res;
}


int msdos_unlink(register struct inode *dir,const char *name,int len)
{
	int res;
	ino_t ino;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	register struct inode *inode;

	bh = NULL;
	inode = NULL;
	if ((res = msdos_find(dir,name,len,&bh,&de,&ino)) < 0)
		goto unlink_done;
	if (!(inode = iget(dir->i_sb,ino))) {
		res = -ENOENT;
		goto unlink_done;
	}
	if (!S_ISREG(inode->i_mode)) {
		res = -EPERM;
		goto unlink_done;
	}
	inode->i_nlink = 0;
	inode->u.msdos_i.i_busy = 1;
	inode->i_dirt = 1;
	de->name[0] = DELETED_FLAG;
	bh->b_dirty = 1;
unlink_done:
	unmap_brelse(bh);
	iput(inode);
	iput(dir);
	return res;
}
