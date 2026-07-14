/*
 * Kernel-resident broker for the original GEM INT EFh AES/VDI ABI.
 *
 * ELKS owns processes, address spaces, scheduling and sleep/wakeup.  This file
 * deliberately does not implement AES, VDI, menus, windows, resources or GEM's
 * old DOS arena allocator.  It provides the rendezvous and segment lifetime
 * rules required for one resident user process to run that original logic on
 * behalf of several real ELKS processes.
 *
 * All counters, identifiers, limits and captured registers are 8- or 16-bit
 * values.  There is no allocation: one fixed slot exists for every ELKS task.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/gemtrap.h>
#include <linuxmt/init.h>
#include <linuxmt/limits.h>
#include <linuxmt/mm.h>
#include <linuxmt/sched.h>
#include <linuxmt/types.h>

#include <arch/irq.h>

#define GEM_REQUEST_FREE        0
#define GEM_REQUEST_PENDING     1
#define GEM_REQUEST_DELIVERED   2
#define GEM_REQUEST_ABANDONED   3

#define GEM_ATTACH_FREE         0
#define GEM_ATTACH_LIVE         1
#define GEM_ATTACH_EXIT_PENDING 2
#define GEM_ATTACH_EXIT_SENT    3

#define GEM_AES_SELECTOR        200U
#define GEM_AES_ALT_SELECTOR    201U
#define GEM_VDI_SELECTOR        0x0473U

/* Packed original parameter-block sizes, in unscaled bytes. */
#define GEM_AESPB_BYTES         24U
#define GEM_VDIPB_BYTES         20U

struct gemtrap_slot {
    byte_t request_state;
    byte_t slot_number;
    pid_t request_pid;
    struct task_struct *client;
    struct gemtrap_slot *next;
    word_t ax;
    word_t bx;
    word_t cx;
    word_t dx;
    word_t es;
    word_t ds;
    segment_s *request_segment;
    word_t data_limit;
    word_t request_generation_lo;
    word_t request_generation_hi;

    byte_t attach_state;
    byte_t attach_tag;
    pid_t attach_pid;
    segment_s *attach_segment;
    word_t attach_limit;
    word_t attach_generation_lo;
    word_t attach_generation_hi;
};

/*
 * Near pointers are one word.  The exact layout makes the complete 16-task
 * table 640 bytes and catches accidental growth at compile time.
 */
typedef char gemtrap_slot_must_be_40_bytes
    [(sizeof(struct gemtrap_slot) == 40) ? 1 : -1];

static struct gemtrap_slot gem_slots[MAX_TASKS];
static struct gemtrap_slot *gem_pending_head;
static struct gemtrap_slot *gem_pending_tail;
static struct task_struct *gem_owner;
static struct wait_queue gem_owner_wait;
static word_t gem_generation_lo;
static word_t gem_generation_hi;

/* Return the fixed broker slot paired with an ELKS task table entry. */
static struct gemtrap_slot *gem_slot_for_task(struct task_struct *wanted)
{
    struct task_struct *candidate;
    struct gemtrap_slot *slot;
    byte_t number;

    candidate = &task[0];
    slot = &gem_slots[0];
    number = 0;
    while (number < (byte_t)max_tasks && number < (byte_t)MAX_TASKS) {
        if (candidate == wanted) {
            slot->slot_number = number;
            return slot;
        }
        candidate++;
        slot++;
        number++;
    }
    return NULL;
}

static struct gemtrap_slot *gem_slot_by_number(word_t number)
{
    struct gemtrap_slot * volatile slot;
    word_t index;

    if (number >= (word_t)MAX_TASKS || number >= (word_t)max_tasks)
        return NULL;

    slot = &gem_slots[0];
    index = 0;
    while (index < number) {
        slot++;
        index++;
    }
    return (struct gemtrap_slot *)slot;
}

/*
 * Allocate one boot-lifetime trap generation using two explicit 16-bit
 * halves.  The scale is exactly one accepted trap per increment.  Low-half
 * wrap carries one into the high half; ffff:ffff saturates and is never
 * reused.  No 32-bit temporary, helper, multiplication or division exists.
 */
