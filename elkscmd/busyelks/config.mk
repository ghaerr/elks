CFLAGS	+= -DCMD_INFO_ARGS
CFLAGS	+= -DCMD_INFO_DESCR

ifeq "$(CONFIG_APP_DISK_UTILS)" "b"
	CMD_fdisk	= yes
endif

ifeq "$(CONFIG_APP_FILE_UTILS)" "b"
	CMD_cat		= yes
	CMD_chgrp	= yes
	CMD_chmod	= yes
	CMD_chown	= yes
	CMD_cmp		= yes
	CMD_cp		= yes
	CMD_dd		= yes
endif

ifeq "$(CONFIG_APP_MINIX1)" "b"
	CMD_cksum	= yes
	CMD_cut		= yes
	CMD_du		= yes
endif

ifeq "$(CONFIG_APP_MINIX3)" "b"
	CMD_cal		= yes
	CMD_diff	= yes
	CMD_find	= yes
endif

ifeq "$(CONFIG_APP_MISC_UTILS)" "b"
	CMD_ed		= yes
endif

#CMD_basename	= yes
#CMD_date	= yes
#CMD_dirname	= yes
#CMD_echo	= yes
#CMD_false	= yes
#CMD_true	= yes
