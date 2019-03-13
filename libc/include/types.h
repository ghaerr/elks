/* Common types between the host and the target */

#ifndef _TYPES_H
#define _TYPES_H

#pragma once

// Machine dependent types

#include <asm/types.h>

/* Portable types */
/* Shorter than the C99 standard ones */

typedef unsigned char  u8_t;  /*  8 bits */
typedef unsigned short u16_t; /* 16 bits */
typedef unsigned int   u32_t; /* 32 bits */

/* This one is universal */

typedef u8_t byte_t;

/* LIBC types */

typedef u16_t pid_t;

typedef u16_t uid_t;
typedef u16_t gid_t;

#endif /* !_TYPES_H */
