/*
 *  linux/fs/filesystems.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  table of configured filesystems
 */

#include <linuxmt/config.h>
#include <linuxmt/types.h>
#include <linuxmt/fs.h>

#include <linuxmt/minix_fs.h>
#include <linuxmt/romfs_fs.h>
#include <linuxmt/elksfs_fs.h>
#include <linuxmt/major.h>

void fs_init(void)
{
#ifdef CONFIG_MINIX_FS
    init_minix_fs();
#endif
#ifdef CONFIG_ROMFS_FS
    init_romfs_fs();
#endif
#ifdef CONFIG_ELKSFS_FS
    init_elksfs_fs();
#endif
}
