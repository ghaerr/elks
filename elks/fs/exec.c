/*
 *      Minix 16bit format binary image loader and exec support routines.
 *
 *      5th Jan 1999    Alistair Riddoch (ajr@ecs.soton.ac.uk)
 *                      Added shared lib support consisting of a sys_dlload
 *                      Which loads dll text into new code segment, and dll
 *                      data into processes data segment.
 *                      This required support for the mh.unused field in the
 *                      bin header to contain the size of the dlls data, which
 *                      is used in sys_exec to offset loading of the processes
 *                      data at run time, new MINIX_DLLID library format, and
 *                      new MINIX_S_SPLITID binary format.
 *      21th Jan 2000   Alistair Riddoch (ajr@ecs.soton.ac.uk)
 *                      Rethink of binary format leads me to think that the
 *                      shared library route is not worth pursuing with the
 *                      implemented so far. Removed hacks from exec()
 *                      related to this, and instead add support for
 *                      binaries which have the stack below the data
 *                      segment. Binaries in this form have a large minix
 *                      format header (0x30 bytes rather than 0x20) which
 *                      contain a field which is reffered to in the a.out.h
 *                      file as "data relocation base". This is taken as the
 *                      address within the data segment where the program
 *                      expects its data segment to begin. Below this we
 *                      then put the stack. This requires adding support
 *                      for allocating blocks of memory which do not start
 *                      at the bottom of the segment. See arch/i86/mm/malloc.c
 *                      for details.
 *
 *      30 Jun 2024     Greg Haerr. Added support for loading OS/2 v1.x binaries.
 */

#include <linuxmt/config.h>
#include <linuxmt/errno.h>
#include <linuxmt/fcntl.h>
#include <linuxmt/stat.h>
#include <linuxmt/string.h>
#include <linuxmt/sched.h>
#include <linuxmt/kernel.h>
#include <linuxmt/signal.h>
#include <linuxmt/time.h>
#include <linuxmt/mm.h>
#include <linuxmt/minix.h>
#include <linuxmt/os2.h>
#include <linuxmt/init.h>
#include <linuxmt/debug.h>
#include <linuxmt/memory.h>

#include <arch/segment.h>

/* for relocation debugging change to printk */
#define debug_reloc     debug
#define debug_reloc2    debug
#define debug_os2       debug

static int execve_aout(struct inode *inode, struct file *filp, char *sptr, size_t slen);
static void finalize_exec(struct inode *inode, segment_s *seg_code, segment_s *seg_data,
    word_t entry, int multisegment);

#ifdef CONFIG_EXEC_OS2
static int execve_os2(struct inode *inode, struct file *filp, char *sptr, size_t slen);
static segment_s *mm_table[MAX_SEGS]; /* holds process segments until exec guaranteed */
#endif

int sys_execve(const char *filename, char *sptr, size_t slen)
{
    int retval;
    struct file *filp;
    seg_t ds;
    struct inode *inode;
    word_t magic;

    /* Open the image */
    debug_file("EXEC(%P): '%t' env %d\n", filename, slen);

    retval = open_namei(filename, 0, 0, &inode, NULL);
    debug("EXEC: open returned %d\n", -retval);
    if (retval) goto error_exec1;

    /* Get a reading file handle */
    if ((retval = open_filp(O_RDONLY, inode, &filp))) goto error_exec2;
    if (!(filp->f_op) || !(filp->f_op->read)) goto error_exec2_5;

    /* Read the header */
    ds = current->t_regs.ds;
    current->t_regs.ds = kernel_ds;
    retval = filp->f_op->read(inode, filp, (char *)&magic, sizeof(magic));
    current->t_regs.ds = ds;
    if (retval != sizeof(magic)) goto error_exec2_5;

#ifdef CONFIG_EXEC_OS2
    if (magic == MZMAGIC)
        retval = execve_os2(inode, filp, sptr, slen);
    else
#endif
    if (magic == AOUTMAGIC)
        retval = execve_aout(inode, filp, sptr, slen);
    else retval = -ENOEXEC;
    goto normal_out;

  error_exec2_5:
    if (retval >= 0)
        retval = -ENOEXEC;
  normal_out:
    close_filp(inode, filp);

    if (retval)
  error_exec2:
        iput(inode);
  error_exec1:
    debug("EXEC(%P): return %d\n", retval);
    return retval;
}