static int gem_allocate_generation(struct gemtrap_slot *slot)
{
    if (gem_generation_lo == 0xffffU
        && gem_generation_hi == 0xffffU)
        return -EOVERFLOW;

    gem_generation_lo++;
    if (gem_generation_lo == 0)
        gem_generation_hi++;
    slot->request_generation_lo = gem_generation_lo;
    slot->request_generation_hi = gem_generation_hi;
    return 0;
}

/* Find a live request using the complete stale-operation guard. */
static struct gemtrap_slot *gem_find_request(word_t number, pid_t pid,
                                              word_t generation_lo,
                                              word_t generation_hi)
{
    struct gemtrap_slot *slot;

    slot = gem_slot_by_number(number);
    if (!slot || slot->request_state == GEM_REQUEST_FREE
        || slot->request_pid != pid
        || slot->request_generation_lo != generation_lo
        || slot->request_generation_hi != generation_hi)
        return NULL;
    return slot;
}

static void gem_queue_append(struct gemtrap_slot *slot)
{
    slot->next = NULL;
    if (gem_pending_tail)
        gem_pending_tail->next = slot;
    else
        gem_pending_head = slot;
    gem_pending_tail = slot;
}

static void gem_queue_remove(struct gemtrap_slot *wanted)
{
    struct gemtrap_slot *slot;
    struct gemtrap_slot *previous;

    previous = NULL;
    slot = gem_pending_head;
    while (slot) {
        if (slot == wanted) {
            if (previous)
                previous->next = slot->next;
            else
                gem_pending_head = slot->next;
            if (gem_pending_tail == slot)
                gem_pending_tail = previous;
            slot->next = NULL;
            return;
        }
        previous = slot;
        slot = slot->next;
    }
}

/*
 * Validate [offset, offset + length) without ever forming a wrapped sum.
 * limit is the exclusive 16-bit ELKS data/stack segment byte boundary.
 */
static int gem_range_valid(word_t offset, word_t length, word_t limit)
{
    if (offset > limit)
        return 0;
    return length <= (word_t)(limit - offset);
}

static void gem_drop_request_segment(struct gemtrap_slot *slot)
{
    if (slot->request_segment) {
        seg_put(slot->request_segment);
        slot->request_segment = NULL;
    }
    slot->data_limit = 0;
}

/* Release one request while leaving any persistent attachment untouched. */
static void gem_release_request(struct gemtrap_slot *slot, int result)
{
    struct task_struct *client;

    client = slot->client;
    if (slot->request_state == GEM_REQUEST_PENDING)
        gem_queue_remove(slot);

    /*
     * INT EF returns one original 16-bit AES/VDI status in AX.  All other
     * saved registers remain exactly as supplied by the client.  Converting
     * the signed result to word_t preserves its two's-complement bit pattern;
     * no rounding or saturation is involved.
     */
    if (client)
        client->t_regs.ax = (word_t)result;

    gem_drop_request_segment(slot);
    slot->request_state = GEM_REQUEST_FREE;
    slot->request_pid = 0;
    slot->client = NULL;
    slot->next = NULL;
    slot->request_generation_lo = 0;
    slot->request_generation_hi = 0;

    /* The signal-abort path is already running in its own client task. */
    if (client && client != current
        && (client->state == TASK_INTERRUPTIBLE
            || client->state == TASK_UNINTERRUPTIBLE))
        wake_up_process(client);
}

/*
 * The owner has already seen this request and may be reading the client DS.
 * Let the signalled client continue with -EINTR, but retain the segment pin
 * until the owner acknowledges the stale request with REPLY or CANCEL.
 */
static void gem_abandon_request(struct gemtrap_slot *slot, int result)
{
    if (slot->client)
        slot->client->t_regs.ax = (word_t)result;
    slot->client = NULL;
    slot->request_state = GEM_REQUEST_ABANDONED;
}

static void gem_release_attachment(struct gemtrap_slot *slot)
{
    if (slot->attach_segment) {
        seg_put(slot->attach_segment);
        slot->attach_segment = NULL;
    }
    slot->attach_state = GEM_ATTACH_FREE;
    slot->attach_tag = 0;
    slot->attach_pid = 0;
    slot->attach_limit = 0;
    slot->attach_generation_lo = 0;
    slot->attach_generation_hi = 0;
}

