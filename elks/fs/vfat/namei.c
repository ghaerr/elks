/*
 *  linux-2.0.34/fs/vfat/namei.c
 *
 *  Written 1992,1993 by Werner Almesberger
 *
 *  Windows95/Windows NT compatible extended MSDOS filesystem
 *    by Gordon Chaffee Copyright (C) 1995.  Send bug reports for the
 *    VFAT filesystem to <chaffee@plateau.cs.berkeley.edu>.  Specify
 *    what file operation caused you trouble and if you can duplicate
 *    the problem, send a script that demonstrates it.
 */
/* neated fangzs' port by zouys, Nov 23rd, 2010 */

#include <features.h>
#include <arch/segment.h>
#include <linuxmt/sched.h>
#include <linuxmt/fs.h>
#include <linuxmt/msdos_fs.h>
#include <linuxmt/msdos_fs_i.h>
#include <linuxmt/msdos_fs_sb.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/stat.h>

/*
 * XXX: It would be better to use the tolower from linux/ctype.h,
 * but _ctype is needed and it is not exported.
 */
//#define tolower(c) (((c) >= 'A' && (c) <= 'Z') ? (c)-('A'-'a') : (c))

struct vfat_find_info {
	const char *name;
	int len;
	int found;

	/* equivalent struct slot_info, instead of '-Os' */
	int is_long;
	int long_slots;
	off_t offset;
	off_t short_offset;
	ino_t ino;
};

char *strncpy(char *dest, char *src, size_t count)
{
    register char *tmp = dest;

    while (count-- && (*tmp++ = *src++))
	/* Do nothing */ ;

    return dest;
}

static int vfat_find(struct inode *dir,const char *name,int len,
		      int find_long,int new_filename,int is_dir,
		      struct slot_info *sinfo_out);

/* Checks the validity of an long MS-DOS filename */
/* Returns negative number on error, 0 for a normal
 * return, and 1 for . or .. */

static int vfat_valid_longname(register const char *name, int len)
{
	if (IS_FREE(name)) return -EINVAL;
	if (name[0] == '.' && (len == 1 || (len == 2 && name[1] == '.'))) {
		return 1;
	}

	if (len && name[len-1] == ' ') return -EINVAL;
	if (len >= 14) return -EINVAL;
	return 0;
}

static int vfat_valid_shortname(register const char *name,int len)
/* Check if msdos8.3 file name */
{
	register const char *walk;
	unsigned char c;

	if (name[0] == DELETED_FLAG){ 
		return -EINVAL;
	}
	if (name[0] == '.' && (len == 1 || (len == 2 && name[1] == '.'))) {
		return 1;
	}
	c = 0;
	for (walk = name; len && walk-name < 8 + 1;) {
	    	c = *walk++;
		len--;
		if (c == '.') break;
	}
	if (len) {
		if (c != '.' || len >= 4) return -EINVAL;
	}
	else {
		if (walk-name == 9 && c != '.') return -EINVAL;
	}

	return 0;
}

/* Takes a short filename and converts it to a formatted MS-DOS filename.
 * If the short filename is not a valid MS-DOS filename, an error is 
 * returned.  The formatted short filename is returned in 'res'.
 */
/* Converted to 11 bytes of msdos83 file name */
static int vfat_format_name(register const char *name,int len,char *res)
{
	register char *walk;
	unsigned char c;

	if (name[0] == DELETED_FLAG) return -EINVAL;
	if (name[0] == '.' && (len == 1 || (len == 2 && name[1] == '.'))) {
		memset(res+1,' ',10);
		while (len--) *res++ = '.';
		return 0;
	}

	c = 0;
	for (walk = res; len && walk-res < 8; walk++) {
	    	c = *name++;
		len--;
		if (c <= ' ' || c == ':' || c == '\\') return -EINVAL;
		if (c == '.') break;
		*walk = toupper(c);
	}
	while (c != '.' && len--) c = *name++;
	if (walk == res) return -EINVAL;
	if (c == '.') {
		while (walk-res < 8) *walk++ = ' ';
		while (len > 0 && walk-res < MSDOS_NAME) {
			c = *name++;
			len--;
			if (c <= ' ' || c == ':' || c == '\\' || c == '.')
				return -EINVAL;
			*walk++ = toupper(c);
		}
	}
	while (walk-res < MSDOS_NAME) *walk++ = ' ';
	return 0;
}

