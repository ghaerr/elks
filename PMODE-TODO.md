# 286 protected-mode port — backlog

Status: full multi-user works on QEMU/KVM — boot → directfd DMA → FAT root →
init → /etc/rc.sys → getty → login → shell; meminfo/ps/uptime/uname/df/date
work. Proven on both the full (far-text) config and a trimmed 55K
single-segment (far-text-off) kernel. All userland currently runs at ring 0.

## Bugs / broken

- ~~Shell pipes wedge the shell~~ FIXED upstream (PR #2719, merged): not
  pipes at all — serfast.S read UART_RX before checking LSR_DR, so QEMU's
  spurious serial IRQs queued a NUL into the tty input, and ash silently
  discarded the poisoned next command line. Entry guard added, matching the
  C handler. Confirmed on stock real-mode ELKS (not a PM-port artifact);
  the fix now arrives via upstream master.
- ~~telnetd faults EXC=0005 at startup~~ RESOLVED: another fragmentation-era
  phantom — the pty driver needed no PM fix at all. With CONFIG_PSEUDO_TTY
  enabled, **telnet login into 286 PM works end-to-end** (client → QEMU
  hostfwd → ne2k → ktcp → telnetd → pty → login → shell) on the small
  single-segment kernel (text 60624). The original roadmap goal is complete.

## Degraded / deferred

- **ps COMMAND column: blank or kernel-string garbage.** Cmdline reads pack
  another process's SS as (seg<<4)+off — not invertible with selectors. The
  kmem_read bounds check (16ad9d65) rejects positions outside the
  (KERNEL_DS<<4)+64K window, but with KERNEL_DS=0x10 the window starts at
  linear 0x100, so many selector-packed positions land INSIDE it and read
  valid-but-wrong kernel data — pty-session processes' SPs collide, so ps
  prints kernel rodata strings as COMMAND (cosmetic; system unaffected).
  Real fix: selector-aware /dev/kmem protocol (e.g. ioctl taking selector +
  offset) + rebuilt ps — blocked on userland rebuilds.
- ~~Userland cannot be rebuilt~~ FIXED: `make -C libc all && make -C libc
  install DESTDIR=/usr` installs elkslibc into the PPA toolchain
  (/usr/ia16-elf/lib/elkslibc) and provides the runtime specs that
  `-melks-libc` needs; apps then build normally (verified: ps links and
  runs). Unblocks: /proc-based ps rewrite, test programs, kmem protocol
  work.
- **procfs (CONFIG_PROC_FS) prototype landed** (41725883): /proc/uptime +
  /proc/pid/{stat,cmd}, automounted, ~2.5K text, correct command names in
  PM via kernel-side selector reads. Next: rewrite ps to read /proc
  (kills the kmem COMMAND garbage + the struct-version coupling), then
  the upstream design issue (user is raising it).
- **Medium-model user executables** (FTEXT != 0): need CONFIG_EXEC_MMODEL +
  selector-aware relocate() + a per-process second code descriptor. No known
  binary on the image needs it; telnet path is all small-model.
- **/bootopts not found** on hand-built images unless the file sits in the
  first 8 root directory entries (setup.S scans only those). mkimg.sh omits
  /bootopts; defaults work. Robust fix: read /bootopts post-root-mount.

## Design debt

- **Ring 3 / protection.** Everything runs at DPL0, so PM currently adds no
  memory protection. Needs DESC_U* descriptors, a TSS for the ring3→ring0
  stack switch, and _irqit/restore_regs rework for the hardware ring switch.
- **Extended memory (>1MB).** XMS group is disabled; kernel uses only the
  639K conventional pool (it is tight: kstack bump once OOM'd buffer_init).
  On 286, >1MB access in PM is natural via descriptors — a PM-native buffer
  cache above 1MB would relieve the main pool.
- **#ifdef reduction / upstream convergence** (ghaerr, issue #2718): replace
  raw CONFIG_286_PMODE ifdefs with mainline-friendly abstractions in the
  KERNEL_DS/BIOSSEG/VIDEOSEG style. Done so far: KERNEL_CS/KERNEL_DS in
  mem.c. Audit remaining ifdef sites.
- **.farinit.init reclaim in small model:** INITPROC is empty with far text
  off, so boot-only code stays resident (~7K). Could add a dedicated near
  .text.init section + linker brackets + seg_add, same as far text does.

## Tooling / process

- **mkimg.sh lives only inside the `elks` container** (/work/mkimg.sh):
  fresh-image build + hard fail if /linux is fragmented. Consider committing
  it to the repo and re-deploying on container rebuild.
  Never quick-update ::/linux in a populated image — the FAT boot sector
  loads consecutive sectors from the first cluster only; a fragmented kernel
  boots with a silently corrupted .data tail (cost a full day to diagnose).
- **Testing breadth:** only QEMU/KVM so far. pcem/86Box (real 286 timing) and
  real hardware untested. ghaerr tests with pcem; slow boot reported.

## Someday / vision

- sbase userland + C99 compiler work on a 286 Unix (the original motivation —
  8086 segmentation memory limits make this impossible without PM).
- Upstream-visible fork maintenance: keep rebasing on mainline ELKS, publish
  images, find users (per the issue #2718 discussion).
