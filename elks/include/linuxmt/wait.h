#ifndef LX86_LINUXMT_WAIT_H
#define LX86_LINUXMT_WAIT_H

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002

#define wake_up(_a) _wake_up(_a,1)
#define wake_up_interruptible(_a) _wake_up(_a,0)

struct wait_queue {
    char pad;
};

#ifdef __KERNEL__

struct select_table_entry {
    struct wait_queue wait;
    struct wait_queue *wait_address;
};

typedef struct select_table_struct {
    int nr;
    struct select_table_entry *entry;
}
select_table;

#define __MAX_SELECT_TABLE_ENTRIES 32

#endif

#endif