/* Given a valid longname, create a unique shortname.  Make sure the
 * shortname does not exist
 */
static int vfat_create_shortname(struct inode *dir, const char *name,
     int len, char *name_res)
{
	register const char *ip; const char *ext_start, *end;
	register char *p;
	int sz/*Base name length*/, extlen, baselen/*8.3 base name length*/, totlen;
	char msdos_name[13];
	char base[9], ext[4];
	int i;
	int res;
	int spaces;
	char buf;
	struct slot_info sinfo;

	if (len && name[len-1]==' ') return -EINVAL;
	if (len <= 12) {
		res = vfat_format_name(name, len, name_res);
		if (res != -1) {
			res = vfat_find(dir, name_res, len, 0, 0, 0, &sinfo);
			if (res != -1) return -EEXIST;
			return 0;
		}
	}

	/* Now, we need to create a shortname from the long name */
	ext_start = end = &name[len];
	while (--ext_start >= name) {
		if (*ext_start == '.') {
			break;
		}
	}
	if (ext_start == name - 1 || ext_start == end - 1) {
		sz = len;
		ext_start = NULL;
	} else {
		sz = ext_start - name;
		ext_start++;
	}

	for (baselen = 0, p = base, ip = name; baselen < sz && baselen < 8; baselen++)
	{
		*p++ = tolower(*ip++);
	}
		
	spaces = 8 - baselen;

	extlen = 0;
	if (ext_start) {
		for (p = ext, ip = ext_start; extlen < 3 && ip < end; extlen++) {
			*p++ = tolower(*ip++);
		}
	}
	ext[extlen] = '\0';
	if (extlen > 0) extlen++;

	strncpy(p = msdos_name, base, baselen);
	*(p += baselen) = '.';
	strcpy(p+1, ext);

	totlen = baselen + extlen;
	i = 0;
	while ((res = vfat_find(dir, msdos_name, totlen, 0, 0, 0, &sinfo)) != -1) {
		/* Create the next shortname to try */
		i++;
		if (i == 10) return -EEXIST;
		buf = '0' + (char)i;
		if (2 > spaces) {
			baselen -= (2 - spaces);
			spaces = 2;
		}

		memcpy(p = msdos_name, base, baselen);
		*(p += baselen) = '~';
		*++p = buf;
		*++p = '.';
		strcpy(p+1, ext);

		totlen = baselen + 2 + extlen;
	}
	return vfat_format_name(msdos_name, totlen, name_res);
}

static loff_t vfat_find_free_slots(register struct inode *dir,int slots)
{
	loff_t offset, curr;
	struct msdos_dir_entry *de;
	struct buffer_head *bh;
	register struct inode *inode;
	ino_t ino;
	int row;
	int done;
	int res;
	int added;

	offset = curr = 0;
	bh = NULL;
	row = 0;
	ino = msdos_get_entry(dir,&curr,&bh,&de);

	for (added = 0; added < 2; added++) {
		while (ino != (ino_t)(-1L)) {
			done = IS_FREE(de->name);
			if (done) {
				inode = iget(dir->i_sb,ino);
				if (inode) {
					/* Directory slots of busy deleted files aren't available yet. */
					done = !MSDOS_I(inode)->i_busy;
				}
				iput(inode);
			}
			if (done) {
				row++;
				if (row == slots) {
					brelse(bh);
					/* printk("----- Free offset at %d\n", offset); */
					return offset;
				}
			} else {
				row = 0;
				offset = curr;
			}
			ino = msdos_get_entry(dir,&curr,&bh,&de);
		}

#ifndef FAT_BITS_32
		if ((dir->i_ino == MSDOS_ROOT_INO) &&
		    (MSDOS_SB(dir->i_sb)->fat_bits != 32)) 
			return -ENOSPC;
#endif
		if ((res = msdos_add_cluster(dir)) < 0) return res;
		ino = msdos_get_entry(dir,&curr,&bh,&de);
	}
	return -ENOSPC;
}

