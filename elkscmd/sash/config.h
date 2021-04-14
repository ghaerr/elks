/* Config file for sash.
 * Comment out #define for commands you do not require
 */

#define WILDCARDS
#define BUILTINS      /* unset to build a very small shell for 256K systems */

#ifdef BUILTINS
#define CMD_ALIAS     /* 1048 bytes. Includes unalias */
#define CMD_CHGRP     /* 2164 bytes */
#define CMD_CHMOD     /*  260 bytes */
#define CMD_CHOWN     /* 2080 bytes */
#define CMD_CMP       /*  904 bytes */
#define CMD_CP        /* 1108 bytes */
#define CMD_DD        /* 2524 bytes */
#define CMD_ECHO      /*  264 bytes */
#define CMD_ED        /* 8300 bytes */
#define CMD_GREP      /*  144 bytes */
#define CMD_HELP      /*   88 bytes */
#define CMD_KILL      /*  532 bytes */
#define CMD_LN        /*  716 bytes */
#define CMD_LS        /* 7092 bytes */
#define CMD_MKDIR     /*  140 bytes */
#define CMD_MKNOD     /*  516 bytes */
#define CMD_MORE      /*  608 bytes */
#define CMD_MOUNT     /*  600 bytes. Includes umount */
#define CMD_MV        /* 1272 bytes */
#define CMD_PRINTENV  /*  260 bytes */
#define CMD_PROMPT    /*            */
#define CMD_PWD       /*  928 bytes */
#define CMD_RM        /*  140 bytes */
#define CMD_RMDIR     /*  140 bytes */
#define CMD_SETENV    /*  129 bytes */
#define CMD_SOURCE    /*      bytes */
#define CMD_SYNC      /*   80 bytes */
#define CMD_TAR       /* 5576 bytes */
#define CMD_TOUCH     /*  236 bytes */
#define CMD_UMASK     /*  272  bytes */
#define CMD_HISTORY
#endif

#ifdef CMD_CP
#define FUNC_COPYFILE
#define FUNC_ISADIR
#define FUNC_BUILDNAME
#endif

#ifdef CMD_MV
#define FUNC_COPYFILE
#define FUNC_ISADIR
#define FUNC_BUILDNAME
#endif

#ifdef CMD_LN
#define FUNC_ISADIR
#define FUNC_BUILDNAME
#endif

#ifdef CMD_LS
#define FUNC_MODESTRING
#define FUNC_TIMESTRING
#endif

#ifdef CMD_TAR
#define FUNC_MODESTRING
#define FUNC_TIMESTRING
#endif
