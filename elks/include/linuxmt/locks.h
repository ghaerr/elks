#ifndef LX86_LINUXMT_LOCKS_H
#define LX86_LINUXMT_LOCKS_H

extern void wait_on_buffer(register struct buffer_head *);
extern void lock_buffer(struct buffer_head *);
extern void unlock_buffer(struct buffer_head *);

extern void wait_on_super(register struct super_block *);
extern void lock_super(register struct super_block *);
extern void unlock_super(register struct super_blocak *);

#endif
