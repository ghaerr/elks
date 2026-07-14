#ifndef __LINUXMT_GEMTRAP_H
#define __LINUXMT_GEMTRAP_H

/*
 * ELKS resident GEM AES/VDI trap broker.
 *
 * Every interface field is one unscaled 16-bit word.  In particular, far
 * addresses remain the original offset and segment register pair; the broker
 * never converts them to a 32-bit linear address and never copies a GEM object
 * tree or parameter block.  A resident AES owner interprets the captured
 * registers in the original way:
 *
 *   AES: CX = 200 or 201, ES:BX = AES parameter block
 *   VDI: CX = 0473h,       DS:DX = VDI parameter block
 */

#define GEMCTL_REGISTER     0
#define GEMCTL_NEXT         1
#define GEMCTL_REPLY        2
#define GEMCTL_CANCEL       3
#define GEMCTL_UNREGISTER   4
#define GEMCTL_NEXT_NOWAIT  5
#define GEMCTL_ATTACH       6
#define GEMCTL_DETACH       7

/*
 * The resident owner uses tags zero through eleven for GEM's original
 * twelve PD slots.  EXIT is a kernel lifecycle record, not an AES or VDI
 * opcode, so it uses a selector which no original GEM trap can issue.
 */
#define GEMTRAP_ATTACH_TAGS 12
#define GEMTRAP_CX_EXIT     0xffffU

struct gemtrap_request {
    unsigned short slot;       /* stable ELKS task-slot number, 0..15 */
    unsigned short pid;        /* guards against reuse of a task slot */
    unsigned short ax;         /* caller AX; reply AX for REPLY */
    unsigned short bx;         /* caller BX */
    unsigned short cx;         /* AES/VDI selector */
    unsigned short dx;         /* caller DX */
    unsigned short es;         /* caller ES */
    unsigned short ds;         /* caller DS */
    unsigned short data_limit; /* exclusive byte limit of the pinned DS */
    unsigned short generation_lo; /* low half: one count per accepted trap */
    unsigned short generation_hi; /* high half: incremented on low carry */
};

typedef char gemtrap_request_must_be_22_bytes
    [(sizeof(struct gemtrap_request) == 22) ? 1 : -1];

/* The public syscall uses one near pointer in the resident owner's DS. */
#ifndef __KERNEL__
int gemctl(unsigned int operation, struct gemtrap_request *request);
#else
struct task_struct;
struct pt_regs;

int sys_gemctl(unsigned int operation, struct gemtrap_request *request);
void gemtrap_interrupt(int irq, struct pt_regs *regs);
void gemtrap_task_exec(struct task_struct *executing);
void gemtrap_task_exit(struct task_struct *exiting);
#endif

#endif
