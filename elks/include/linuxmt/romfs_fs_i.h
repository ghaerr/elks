#ifndef LX86_LINUXMT_ROMFS_FS_I_H
#define LX86_LINUXMT_ROMFS_FS_I_H

/* inode in-kernel data */

struct romfs_inode_info {
    unsigned long i_metasize;	/* size of non-data area */
    unsigned long i_dataoffset;	/* from the start of fs */
};

#endif
