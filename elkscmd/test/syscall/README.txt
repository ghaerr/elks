ELKS user-access regression tests
=================================

These tests target the confirmed Step 1 user/kernel memory-boundary bugs.
Run them one by one on an instrumented or disposable kernel: several of the
buggy-kernel behaviors involve out-of-range reads/writes and may crash,
hang, or corrupt user/kernel memory.

Expected result on a fixed kernel
---------------------------------
Each test should print PASS and exit 0.
SKIP means the environment does not expose the needed device or protocol.

Important
---------
12_bind_unix_full_len.c matches the minimal AF_UNIX hardening policy from the
patch-oriented report: reject a full-size sockaddr_un that leaves no room for
an added terminator. If you implement a different but still-correct fix, update
that test's expected result while keeping the core invariant: no kernel memory
overwrite and no unsafe handling of unterminated sun_path.


Host-side helper
----------------
`run-regress.sh` is a small wrapper for the extracted regression tree.

Typical flow:
1. On the host: `./run-regress.sh build`
2. Copy the resulting binaries into an ELKS test filesystem, or use
   `./run-regress.sh stage /path/to/staging/dir`
3. Inside ELKS: run each test, or invoke the script again as
   `./run-regress.sh run` if the script is present there too.

If auto-detection fails, set:
- `ELKS_SRC=/path/to/elks`
- `LIBC_SRC=/path/to/libc`

The script uses the ELKS source trees for headers and expects an
ELKS-capable `ia16-elf-gcc` toolchain with libc installed in its sysroot.