#ifndef __GNUC__
/* FIXME: evaluates some operands twice */
#   define add_overflow(a, b, res) \
        (*(res) = (a) + (b), *(res) < (b))
#   define bytes_to_paras(bytes) \
        ((segext_t)(((unsigned long)(bytes) + 15) >> 4))
#else
#   define add_overflow __builtin_add_overflow
#   define bytes_to_paras(bytes) __extension__({ \
        segext_t __w; \
        asm("addw $15, %0; rcrw %0" \
              : "=&r" (__w) \
              : "0" (bytes) \
              : "cc"); \
        __w >> 3; })
#endif

#ifdef CONFIG_EXEC_MMODEL
/*
 * Read relocations for a particular segment and apply them
 * Only IA-16 segment relocations are accepted
 */
static int relocate(seg_t place_base, unsigned long rsize, segment_s *seg_code,
               segment_s *seg_data, struct inode *inode, struct file *filp, size_t tseg)
{
    int retval = 0;
    seg_t save_ds = current->t_regs.ds;
    struct minix_reloc reloc;   //FIXME too large
    word_t val;

    if ((int)rsize % sizeof(struct minix_reloc))
        return -EINVAL;
    current->t_regs.ds = kernel_ds;
    debug_reloc("EXEC: applying %04lx bytes of relocations to segment %x\n",
           (unsigned long)rsize, place_base);
    while (rsize >= sizeof(struct minix_reloc)) {
        retval = filp->f_op->read(inode, filp, (char *)&reloc, sizeof(reloc));
        if (retval != sizeof(reloc))
            goto error;
        switch (reloc.r_type) {
        case R_SEGWORD:
            switch (reloc.r_symndx) {
            case S_TEXT:
                val = seg_code->base; break;
            case S_FTEXT:
                val = seg_code->base + bytes_to_paras(tseg); break;
            case S_DATA:
                val = seg_data->base; break;
            default:
                debug_reloc("EXEC: bad relocation symbol index 0x%x\n", reloc.r_symndx);
                goto error;
            }
            debug_reloc("EXEC: reloc %d,%d,%04x:%04x %04x -> %04x\n",
                reloc.r_type, reloc.r_symndx, place_base, (word_t)reloc.r_vaddr,
                peekw((word_t)reloc.r_vaddr, place_base), val);
            pokew((word_t)reloc.r_vaddr, place_base, val);
            break;
        default:
            debug_reloc("EXEC: bad relocation type 0x%x\n", reloc.r_type);
            goto error;
        }
        rsize -= sizeof(struct minix_reloc);
    }
    current->t_regs.ds = save_ds;
    return 0;
  error:
    debug_reloc("EXEC: error in relocations\n");
    current->t_regs.ds = save_ds;
    if (retval >= 0)
        retval = -EINVAL;
    return retval;
}
#endif