/* Translate a string, including coded sequences into Unicode */
static int
xlate_to_uni(const unsigned char *name, int len, register unsigned short *outname, register int *outlen)
{
	int i, fill;

	if (name[len-1] == '.') 
		len--;

	for (i = 0; i < len; i++)
		*outname++ = (unsigned short) *name++;

	*outlen = i;
	if (*outlen > 14)
		return -ENAMETOOLONG;

	if (*outlen % 13) {
		*outname++ = 0;
		*outlen += 1;
		if (*outlen % 13) {
			fill = 13 - (*outlen % 13);
			for (i = 0; i < fill; i++) {
				*outname++ = 0xffff;
			}
			*outlen += fill;
		}
	}

	return 0;
}

static int
vfat_fill_long_slots(struct msdos_dir_slot *ds, const char *name, int len,
		     char *msdos_name, int *slots)
{
	register struct msdos_dir_slot *ps;
	int res;
	int slot;
	unsigned char cksum;
	char uniname[52];
	register const char *ip;
	int unilen;
	int i;

	if (name[len-1] == '.') len--;
	res = xlate_to_uni(name, len, uniname, &unilen);
	if (res < 0) {
		return res;
	}

	*slots = unilen / 13;
	for (cksum = 0, i = 0; i < 11; i++) {
		//cksum = (((cksum&1)<<7)|((cksum&0xfe)>>1));
		asm("rorb [bp-66], 1"); /* Note the offset */
		cksum += msdos_name[i];
	}

	for (ps = ds, slot = *slots; slot > 0; slot--, ps++) {
		ps->id = slot;
		ps->attr = ATTR_EXT;
		ps->reserved = 0;
		ps->alias_checksum = cksum;
		ps->start = 0;
		ip = &uniname[(slot - 1) * 26];
		memcpy(ps->name0_4, ip, 10);
		memcpy(ps->name5_10, ip += 10, 12);
		memcpy(ps->name11_12, ip += 12, 4);
	}
	ds[0].id |= 0x40;

	strncpy(((struct msdos_dir_entry *) ps)->name, msdos_name, MSDOS_NAME);

	return 0;
}
		
static int vfat_build_slots(struct inode *dir,register const char *name,int len,
     struct msdos_dir_slot *ds, int *slots, int *is_long)
{
	register struct msdos_dir_entry *de;
	char msdos_name[MSDOS_NAME];
	int res;

	de = (struct msdos_dir_entry *) ds;

	*slots = 1;
	*is_long = 0;
	if (len == 1 && name[0] == '.') {
		strncpy(de->name, MSDOS_DOT, MSDOS_NAME);
	} else if (len == 2 && name[0] == '.' && name[1] == '.') {
		strncpy(de->name, MSDOS_DOTDOT, MSDOS_NAME);
	} else {
		res = vfat_valid_shortname(name, len);
		if (res != -1) {
			res = vfat_format_name(name, len, de->name);
		} else {
			res = vfat_create_shortname(dir, name, len, msdos_name);
			if (res < 0) {
				return res;
			}

			res = vfat_valid_longname(name, len);
			if (res < 0) {
				return res;
			}

			*is_long = 1;

			return vfat_fill_long_slots(ds, name, len, msdos_name,
						    slots);
		}
	}
	return 0;
}