static void gem_release_owner(int result)
{
    struct gemtrap_slot *slot;
    byte_t count;

    gem_owner = NULL;
    gemtrap_vector_disable();

    slot = &gem_slots[0];
    count = 0;
    while (count < (byte_t)MAX_TASKS) {
        if (slot->request_state != GEM_REQUEST_FREE)
            gem_release_request(slot, result);
        if (slot->attach_state != GEM_ATTACH_FREE)
            gem_release_attachment(slot);
        slot++;
        count++;
    }
    gem_pending_head = NULL;
    gem_pending_tail = NULL;
}

static struct gemtrap_slot *gem_exit_pending(void)
{
    struct gemtrap_slot *slot;
    byte_t count;

    slot = &gem_slots[0];
    count = 0;
    while (count < (byte_t)MAX_TASKS) {
        if (slot->attach_state == GEM_ATTACH_EXIT_PENDING)
            return slot;
        slot++;
        count++;
    }
    return NULL;
}

static void gem_copy_trap_request(struct gemtrap_request *request,
                                  struct gemtrap_slot *slot)
{
    request->slot = slot->slot_number;
    request->pid = slot->request_pid;
    request->ax = slot->ax;
    request->bx = slot->bx;
    request->cx = slot->cx;
    request->dx = slot->dx;
    request->es = slot->es;
    request->ds = slot->ds;
    request->data_limit = slot->data_limit;
    request->generation_lo = slot->request_generation_lo;
    request->generation_hi = slot->request_generation_hi;
}

static void gem_copy_exit_request(struct gemtrap_request *request,
                                  struct gemtrap_slot *slot)
{
    request->slot = slot->slot_number;
    request->pid = slot->attach_pid;
    request->ax = (word_t)slot->attach_tag;
    request->bx = 0;
    request->cx = GEMTRAP_CX_EXIT;
    request->dx = 0;
    request->es = slot->attach_segment ? slot->attach_segment->base : 0;
    request->ds = request->es;
    request->data_limit = slot->attach_limit;
    request->generation_lo = slot->attach_generation_lo;
    request->generation_hi = slot->attach_generation_hi;
}

/*
 * Dequeue one lifecycle record or trap.  Exit records are deliberately first:
 * after exec the same PID may immediately queue APPL_INIT from a new segment,
 * and the owner must destroy the old PD state before attaching that new image.
 */
static int FARPROC gem_take_request(struct gemtrap_request *user_request,
                                    int may_wait)
{
    struct gemtrap_request request;
    struct gemtrap_slot *exit_slot;
    struct gemtrap_slot *request_slot;
    int error;

    if (!may_wait) {
        exit_slot = gem_exit_pending();
        request_slot = gem_pending_head;
        if (!exit_slot && !request_slot)
            return -EAGAIN;
    } else {
        for (;;) {
            prepare_to_wait_interruptible(&gem_owner_wait);
            exit_slot = gem_exit_pending();
            request_slot = gem_pending_head;
            if (exit_slot || request_slot)
                break;
            if (current->signal) {
                finish_wait(&gem_owner_wait);
                return -EINTR;
            }
            do_wait();
            finish_wait(&gem_owner_wait);
        }
    }

    if (exit_slot)
        gem_copy_exit_request(&request, exit_slot);
    else
        gem_copy_trap_request(&request, request_slot);

    error = verified_memcpy_tofs(user_request, &request, sizeof(request));
    if (!error) {
        if (exit_slot)
            exit_slot->attach_state = GEM_ATTACH_EXIT_SENT;
        else {
            gem_queue_remove(request_slot);
            request_slot->request_state = GEM_REQUEST_DELIVERED;
        }
    }
    if (may_wait)
        finish_wait(&gem_owner_wait);
    return error;
}

/*
 * Called by the normal ELKS software-interrupt path with the complete saved
 * user register frame.  Only the primary data segment is pinned; the kernel
 * does not parse or copy nested AES/VDI pointers.
 */