static int execve_aout(struct inode *inode, struct file *filp, char *sptr, size_t slen)
{
    register struct task_struct *currentp;
    int retval;
    seg_t ds = current->t_regs.ds;
    seg_t base_data = 0;
    segment_s * seg_code;
    segment_s * seg_data;
    size_t len, min_len, heap, stack = 0;
    size_t bytes;
    segext_t paras;
    ASYNCIO_REENTRANT struct minix_exec_hdr mh;         /* 32 bytes */
#ifdef CONFIG_EXEC_MMODEL
    ASYNCIO_REENTRANT struct elks_supl_hdr esuph;       /* 24 bytes */
    int need_reloc_code = 1;
#endif

    /* (Re)read the header */
    current->t_regs.ds = kernel_ds;
    filp->f_pos = 0;
    retval = filp->f_op->read(inode, filp, (char *) &mh, sizeof(mh));

    /* Sanity check it */
    if (retval != sizeof(mh)) goto error_exec3;

    if ((mh.type != MINIX_SPLITID_AHISTORICAL && mh.type != MINIX_SPLITID) ||
        (size_t)mh.tseg == 0) {
        debug("EXEC: bad header, result %d\n", retval);
        goto error_exec3;
    }

    /* Look for the binary in memory */
    seg_code = 0;
    currentp = &task[0];
    do {
        if ((currentp->state <= TASK_STOPPED) && (currentp->t_inode == inode)) {
            debug("EXEC found copy\n");
            seg_code = currentp->mm[SEG_CODE];
            break;
        }
    } while (++currentp < &task[max_tasks]);
    currentp = current;

    min_len = (size_t)mh.dseg;
    if (add_overflow(min_len, (size_t)mh.bseg, &min_len)) {
        retval = -EINVAL;
        goto error_exec3;
    }

#ifdef CONFIG_EXEC_MMODEL
    memset(&esuph, 0, sizeof(esuph));
#endif

    switch (mh.hlen) {
    case EXEC_MINIX_HDR_SIZE:
        break;
#ifdef CONFIG_EXEC_MMODEL
    case EXEC_RELOC_HDR_SIZE:
    case EXEC_FARTEXT_HDR_SIZE:
        /* BIG HEADER */
        retval = filp->f_op->read(inode, filp, (char *) &esuph,
                                  mh.hlen - EXEC_MINIX_HDR_SIZE);
        if (retval != mh.hlen - EXEC_MINIX_HDR_SIZE) {
            debug("EXEC: Bad secondary header, result %u\n", retval);
            goto error_exec3;
        }
        debug_reloc("EXEC: text rels %d, data rels %d, "
               "ftext rels %d, text/data base %x,%x\n",
               (int)esuph.msh_trsize/sizeof(struct minix_reloc),
               (int)esuph.msh_drsize/sizeof(struct minix_reloc),
               (int)esuph.esh_ftrsize/sizeof(struct minix_reloc),
               (int)esuph.msh_tbase, (int)esuph.msh_dbase);
#ifndef CONFIG_EXEC_COMPRESS
        if (esuph.esh_compr_tseg || esuph.esh_compr_ftseg || esuph.esh_compr_dseg)
            goto error_exec3;
#endif
        retval = -EINVAL;
        if (esuph.msh_tbase != 0)
            goto error_exec3;
        base_data = esuph.msh_dbase;
#ifdef CONFIG_EXEC_LOW_STACK
        if (base_data & 0xf)
            goto error_exec3;
        if (base_data != 0)
            debug("EXEC: New type executable stack = %x\n", base_data);

        if (add_overflow(min_len, base_data, &min_len)) /* adds stack size*/
            goto error_exec3;
#else
        if (base_data != 0)
            goto error_exec3;
#endif
        break;
#endif /* CONFIG_EXEC_MMODEL*/
    default:
        goto error_exec3;
    }

    /*
     * mh.version == 1: chmem is size of heap, 0 means use default heap
     * mh.version == 0: old ld86 used chmem as size of data+bss+heap+stack
     */
    switch (mh.version) {
    default:
        goto error_exec3;
    case 1:
        len = min_len;
#ifdef CONFIG_EXEC_LOW_STACK
        if (!base_data)
#endif
        {
            stack = mh.minstack? mh.minstack: INIT_STACK;
            if (add_overflow(len, stack, &len)) {       /* add stack */
                retval = -EFBIG;
                goto error_exec3;
            }
            if (add_overflow(len, slen, &len)) {        /* add argv, envp */
                retval = -E2BIG;
                goto error_exec3;
            }
        }
        heap = mh.chmem? mh.chmem: INIT_HEAP;
        if (heap >= 0xFFF0) {                           /* max heap specified*/
            if (len < 0xFFF0)                           /* len could be near overflow from above*/
                len = 0xFFF0;
        } else {
            if (add_overflow(len, heap, &len)) {        /* add heap */
                retval = -EFBIG;
                goto error_exec3;
            }
        }
        debug("EXEC: stack %u heap %u env %u total %u\n", stack, heap, slen, len);
        break;
    case 0:
        len = mh.chmem;
        if (len) {
            if (len <= min_len) {                       /* check bad chmem*/
                retval = -EINVAL;
                goto error_exec3;
            }
            stack = 0;                                  /* no protected stack space*/
            heap = len - min_len;
            if (heap < slen) {                          /* check space for environment*/
                retval = -E2BIG;
                goto error_exec3;
            }
        } else {
            stack = INIT_STACK;
            len = min_len;
#ifdef CONFIG_EXEC_LOW_STACK
            if (base_data) {
                if (add_overflow(len, INIT_HEAP, &len)) {
                    retval = -EFBIG;
                    goto error_exec3;
                }
            } else
#endif
            {
                if (add_overflow(len, INIT_HEAP + INIT_STACK, &len)) {
                    retval = -EFBIG;
                    goto error_exec3;
                }
            }
            if (add_overflow(len, slen, &len)) {
                retval = -E2BIG;
                goto error_exec3;
            }
        }
        debug("EXEC v0: stack %u heap %u env %u total %u\n", stack, len-min_len-stack-slen, slen, len);
    }

    /* Round data segment length up to a paragraph boundary
       (If the length overflows at this point, blame argv and envp...) */
    if (add_overflow(len, 15, &len)) {
        retval = -E2BIG;
        goto error_exec3;
    }
    len &= ~(size_t)15;

    debug("EXEC: Malloc time\n");

    /*
     *      Looks good. Get the memory we need
     */
    if (!seg_code) {
        bytes = (size_t)mh.tseg;
        paras = bytes_to_paras(bytes);
        retval = -ENOMEM;
#ifdef CONFIG_EXEC_COMPRESS
        if (esuph.esh_compr_tseg || esuph.esh_compr_ftseg) {
            if (esuph.esh_compr_tseg)
                bytes = esuph.esh_compr_tseg;
            paras += 1;         /* add 16 bytes for safety offset */
        }
#endif
#ifdef CONFIG_EXEC_MMODEL
        paras += bytes_to_paras((size_t)esuph.esh_ftseg);
#endif
        debug_reloc("EXEC: allocating %04x paras (%04x bytes) for text segment(s)\n", paras,
            bytes);
        seg_code = seg_alloc(paras, SEG_FLAG_CSEG);
        if (!seg_code) goto error_exec3;
        currentp->t_regs.ds = seg_code->base;
        retval = filp->f_op->read(inode, filp, 0, bytes);
        if (retval != bytes) {
            debug("EXEC(tseg read): bad result %u, expected %u\n", retval, bytes);
            goto error_exec4;
        }
#ifdef CONFIG_EXEC_COMPRESS
        retval = -ENOEXEC;
        if (esuph.esh_compr_tseg &&
            decompress(0, seg_code->base, (size_t)mh.tseg, bytes, 16) != (size_t)mh.tseg)
                goto error_exec4;
#endif
#ifdef CONFIG_EXEC_MMODEL
        bytes = esuph.esh_ftseg;
        if (bytes) {
#ifdef CONFIG_EXEC_COMPRESS
            if (esuph.esh_compr_ftseg)
                bytes = esuph.esh_compr_ftseg;
#endif
            currentp->t_regs.ds = seg_code->base + bytes_to_paras((size_t)mh.tseg);
            retval = filp->f_op->read(inode, filp, 0, bytes);
            if (retval != bytes) {
                debug("EXEC(ftseg read): bad result %u, expected %u\n", retval, bytes);
                goto error_exec4;
            }
#ifdef CONFIG_EXEC_COMPRESS
            retval = -ENOEXEC;
            if (esuph.esh_compr_ftseg &&
                decompress(0, seg_code->base + bytes_to_paras((size_t)mh.tseg),
                    (size_t)esuph.esh_ftseg, bytes, 16) != (size_t)esuph.esh_ftseg)
                        goto error_exec4;
#endif
        }
#endif
    } else {
        seg_get (seg_code);
#ifdef CONFIG_EXEC_MMODEL
        filp->f_pos += esuph.esh_compr_tseg? esuph.esh_compr_tseg: (size_t)mh.tseg;
        filp->f_pos += esuph.esh_compr_ftseg? esuph.esh_compr_ftseg: (size_t)esuph.esh_ftseg;
        need_reloc_code = 0;
#else
        filp->f_pos += (size_t)mh.tseg;
#endif
    }

    paras = len >> 4;
    retval = -ENOMEM;
    debug_reloc("EXEC: allocating %04x paras (%04x bytes) for data segment\n", paras, len);
    seg_data = seg_alloc (paras, SEG_FLAG_DSEG);
    if (!seg_data) goto error_exec4;
    debug("EXEC: Malloc succeeded - cs=%x ds=%x\n", seg_code->base, seg_data->base);

    bytes = (size_t)mh.dseg;
#ifdef CONFIG_EXEC_COMPRESS
    if (esuph.esh_compr_dseg) {
            bytes = esuph.esh_compr_dseg;
            //if (mh.bss < 16 && !stack && !heap)
                //paras += 1;           /* add 16 bytes for safety offset */
    }
#endif
    currentp->t_regs.ds = seg_data->base;
    retval = filp->f_op->read(inode, filp, (char *)base_data, bytes);
    if (retval != bytes) {
        debug("EXEC(dseg read): bad result %d, expected %u\n", retval, bytes);
        goto error_exec5;
    }
#ifdef CONFIG_EXEC_COMPRESS
        if (esuph.esh_compr_dseg &&
            decompress(0, seg_data->base, (size_t)mh.dseg, bytes, 16) != (size_t)mh.dseg)
                goto error_exec5;
#endif

#ifdef CONFIG_EXEC_MMODEL
    if (need_reloc_code) {
        /* Read and apply text segment relocations */
        retval = relocate(seg_code->base, esuph.msh_trsize, seg_code, seg_data,
                          inode, filp, mh.tseg);
        if (retval != 0)
            goto error_exec5;
        /* Read and apply far text segment relocations */
        retval = relocate(seg_code->base + bytes_to_paras((size_t)mh.tseg),
                          esuph.esh_ftrsize, seg_code, seg_data,
                          inode, filp, mh.tseg);
        if (retval != 0)
            goto error_exec5;
    } else {
        /* If reusing existing text segments, no need to re-relocate */
        filp->f_pos += esuph.msh_trsize;
        filp->f_pos += esuph.esh_ftrsize;
    }
    /* Read and apply data relocations */
    retval = relocate(seg_data->base, esuph.msh_drsize, seg_code, seg_data,
                      inode, filp, mh.tseg);
    if (retval != 0)
        goto error_exec5;
#endif

    /* From this point, exec() will surely succeed */

    /* clear bss */
    fmemsetb((char *)(size_t)mh.dseg + base_data, seg_data->base, 0, (size_t)mh.bseg);

    /* set data/stack limits and copy argc/argv */
    currentp->t_enddata = (size_t)mh.dseg + (size_t)mh.bseg + base_data;
    currentp->t_endseg = len;
    currentp->t_minstack = stack;

#ifdef CONFIG_EXEC_LOW_STACK
    currentp->t_begstack = ((base_data   /* Just above the top of stack */
        ? base_data
        : currentp->t_endseg) - slen) & ~1;
#else
    currentp->t_begstack = (currentp->t_endseg - slen) & ~1; /* force even SP and argv */
#endif
    fmemcpyb((char *)currentp->t_begstack, seg_data->base, sptr, ds, slen);

    finalize_exec(inode, seg_code, seg_data, (word_t)mh.entry, 0);
    return 0;           /* success */

  error_exec5:
    seg_put (seg_data);

  error_exec4:
    seg_put (seg_code);

  error_exec3:
    current->t_regs.ds = ds;
    if (retval >= 0)
        retval = -ENOEXEC;
    return retval;
}

