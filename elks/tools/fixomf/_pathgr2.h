/****************************************************************************
*
*                            Open Watcom Project
*
* Copyright (c) 2007-2020 The Open Watcom Contributors. All Rights Reserved.
*
*  ========================================================================
*
* Description:  PathGroup2 structure declaration (for _splitpath2)
*
****************************************************************************/


typedef struct pgroup2 {
    char    *drive;
    char    *dir;
    char    *fname;
    char    *ext;
    char    buffer[PATH_MAX + 4];
} pgroup2;