/* To determine whether to read the required directory entry */
static int vfat_readdir_cb(
	filldir_t filldir,
	void * buf,
	const char * name/*Read the file name*/,
	int name_len,
	int is_long/*Whether it is a long file name*/,
	off_t offset,
	off_t short_offset,
	int long_slots/*The correct number of long file name entries*/,
	ino_t ino)
{
	register struct vfat_find_info *vf = (struct vfat_find_info *) buf;
	int i;

#ifdef DEBUG
	if (debug) printk("cb: vf.name=%s, len=%d, name=%s, name_len=%d\n",
			  vf->name, vf->len, name, name_len);
#endif

	/* Filenames cannot end in '.' or we treat like it has none */
	if (vf->len != name_len) {
		if ((vf->len != name_len + 1) || (vf->name[name_len] != '.')) {
			return 0;
		}
	}

	for (i = 0; i < name_len; i++) {
			if (tolower(*(name+i)) != tolower(*(vf->name+i)))
				return 0;
	}
	vf->found = 1;
	vf->is_long = is_long;
	vf->offset = ((short)offset == 2) ? 0 : offset;
	vf->short_offset = ((short)short_offset == 2) ? 0 : short_offset;
	vf->long_slots = long_slots;
	vf->ino = ino;
	return -1;
}

static int vfat_find(register struct inode *dir,const char *name,int len,
    int find_long, int new_filename,int is_dir,register struct slot_info *sinfo_out)
{
	struct vfat_find_info vf;
	struct file fil;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	struct msdos_dir_slot *ps;
	loff_t offset;
	struct msdos_dir_slot ds[3];/* max # of slots needed for short and long names */
	int is_long;
	int slots, slot;
	int res;

	memset(ds, 0, sizeof(ds));

	fil.f_pos = 0;
	vf.name = name;
	vf.len = len;
	vf.found = 0;
	res = fat_readdirx(dir,&fil,(void *)&vf,vfat_readdir_cb,NULL,1,find_long);
	if (res < 0) goto cleanup;
	if (vf.found) {
		if (new_filename) {
			res = -EEXIST;
			goto cleanup;
		}
		*sinfo_out = *(struct slot_info *)&vf.is_long;
#if 0 //def CONFIG_UMSDOS_FS
		if (dir->i_ino == MSDOS_ROOT_INO && 
		    vf.len == 3 && !strncmp(vf.name,"dev",3)) {
			MSDOS_SB(dir->i_sb)->dev_ino = vf.ino;
		}
#endif

		res = 0;
		goto cleanup;
	}
	else if (!new_filename) {
		res = -ENOENT;
		goto cleanup;
	}
	
	res = vfat_build_slots(dir, name, len, ds, &slots, &is_long);
	if (res < 0) goto cleanup;

	bh = NULL;
	if (new_filename) {
		if (is_long) slots++;
		offset = vfat_find_free_slots(dir, slots);
		if (offset < 0) {
			res = (int)offset;
			goto cleanup;
		}

		/* Now create the new entry */
		for (slot = 0, ps = ds; slot < slots; slot++, ps++) {
			sinfo_out->ino = msdos_get_entry(dir,&offset,&bh,&de);
			if ((long)sinfo_out->ino < 0) {
				res = (int)sinfo_out->ino;
				goto cleanup;
			}
			memcpy(de, ps, sizeof(struct msdos_dir_slot));
			mark_buffer_dirty(bh,1);
		}

		dir->i_mtime = CURRENT_TIME;
		dir->i_dirt = 1;

		date_unix2dos(dir->i_mtime,&de->time,&de->date);
		de->start = 0;
		de->starthi = 0;
		de->size = 0;
		de->attr = is_dir ? ATTR_DIR : ATTR_ARCH;


		mark_buffer_dirty(bh,1);
		brelse(bh);

		sinfo_out->long_slots = 
		(sinfo_out->is_long = (slots > 1) ? 1 : 0) ? slots - 1 : 0;
		sinfo_out->shortname_offset = offset - sizeof(struct msdos_dir_slot);
		sinfo_out->longname_offset = offset - sizeof(struct msdos_dir_slot) * slots;
		res = 0;
	} else {
		res = -ENOENT;
	}

cleanup:
	return res;
}