void gemtrap_interrupt(int irq, struct pt_regs *regs)
{
    struct gemtrap_slot *slot;
    segment_s *data_segment;
    word_t parameter_offset;
    word_t parameter_bytes;

    if (irq != IDX_GEM || !gem_owner) {
        regs->ax = (word_t)-ENOSYS;
        return;
    }
    if (current == gem_owner) {
        /* The sole dequeuer cannot sleep waiting for its own reply. */
        regs->ax = (word_t)-EDEADLK;
        return;
    }

    data_segment = current->mm[SEG_DATA];
    if (!data_segment || regs->ds != data_segment->base) {
        regs->ax = (word_t)-EFAULT;
        return;
    }

    if (regs->cx == GEM_AES_SELECTOR
        || regs->cx == GEM_AES_ALT_SELECTOR) {
        if (regs->es != data_segment->base) {
            regs->ax = (word_t)-EFAULT;
            return;
        }
        parameter_offset = regs->bx;
        parameter_bytes = GEM_AESPB_BYTES;
    } else if (regs->cx == GEM_VDI_SELECTOR) {
        parameter_offset = regs->dx;
        parameter_bytes = GEM_VDIPB_BYTES;
    } else {
        regs->ax = (word_t)-ENOSYS;
        return;
    }

    if (!gem_range_valid(parameter_offset, parameter_bytes,
                         current->t_endseg)) {
        regs->ax = (word_t)-EFAULT;
        return;
    }

    slot = gem_slot_for_task(current);
    if (!slot || slot->request_state != GEM_REQUEST_FREE) {
        regs->ax = (word_t)-EAGAIN;
        return;
    }
    if (gem_allocate_generation(slot)) {
        regs->ax = (word_t)-EOVERFLOW;
        return;
    }

    slot->request_state = GEM_REQUEST_PENDING;
    slot->request_pid = current->pid;
    slot->client = current;
    slot->ax = regs->ax;
    slot->bx = regs->bx;
    slot->cx = regs->cx;
    slot->dx = regs->dx;
    slot->es = regs->es;
    slot->ds = regs->ds;
    slot->request_segment = seg_get(data_segment);
    slot->data_limit = current->t_endseg;
    gem_queue_append(slot);

    wake_up(&gem_owner_wait);

    /*
     * Keep the INT EF kernel continuation until the owner releases the slot
     * or a signal becomes pending.  Marking the task interruptible before the
     * state test closes the single-CPU wakeup race without a lock.
     */
    for (;;) {
        current->state = TASK_INTERRUPTIBLE;
        if (slot->request_state == GEM_REQUEST_FREE)
            break;
        if (current->signal) {
            if (slot->request_state == GEM_REQUEST_PENDING)
                gem_release_request(slot, -EINTR);
            else if (slot->request_state == GEM_REQUEST_DELIVERED)
                gem_abandon_request(slot, -EINTR);
            break;
        }
        schedule();
    }
    current->state = TASK_RUNNING;
}

static int FARPROC gem_attach_request(struct gemtrap_request *request)
{
    struct gemtrap_slot *slot;
    struct gemtrap_slot *candidate;
    byte_t count;
    byte_t tag;

    if (request->ax >= GEMTRAP_ATTACH_TAGS)
        return -EINVAL;
    tag = (byte_t)request->ax;

    slot = gem_find_request(request->slot, (pid_t)request->pid,
                            request->generation_lo,
                            request->generation_hi);
    if (!slot || slot->request_state != GEM_REQUEST_DELIVERED
        || !slot->request_segment)
        return -ESRCH;
    if (request->ds != slot->ds || request->ds != slot->request_segment->base)
        return -ESRCH;

    /*
     * A task-table slot may be reused while an old image's EXIT has been sent
     * but not detached.  Never overwrite that attachment pointer: doing so
     * would lose its sole seg_put() path.  Only the exact live
     * image/tag/DS/generation tuple is an idempotent repeat.
     */
    if (slot->attach_state != GEM_ATTACH_FREE) {
        if (slot->attach_state == GEM_ATTACH_LIVE
            && slot->attach_pid == slot->request_pid
            && slot->attach_tag == tag
            && slot->attach_segment == slot->request_segment
            && slot->attach_generation_lo == slot->request_generation_lo
            && slot->attach_generation_hi == slot->request_generation_hi)
            return 0;
        return -EBUSY;
    }

    candidate = &gem_slots[0];
    count = 0;
    while (count < (byte_t)MAX_TASKS) {
        if (candidate->attach_state != GEM_ATTACH_FREE) {
            if (candidate->attach_tag == tag
                || candidate->attach_pid == slot->request_pid)
                return -EBUSY;
        }
        candidate++;
        count++;
    }

    slot->attach_segment = seg_get(slot->request_segment);
    slot->attach_limit = slot->data_limit;
    slot->attach_pid = slot->request_pid;
    slot->attach_tag = tag;
    slot->attach_generation_lo = slot->request_generation_lo;
    slot->attach_generation_hi = slot->request_generation_hi;
    slot->attach_state = GEM_ATTACH_LIVE;
    return 0;
}

