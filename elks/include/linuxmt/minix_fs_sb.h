#ifndef LX86_LINUX_MINIX_FS_SB
#define LX86_LINUX_MINIX_FS_SB

/*
 * minix super-block data in memory
 */

struct minix_sb_info {
    unsigned short s_ninodes;
    unsigned short s_nzones;
    unsigned short s_imap_blocks;
    unsigned short s_zmap_blocks;
    unsigned short s_firstdatazone;
    unsigned short s_log_zone_size;
    unsigned long s_max_size;
    struct buffer_head *s_imap[8];
    struct buffer_head *s_zmap[64];
    unsigned short s_dirsize;
    unsigned short s_namelen;
    struct buffer_head *s_sbh;
    struct minix_super_block *s_ms;
    unsigned short s_mount_state;
    unsigned short s_version;
};

#endif