int msdos_lookup(register struct inode *dir,const char *name,int len,
    register struct inode **result)
{
	int res;
	ino_t ino;
	struct slot_info sinfo;

	char tmp_name[16];
	tmp_name[len] = '\0';
	memcpy_fromfs(tmp_name,name,len);

	*result = NULL;
/*	if (!dir) return -ENOENT; dir != NULL always, because reached this function dereferencing dir */
	if (!S_ISDIR(dir->i_mode)) {
		iput(dir);
		return -ENOENT;
	}
	if (len == 1 && tmp_name[0] == '.') {
		*result = dir;
		return 0;
	}
	if (len == 2 && tmp_name[0] == '.' && tmp_name[1] == '.') {
		ino = msdos_parent_ino(dir,0);
		iput(dir);
		if ((__s32)ino < 0) return ino;
		if (!(*result = iget(dir->i_sb,ino))) return -EACCES;
		return 0;
	}
	if ((res = vfat_find(dir,tmp_name,len,1,0,0,&sinfo)) < 0) {
		iput(dir);
		return res;
	}
	if (!(*result = iget(dir->i_sb,sinfo.ino))) {
		iput(dir);
		return -EACCES;
	}
	if (!(*result)->i_sb) {
		/* crossed a mount point into a non-msdos fs */
		iput(dir);
		return 0;
	}
	if (MSDOS_I(*result)->i_busy) { /* mkdir in progress */
		iput(*result);
		iput(dir);
		return -ENOENT;
	}
	iput(dir);
	return 0;
}


static int vfat_create_entry(struct inode *dir,const char *name,int len,
    int is_dir, register struct inode **result)
{
	int res;
	ino_t ino;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	struct slot_info sinfo;

	res = vfat_find(dir, name, len, 1, 1, is_dir, &sinfo);
	if (res < 0) {
		return res;
	}

	bh = NULL;
	ino = msdos_get_entry(dir, &sinfo.shortname_offset, &bh, &de);
	if ((long)ino < 0) {
		if (bh)
			brelse(bh);
		return ino;
	}

	if ((*result = iget(dir->i_sb,ino)) != NULL)
		msdos_read_inode(*result);
	brelse(bh);
	if (!*result)
		return -EIO;
	(*result)->i_mtime =
	    CURRENT_TIME;
	(*result)->i_dirt = 1;

	return 0;
}

int msdos_create(register struct inode *dir,const char *name,int len,int mode,
	struct inode **result)
{
	int res;

	char tmp_name[16];
	tmp_name[len] = '\0';
	memcpy_fromfs(tmp_name,name,len);

	if (!dir) return -ENOENT;

	lock_creation();
	res = vfat_create_entry(dir,tmp_name,len,0,result);

	unlock_creation();
	iput(dir);
	return res;
}

static int vfat_create_a_dotdir(register struct inode *dir,struct inode *parent,
     struct buffer_head *bh,
     struct msdos_dir_entry *de,ino_t ino,char *name)
{
	register struct inode *dot;

	/*
	 * XXX all times should be set by caller upon successful completion.
	 */
	dir->i_mtime = CURRENT_TIME;
	dir->i_dirt = 1;
	memcpy(de->name,name,MSDOS_NAME);
	de->attr = ATTR_DIR;
	de->start = 0;
	de->starthi = 0;
	date_unix2dos(dir->i_mtime,&de->time,&de->date);
	de->size = 0;
	mark_buffer_dirty(bh, 1);
	if ((dot = iget(dir->i_sb,ino)) != NULL)
		msdos_read_inode(dot);
	if (!dot) return -EIO;
	dot->i_mtime = CURRENT_TIME;
	dot->i_dirt = 1;
	if (parent) dir = parent;
		dot->i_size = dir->i_size;
		MSDOS_I(dot)->i_start = MSDOS_I(dir)->i_start;
		dot->i_nlink = dir->i_nlink;