/* seg_code is entry code segment, seg_data is main (auto stack/heap) data segment */
static void finalize_exec(struct inode *inode, segment_s *seg_code, segment_s *seg_data,
    word_t entry, int multisegment)
{
    struct task_struct *currentp = current;
    int i, n, v;

    /* From this point, the old code and data segments are not needed anymore */
    for (i = 0; i < MAX_SEGS; i++) {
        if (currentp->mm[i])
            seg_put(currentp->mm[i]);
    }

#ifdef CONFIG_EXEC_OS2
    if (multisegment) {
        for (i = 0; i < MAX_SEGS; i++) {
            currentp->mm[i] = mm_table[i];
            mm_table[i] = 0;
        }
    } else
#endif
    {
        currentp->mm[SEG_CODE] = seg_code;
        currentp->mm[SEG_DATA] = seg_data;
    }

    currentp->t_xregs.cs = seg_code->base;
    currentp->t_regs.ss = currentp->t_regs.es = currentp->t_regs.ds = seg_data->base;
    currentp->t_regs.sp = currentp->t_begstack;

    /* An even start break address speeds up libc malloc/sbrk */
    currentp->t_endbrk =  (currentp->t_enddata + 1) & ~1;

    /* argv and envp are two NULL-terminated arrays of pointers, located
     * right after argc.  This fixes them up so that the loaded program
     * gets the right strings. */

    i = n = 0;          /* Start by skipping argc */
    do {
        i += sizeof(__u16);
        if ((v = get_ustack(currentp, i)) != 0)
            put_ustack(currentp, i, (currentp->t_begstack + v));
        else n++;       /* increments for each array traversed */
    } while (n < 2);

    /* Clear signal handlers */
    i = 0;
    do {
        currentp->sig.action[i].sa_dispose = SIGDISP_DFL;
    } while (++i < NSIG);
    currentp->sig.handler = (__kern_sighandler_t)NULL;

    /* Close required files */
    i = 0;
    do {
        if (FD_ISSET(i, &currentp->files.close_on_exec))
            sys_close(i);
    } while (++i < NR_OPEN);

    iput(currentp->t_inode);
    currentp->t_inode = inode;

    /* this could be a good place to set the effective user identifier
     * in case the suid bit of the executable had been set */

    /* can I trust the following fields?  */
    if (inode->i_mode & S_ISUID)
        currentp->euid = inode->i_uid;
    if (inode->i_mode & S_ISGID)
        currentp->egid = inode->i_gid;

#if UNUSED      /* used only for vfork()*/
    wake_up(&currentp->p_parent->child_wait);
#endif

    /*
     * Arrange for our return from sys_execve onto the new
     * user stack and to CS:entry of the user process.
     */
    arch_setup_user_stack(currentp, entry);
}

