#ifndef __LINUX_BIOSHD_H__
#define __LINUX_BIOSHD_H__

/* bioshd.h */
/* Copyright (C) 1994 Yggdrasil Computing, Incorporated
   4880 Stevens Creek Blvd. Suite 205
   San Jose, CA 95129-1034
   USA
   Tel (408) 261-6630
   Fax (408) 261-6631

   This file is part of the Linux Kernel

   Linux is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   Linux is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Linux; see the file COPYING.  If not, write to
   the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */
 
/* By Ross Biro July 1994 */

#define BIOSHD_INT 0x13
#define BIOSHD_TESTDRIVE 0x1000
#define BIOSHD_DRIVE_PARMS 0x0800
#define BIOSHD_RESET 0x0000
#define BIOSHD_WRITE 0x0300
#define BIOSHD_READ 0x0200

#define MAJOR_NR BIOSHD_MAJOR

#define MAX_ERRS 5

#endif