	iput(dot);

	return 0;
}

static int vfat_create_dotdirs(register struct inode *dir, struct inode *parent)
{
	ino_t ino;  int res;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	loff_t offset;

	if ((res = msdos_add_cluster(dir)) < 0) return res;

	offset = 0;
	bh = NULL;
	if ((long)(ino = msdos_get_entry(dir,&offset,&bh,&de)) < 0) return (int)ino;
	
	res = vfat_create_a_dotdir(dir, NULL, bh, de, (ino_t)res, MSDOS_DOT);
	if (res < 0) {
		brelse(bh);
		return res;
	}

	if ((long)(ino = msdos_get_entry(dir,&offset,&bh,&de)) < 0) {
		brelse(bh);
		return (int)ino;
	}

	res = vfat_create_a_dotdir(dir, parent, bh, de, (ino_t)res, MSDOS_DOTDOT);
	brelse(bh);

	return res;
}

/***** See if directory is empty */
static int vfat_empty(register struct inode *dir)
{
	loff_t pos;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;

	if (dir->i_count > 1)
		return -EBUSY;
	if (MSDOS_I(dir)->i_start) { /* may be zero in mkdir */
		pos = 0;
		bh = NULL;
		while (msdos_get_entry(dir,&pos,&bh,&de) != -1) {
			/* Skip extended filename entries */
			if (de->attr == ATTR_EXT) continue;

			if (!IS_FREE(de->name) && strncmp(de->name,MSDOS_DOT,
			    MSDOS_NAME) && strncmp(de->name,MSDOS_DOTDOT,
			    MSDOS_NAME)) {
				brelse(bh);
				return -ENOTEMPTY;
			}
		}
		if (bh)
			brelse(bh);
	}
	return 0;
}

static int vfat_rmdir_free_ino(register struct inode *dir,struct buffer_head *bh,
     struct msdos_dir_entry *de,ino_t ino)
{
	register struct inode *inode;
	int res;

	if (!(inode = iget(dir->i_sb,ino))) return -ENOENT;
	if (!S_ISDIR(inode->i_mode)) {
		iput(inode);
		return -ENOTDIR;
	}
	if (dir->i_dev != inode->i_dev || dir == inode) {
		iput(inode);
		return -EBUSY;
	}
	res = vfat_empty(inode);
	if (res) {
		iput(inode);
		return res;
	}
	inode->i_nlink = 0;
	inode->i_mtime = dir->i_mtime = CURRENT_TIME;
	dir->i_nlink--;
	inode->i_dirt = dir->i_dirt = 1;
	de->name[0] = DELETED_FLAG;
	mark_buffer_dirty(bh,1);
	iput(inode);

	return 0;
}

static int vfat_unlink_free_ino(register struct inode *dir,struct buffer_head *bh,
     struct msdos_dir_entry *de,ino_t ino)
{
	register struct inode *inode;
	if (!(inode = iget(dir->i_sb,ino))) return -ENOENT;
	if ((!S_ISREG(inode->i_mode)) || IS_IMMUTABLE(inode)) {
		iput(inode);
		return -EPERM;
	}
	inode->i_nlink = 0;
	inode->i_mtime = dir->i_mtime = CURRENT_TIME;
	MSDOS_I(inode)->i_busy = 1;
	inode->i_dirt = dir->i_dirt = 1;
	de->name[0] = DELETED_FLAG;
	mark_buffer_dirty(bh,1);

	iput(inode);
	return 0;
}

