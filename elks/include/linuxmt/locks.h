#ifndef _LINUX_LOCKS_H
#define _LINUX_LOCKS_H

extern void wait_on_buffer();
extern void lock_buffer();
extern void unlock_buffer();
extern void wait_on_super();
extern void lock_super();
extern void unlock_super();

#endif
