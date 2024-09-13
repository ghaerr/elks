#ifndef __LINUXMT_SYSCTL_H
#define __LINUXMT_SYSCTL_H

/* sysctl(2) parameters */

#define CTL_MAXNAMESZ   32      /* max size of option name */

#define CTL_LIST        0       /* get option list from index */
#define CTL_GET         (-1)    /* get option value by name */
#define CTL_SET         (-2)    /* set option value by name */

#endif