#ifdef CONFIG_EXEC_OS2
static int execve_os2(struct inode *inode, struct file *filp, char *sptr, size_t slen)
{
    int retval;
    unsigned int n, seg;
    struct ne_segment *segp;
    segext_t paras, auto_paras;
    seg_t ds = current->t_regs.ds;
    segment_s *seg_code, *seg_data;
    //FIXME statics below need mutex against exec reentry, too large to be stack vars
    static struct dos_exec_hdr doshdr;
    static struct ne_exec_hdr os2hdr;
    static struct ne_segment ne_segment_table[MAX_SEGS];
    static struct ne_reloc_num reloc_num;
    static struct ne_reloc reloc;

    /* read MZ header, then OS/2 header */
    current->t_regs.ds = kernel_ds;
    filp->f_pos = 0;
    retval = filp->f_op->read(inode, filp, (char *)&doshdr, sizeof(doshdr));
    if (retval != sizeof(doshdr)) goto errout;

    filp->f_pos = doshdr.ext_hdr_offset;
    retval = filp->f_op->read(inode, filp, (char *)&os2hdr, sizeof(os2hdr));
    if (retval != sizeof(os2hdr)) goto errout;
    if (os2hdr.magic != NEMAGIC || os2hdr.target_os != NETARGET_OS2 ||
            (os2hdr.program_flags & 0x1F) != NEPRG_FLG_MULTIPLEDATA ||
            !os2hdr.auto_data_segment || os2hdr.auto_data_segment > os2hdr.num_segments ||
            os2hdr.num_modules || os2hdr.num_movable_entries ||
            os2hdr.num_resource_entries) {
        debug_os2("EXEC: Unsupported OS/2 format\n");
        goto errout;
    }

    debug_os2("Segments: %d\n", os2hdr.num_segments);
    debug_os2("Auto data segment: %d\n", os2hdr.auto_data_segment);
    debug_os2("Heap: %u\n", os2hdr.heap_size);
    debug_os2("Stack: %u\n", os2hdr.stack_size);
    if (!os2hdr.heap_size)
        os2hdr.heap_size = INIT_HEAP;
    if (!os2hdr.stack_size)
        os2hdr.stack_size = INIT_STACK;

    if (os2hdr.num_segments > MAX_SEGS) {
        printk("Too many segments: %d (configured for %d)\n",
            os2hdr.num_segments, MAX_SEGS);
        retval = -E2BIG;
        goto errout;
    }

    /* read segment table and allocate memory for segments */
    filp->f_pos = os2hdr.segment_table_offset + doshdr.ext_hdr_offset;
    n = os2hdr.num_segments * sizeof(struct ne_segment);
    retval = filp->f_op->read(inode, filp, (char *)ne_segment_table, n);
    if (retval != n) goto errout;

    auto_paras = 0;
    seg_code = seg_data = 0;
    for(seg = 0; seg < os2hdr.num_segments; seg++) {
        segp = &ne_segment_table[seg];
        paras = ((segp->min_alloc + 15) & ~15) >> 4;
        if (!paras || !segp->size) {
            printk("64K segments not supported\n");
            goto errout3;
        }
        if (seg+1 == os2hdr.auto_data_segment) {    /* main data segment w/stack & heap */
            if ((n = os2hdr.heap_size) == 0xffff)   /* calc max heap */
                n = 1;
            paras += (os2hdr.stack_size + slen + n + 15) >> 4;
            if (paras >= 0x1000) {
                retval = -E2BIG;
                goto errout2;
            }
            if (os2hdr.heap_size == 0xffff) {
                debug_os2("Auto max heap: %u\n", (0x0FFF - paras) << 4);
                paras = 0x0FFF;
            }
            auto_paras = paras;
        }
        debug_os2("Segment %d: offset %04x size %5u/%6lu flags %04x minalloc %u\n",
            seg+1, segp->offset << os2hdr.file_alignment_shift_count, segp->size,
            (unsigned long)paras<<4, segp->flags, segp->min_alloc);

        if (!(mm_table[seg] = seg_alloc(paras, (segp->flags & NESEG_DATA)
                ? ((seg+1 == os2hdr.auto_data_segment)? SEG_FLAG_DSEG: SEG_FLAG_DDAT)
                : SEG_FLAG_CSEG))) {
            retval = -ENOMEM;
            goto errout2;
        }
        if (seg+1 == os2hdr.reg_cs)             /* save entry code segment */
            seg_code = mm_table[seg];
        if (seg+1 == os2hdr.auto_data_segment)  /* save auto data segment */
            seg_data = mm_table[seg];

        /* clear bss */
        if (segp->min_alloc > segp->size) {
            n = segp->min_alloc - segp->size;
            debug_os2("Clear bss at %04x:%04x for %u bytes\n",
                mm_table[seg]->base, segp->size, n);
            fmemsetb((char *)segp->size, mm_table[seg]->base, 0, n);
        }
    }
    if (!seg_code || !seg_data) {
        printk("Missing entry point or auto data segment\n");
        goto errout3;
    }

    /* read in segments and perform relocations */
    for(seg = 0; seg < os2hdr.num_segments; seg++) {
        segp = &ne_segment_table[seg];
        filp->f_pos = (unsigned long)segp->offset << os2hdr.file_alignment_shift_count;
        current->t_regs.ds = mm_table[seg]->base;
        debug_os2("Reading seg %d %04x:0000 %u bytes offset %04lx\n", seg+1,
            current->t_regs.ds, segp->size, filp->f_pos);
        retval = filp->f_op->read(inode, filp, 0, segp->size);
        if (retval != segp->size) goto errout2;

        current->t_regs.ds = kernel_ds;
        if (segp->flags & NESEG_RELOCINFO) {
            retval = filp->f_op->read(inode, filp, (char *)&reloc_num, sizeof(reloc_num));
            if (retval != sizeof(reloc_num)) goto errout2;
            debug_reloc2("%d relocation records\n", reloc_num.num_records);

            for (n = 0; n < reloc_num.num_records; n++) {
                retval = filp->f_op->read(inode, filp, (char *)&reloc, sizeof(reloc));
                if (retval != sizeof(reloc)) goto errout2;
                debug_reloc2("Reloc %d: type %d flags %d, "
                    "source chain %04x (segment %d offset %04x)\n",
                    n+1, reloc.src_type, reloc.flags, reloc.src_chain, reloc.segment,
                    reloc.offset);

                if ((reloc.flags & NEFIXFLG_TARGET_MASK) != NEFIXFLG_INTERNALREF ||
                    (reloc.src_type != NEFIXSRC_SEGMENT &&
                     reloc.src_type != NEFIXSRC_FARADDR)) {
                    unsupported:
                        printk("Unsupported fixup type %d flags %d\n", reloc.src_type,
                            reloc.flags);
                        goto errout2;
                }
                switch (reloc.src_type) {
                case NEFIXSRC_SEGMENT:
                    if (reloc.flags & NEFIXFLG_ADDITIVE) goto unsupported;
                    debug_reloc2("pokew %04x:%04x <- seg %d %04x\n",
                        mm_table[seg]->base, reloc.src_chain,
                        reloc.segment, mm_table[reloc.segment-1]->base);
                    pokew(reloc.src_chain, mm_table[seg]->base,
                        mm_table[reloc.segment-1]->base);
                    break;
                case NEFIXSRC_FARADDR:
                    if (reloc.flags & NEFIXFLG_ADDITIVE) {
                        debug_reloc2("pokew %04x:%04x += offset %04x\n",
                            mm_table[seg]->base, reloc.src_chain, reloc.offset);
                        word_t prev = peekw(reloc.src_chain, mm_table[seg]->base);
                        pokew(reloc.src_chain, mm_table[seg]->base, prev+reloc.offset);
                    } else {
                        debug_reloc2("pokew %04x:%04x = offset %04x\n",
                            mm_table[seg]->base, reloc.src_chain, reloc.offset);
                        pokew(reloc.src_chain, mm_table[seg]->base, reloc.offset);
                    }
                    debug_reloc2("pokew %04x:%04x <- seg %d %04x\n",
                        mm_table[seg]->base, reloc.src_chain+2,
                        reloc.segment, mm_table[reloc.segment-1]->base);
                    pokew(reloc.src_chain+2, mm_table[seg]->base,
                        mm_table[reloc.segment-1]->base);
                    break;
                }
            }
        }
    }
    /* From this point, exec() will surely succeed */

    /* set data/stack limits and copy argc/argv */
    current->t_enddata = ne_segment_table[os2hdr.auto_data_segment-1].min_alloc;
    current->t_endseg = auto_paras << 4;;
    current->t_begstack = (current->t_endseg - slen) & ~1;
    current->t_regs.dx = current->t_minstack = os2hdr.stack_size;

    debug_os2("t_endseg %04x, t_begstack %04x\n", current->t_endseg, current->t_begstack);
    debug_os2("fmemcpy %04x:%04x <- %d\n", seg_data->base, current->t_begstack, slen);
    fmemcpyb((char *)current->t_begstack, seg_data->base, sptr, ds, slen);

    finalize_exec(inode, seg_code, seg_data, os2hdr.reg_ip, 1);
    return 0;       /* success */

errout3:
    retval = -EINVAL;
errout2:
    for (seg = 0; seg < MAX_SEGS; seg++) {
        if (mm_table[seg])
            seg_put(mm_table[seg]);
        mm_table[seg] = 0;
    }
errout:
    current->t_regs.ds = ds;
    if (retval >= 0)
        retval = -EINVAL;
    return retval;
}
#endif
