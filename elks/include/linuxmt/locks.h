#ifndef __LINUX_LOCKS_H__
#define __LINUX_LOCKS_H__

extern void wait_on_buffer();
extern void lock_buffer();
extern void unlock_buffer();
extern void wait_on_super();
extern void lock_super();
extern void unlock_super();

#endif
