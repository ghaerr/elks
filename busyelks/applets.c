#ifdef BUSYELKS1
#ifdef CONFIG_APPLET_BASENAME
if(strcmp(progname, "basename") == 0) basename_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CAT
if(strcmp(progname, "cat") == 0) cat_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CHGRP
if(strcmp(progname, "chgrp") == 0) chgrp_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CHMOD
if(strcmp(progname, "chmod") == 0) chmod_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CHOWN
if(strcmp(progname, "chown") == 0) chown_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CLOCK
if(strcmp(progname, "clock") == 0) clock_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CMP
if(strcmp(progname, "cmp") == 0) cmp_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CP
if(strcmp(progname, "cp") == 0) cp_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_DATE
if(strcmp(progname, "date") == 0) date_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_DD
if(strcmp(progname, "dd") == 0) dd_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_DIRNAME
if(strcmp(progname, "dirname") == 0) dirname_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_DU
if(strcmp(progname, "du") == 0) du_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_ECHO
if(strcmp(progname, "echo") == 0) echo_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_GETTY
if(strcmp(progname, "getty") == 0) getty_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_GREP
if(strcmp(progname, "grep") == 0) grep_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_HEAD
if(strcmp(progname, "head") == 0) head_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_INIT
if(strcmp(progname, "init") == 0) init_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_KILL
if(strcmp(progname, "kill") == 0) kill_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_L
if(strcmp(progname, "l") == 0) l_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_LN
if(strcmp(progname, "ln") == 0) ln_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_LOGIN
if(strcmp(progname, "login") == 0) login_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_LS
if(strcmp(progname, "ls") == 0) ls_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MEMINFO
if(strcmp(progname, "meminfo") == 0) meminfo_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MKDIR
if(strcmp(progname, "mkdir") == 0) mkdir_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MKFIFO
if(strcmp(progname, "mkfifo") == 0) mkfifo_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MKNOD
if(strcmp(progname, "mknod") == 0) mknod_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MORE
if(strcmp(progname, "more") == 0) more_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MOUNT
if(strcmp(progname, "mount") == 0) mount_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MV
if(strcmp(progname, "mv") == 0) mv_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_PASSWD
if(strcmp(progname, "passwd") == 0) passwd_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_PRINTENV
if(strcmp(progname, "printenv") == 0) printenv_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_PS
if(strcmp(progname, "ps") == 0) ps_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_PWD
if(strcmp(progname, "pwd") == 0) pwd_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_REBOOT
if(strcmp(progname, "reboot") == 0) reboot_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_RM
if(strcmp(progname, "rm") == 0) rm_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_RMDIR
if(strcmp(progname, "rmdir") == 0) rmdir_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_SETENV
if(strcmp(progname, "setenv") == 0) setenv_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_SH
if(strcmp(progname, "sh") == 0) sh_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_TAIL
if(strcmp(progname, "tail") == 0) tail_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_TAR
if(strcmp(progname, "tar") == 0) tar_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_TEE
if(strcmp(progname, "tee") == 0) tee_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_TEST
if(strcmp(progname, "test") == 0) test_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_TOUCH
if(strcmp(progname, "touch") == 0) touch_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_TR
if(strcmp(progname, "tr") == 0) tr_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_UMOUNT
if(strcmp(progname, "umount") == 0) umount_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_UNIQ
if(strcmp(progname, "uniq") == 0) uniq_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_WHICH
if(strcmp(progname, "which") == 0) which_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_WHO
if(strcmp(progname, "who") == 0) who_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_WHOAMI
if(strcmp(progname, "whoami") == 0) whoami_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_YES
if(strcmp(progname, "yes") == 0) yes_main(argc, argv);
#endif
#endif
#ifdef BUSYELKS2
#ifdef CONFIG_APPLET_BANNER
if(strcmp(progname, "banner") == 0) banner_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CAL
if(strcmp(progname, "cal") == 0) cal_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CKSUM
if(strcmp(progname, "cksum") == 0) cksum_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_COMPRESS
if(strcmp(progname, "compress") == 0) compress_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_CUT
if(strcmp(progname, "cut") == 0) cut_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_DIFF
if(strcmp(progname, "diff") == 0) diff_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_ED
if(strcmp(progname, "ed") == 0) ed_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_FDISK
if(strcmp(progname, "fdisk") == 0) fdisk_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_PARTYPE
if(strcmp(progname, "partype") == 0) partype_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_RAMDISK
if(strcmp(progname, "ramdisk") == 0) ramdisk_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_SORT
if(strcmp(progname, "sort") == 0) sort_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_STTY
if(strcmp(progname, "stty") == 0) stty_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_SWAPON
if(strcmp(progname, "swapon") == 0) swapon_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_WC
if(strcmp(progname, "wc") == 0) wc_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_XARGS
if(strcmp(progname, "xargs") == 0) xargs_main(argc, argv);
#endif
#endif
#ifdef BUSYELKS3
#ifdef CONFIG_APPLET_FIND
if(strcmp(progname, "find") == 0) find_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_LOGNAME
if(strcmp(progname, "logname") == 0) logname_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MESG
if(strcmp(progname, "mesg") == 0) mesg_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_MKFS
if(strcmp(progname, "mkfs") == 0) mkfs_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_SED
if(strcmp(progname, "sed") == 0) sed_main(argc, argv);
#endif
#ifdef CONFIG_APPLET_WRITE
if(strcmp(progname, "write") == 0) write_main(argc, argv);
#endif
#endif