static int FARPROC gem_detach_request(struct gemtrap_request *request)
{
    struct gemtrap_slot *slot;

    if (request->ax >= GEMTRAP_ATTACH_TAGS)
        return -EINVAL;
    slot = gem_slot_by_number(request->slot);
    if (!slot || slot->attach_state == GEM_ATTACH_FREE
        || slot->attach_pid != (pid_t)request->pid
        || slot->attach_tag != (byte_t)request->ax
        || !slot->attach_segment
        || slot->attach_segment->base != request->ds
        || slot->attach_generation_lo != request->generation_lo
        || slot->attach_generation_hi != request->generation_hi)
        return -ESRCH;

    gem_release_attachment(slot);
    return 0;
}

int sys_gemctl(unsigned int operation, struct gemtrap_request *user_request)
{
    struct gemtrap_request request;
    struct gemtrap_slot *slot;
    int error;

    if (operation == GEMCTL_REGISTER) {
        if (gem_owner && gem_owner != current)
            return -EBUSY;
        if (!gem_owner) {
            gem_owner = current;
            gemtrap_vector_enable();
        }
        return 0;
    }

    if (gem_owner != current)
        return -EPERM;

    if (operation == GEMCTL_NEXT || operation == GEMCTL_NEXT_NOWAIT) {
        if (!user_request)
            return -EFAULT;
        return gem_take_request(user_request, operation == GEMCTL_NEXT);
    }

    if (operation == GEMCTL_UNREGISTER) {
        gem_release_owner(-EPIPE);
        return 0;
    }

    if (!user_request)
        return -EFAULT;
    error = verified_memcpy_fromfs(&request, user_request, sizeof(request));
    if (error)
        return error;

    if (operation == GEMCTL_ATTACH)
        return gem_attach_request(&request);
    if (operation == GEMCTL_DETACH)
        return gem_detach_request(&request);
    if (operation != GEMCTL_REPLY && operation != GEMCTL_CANCEL)
        return -EINVAL;

    slot = gem_find_request(request.slot, (pid_t)request.pid,
                            request.generation_lo,
                            request.generation_hi);
    if (!slot)
        return -ESRCH;
    if (slot->request_state == GEM_REQUEST_ABANDONED) {
        gem_release_request(slot, -EINTR);
        return -ESRCH;
    }

    if (operation == GEMCTL_REPLY) {
        if (slot->request_state != GEM_REQUEST_DELIVERED)
            return -EINVAL;
        gem_release_request(slot, (int)request.ax);
    } else {
        gem_release_request(slot, -EINTR);
    }
    return 0;
}

/* Mark a retained old image before finalize_exec drops its normal DS ref. */
void gemtrap_task_exec(struct task_struct *executing)
{
    struct gemtrap_slot *slot;

    if (!gem_owner)
        return;
    if (executing == gem_owner) {
        /*
         * The registered server identity is an executable image, not merely
         * a PID.  Replacing that image must disable INT EFh and release every
         * request and attachment exactly as owner exit or UNREGISTER does.
         */
        gem_release_owner(-EPIPE);
        return;
    }
    slot = gem_slot_for_task(executing);
    if (!slot || slot->attach_state != GEM_ATTACH_LIVE
        || slot->attach_pid != executing->pid)
        return;

    slot->attach_state = GEM_ATTACH_EXIT_PENDING;
    wake_up(&gem_owner_wait);
}

void gemtrap_task_exit(struct task_struct *exiting)
{
    struct gemtrap_slot *slot;

    if (gem_owner == exiting) {
        gem_release_owner(-EPIPE);
        return;
    }

    slot = gem_slot_for_task(exiting);
    if (!slot)
        return;

    if (slot->request_state == GEM_REQUEST_PENDING)
        gem_release_request(slot, -EINTR);
    else if (slot->request_state == GEM_REQUEST_DELIVERED)
        gem_abandon_request(slot, -EINTR);

    if (slot->attach_state == GEM_ATTACH_LIVE
        && slot->attach_pid == exiting->pid)
        slot->attach_state = GEM_ATTACH_EXIT_PENDING;
    wake_up(&gem_owner_wait);
}