static int vfat_remove_entry(struct inode *dir,struct slot_info *sinfo,
     register struct buffer_head **bh,register struct msdos_dir_entry **de,
     int is_dir)
{
	//*register*/ struct inode *inode;   /* bcc_bug */
	loff_t offset;
	ino_t ino;  int res, i;

	/* remove the shortname */
	offset = sinfo->shortname_offset;
	ino = msdos_get_entry(dir, &offset, bh, de);
	if ((long)ino < 0) return (int)ino;
	if (is_dir) {
		res = vfat_rmdir_free_ino(dir,*bh,*de,ino);
	} else {
		res = vfat_unlink_free_ino(dir,*bh,*de,ino);
	}
	if (res < 0) return res;
		
	/* remove the longname */
	offset = sinfo->longname_offset;
	for (i = sinfo->long_slots; i > 0; --i) {
		ino = msdos_get_entry(dir, &offset, bh, de);
		if ((long)ino < 0) {
			printk("vfat_remove_entry: problem 1\n");
			continue;
		}
		(*de)->name[0] = DELETED_FLAG;
		(*de)->attr = 0;
		mark_buffer_dirty(*bh,1);
	}
	return 0;
}


static int vfat_rmdirx(register struct inode *dir,const char *name,int len)
{
	int res;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	struct slot_info sinfo;

	char tmp_name[16];
	tmp_name[len] = '\0';
	memcpy_fromfs(tmp_name,name,len);

	bh = NULL;
	res = -EPERM;
	if(tmp_name[0] == '.' && (len == 1 || (len == 2 && tmp_name[1] == '.')))
		goto rmdir_done;
	res = vfat_find(dir,tmp_name,len,1,0,0,&sinfo);

	if (res >= 0) {
		res = vfat_remove_entry(dir,&sinfo,&bh,&de,1);
		if (res > 0) {
			res = 0;
		}
	}

rmdir_done:
	brelse(bh);
	iput(dir);
	return res;
}

/***** Remove a directory */
int msdos_rmdir(struct inode *dir,const char *name,int len)
{
	asm("br _vfat_rmdirx");
	//return vfat_rmdirx(dir, name, len);
}

static int vfat_unlinkx(
	register struct inode *dir,
	const char *name,
	int len)
{
	int res;
	struct buffer_head *bh;
	struct msdos_dir_entry *de;
	struct slot_info sinfo;

	char tmp_name[16];
	tmp_name[len] = '\0';
	memcpy_fromfs(tmp_name,name,len);

	bh = NULL;
	res = vfat_find(dir,tmp_name,len,1,0,0,&sinfo);

	if (res >= 0) {
		res = vfat_remove_entry(dir,&sinfo,&bh,&de,0);
		if (res > 0) {
			res = 0;
		}
	}

	brelse(bh);
	iput(dir);
	return res;
}

int msdos_mkdir(register struct inode *dir,const char *name,int len,int mode)
{
	register struct inode *inode;
	struct inode *dot;  /* instead of '-Os' */
	int res;

	char tmp_name[16];
	tmp_name[len] = '\0';
	memcpy_fromfs(tmp_name,name,len);

	lock_creation();
	if ((res = vfat_create_entry(dir,tmp_name,len,1,&dot)) < 0) {
		unlock_creation();
		iput(dir);
		return res;
	}
	inode = dot;

	dir->i_nlink++;
	inode->i_nlink = 2; /* no need to mark them dirty */
	MSDOS_I(inode)->i_busy = 1; /* prevent lookups */

	res = vfat_create_dotdirs(inode, dir);
	unlock_creation();
	MSDOS_I(inode)->i_busy = 0;
	iput(inode);
	iput(dir);
	if (res < 0) {
		if (msdos_rmdir(dir,name,len) < 0)
			panic("rmdir in mkdir failed");
	}
	return res;
}

/***** Unlink, as called for msdosfs */
int msdos_unlink(struct inode *dir,const char *name,int len)
{
	asm("br _vfat_unlinkx");
	//return vfat_unlinkx (dir,name,len);
}
