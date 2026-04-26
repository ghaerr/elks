#!/bin/sh
set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ACTION=${1:-build}
if [ $# -gt 0 ]; then
    shift
fi

PROGS="
01_verify_area_wrap_read
02_lseek_bad_ptr
03_utime_bad_times
04_ioctl_fionbio_bad_arg
05_tiosetconsole_bad_arg
06_kmem_getusage_bad_arg
07_sysctl_bad_name
08_symlink_bad_oldname
09_chdir_bad_path
10_connect_unix_short_len
11_connect_inet_short_len
12_bind_unix_full_len
"

usage() {
    cat <<USAGE
Usage:
  $(basename "$0") [build|stage|run|clean|list|help] [args]

Modes:
  build            Cross-build all regression tests for ELKS (default)
  stage DIR        Build and copy binaries plus README into DIR
  run              Run already-built tests in the current ELKS userspace
  clean            Remove built artifacts
  list             Print test program names
  help             Show this help

Environment:
  ELKS_SRC         Path to ELKS kernel source tree (auto-detected if possible)
  LIBC_SRC         Path to ELKS libc source tree (auto-detected if possible)
  CC               Cross-compiler (default: ia16-elf-gcc)
  STRIP            Binary stripper (default: ia16-elf-strip if available)
  EXTRA_CFLAGS     Extra compiler flags
  EXTRA_LDFLAGS    Extra linker flags
  DESTDIR          Optional alias for stage destination

Notes:
  - build/stage use the ELKS source trees for header selection, but rely on an
    ELKS-capable ia16 toolchain and libc being installed in the compiler sysroot.
  - run is intended to execute inside ELKS after the binaries have been copied
    into a test filesystem or image.
USAGE
}

autodetect_tree() {
    name=$1
    envvar=$2
    eval current='${'$envvar':-}'
    if [ -n "$current" ] && [ -d "$current" ]; then
        printf '%s\n' "$current"
        return 0
    fi
    for d in \
        "$SCRIPT_DIR/../$name" \
        "$SCRIPT_DIR/../../$name" \
        "$PWD/$name" \
        "$PWD/../$name"
    do
        if [ -d "$d" ]; then
            printf '%s\n' "$d"
            return 0
        fi
    done
    return 1
}

is_elks_runtime() {
    if [ -f /bin/elks ]; then
        return 0
    fi
    if uname -s 2>/dev/null | grep -qi '^elks$'; then
        return 0
    fi
    return 1
}

build_tests() {
    CC=${CC:-ia16-elf-gcc}
    STRIP=${STRIP:-ia16-elf-strip}

    ELKS_SRC=$(autodetect_tree elks ELKS_SRC || true)
    LIBC_SRC=$(autodetect_tree libc LIBC_SRC || true)

    if [ -z "${ELKS_SRC:-}" ] || [ -z "${LIBC_SRC:-}" ]; then
        echo "error: could not auto-detect ELKS_SRC and LIBC_SRC" >&2
        echo "set ELKS_SRC=/path/to/elks and LIBC_SRC=/path/to/libc" >&2
        exit 2
    fi

    if ! command -v "$CC" >/dev/null 2>&1; then
        echo "error: compiler '$CC' not found" >&2
        exit 2
    fi

    BASE_CFLAGS='-ffreestanding -fno-inline -melks -mtune=i8086 -mcmodel=small -mno-segment-relocation-stuff -Wall -Wextra -Wtype-limits -Wno-unused-parameter -Wno-sign-compare -Os'
    INCLUDES="-I$SCRIPT_DIR -I$LIBC_SRC/include -I$ELKS_SRC/include"
    CFLAGS="${CFLAGS:-} $BASE_CFLAGS $INCLUDES ${EXTRA_CFLAGS:-}"
    LDFLAGS="${LDFLAGS:-} ${EXTRA_LDFLAGS:-}"

    echo "== building ELKS regression tests =="
    echo "   ELKS_SRC=$ELKS_SRC"
    echo "   LIBC_SRC=$LIBC_SRC"
    echo "   CC=$CC"
    make -C "$SCRIPT_DIR" clean >/dev/null
    make -C "$SCRIPT_DIR" CC="$CC" CFLAGS="$CFLAGS" LDFLAGS="$LDFLAGS"

    if command -v "$STRIP" >/dev/null 2>&1; then
        for t in $PROGS; do
            [ -f "$SCRIPT_DIR/$t" ] && "$STRIP" "$SCRIPT_DIR/$t" || true
        done
    fi
}

stage_tests() {
    dest=${1:-${DESTDIR:-}}
    if [ -z "$dest" ]; then
        echo "error: stage requires a destination directory" >&2
        exit 2
    fi
    build_tests
    mkdir -p "$dest"
    for t in $PROGS; do
        cp "$SCRIPT_DIR/$t" "$dest/"
    done
    cp "$SCRIPT_DIR/README.txt" "$dest/"
    echo "staged test binaries in $dest"
}

run_tests() {
    if ! is_elks_runtime; then
        echo "error: run mode is intended to execute inside ELKS" >&2
        echo "build on the host, copy the binaries into the ELKS filesystem, then re-run there" >&2
        exit 2
    fi
    missing=0
    for t in $PROGS; do
        if [ ! -x "$SCRIPT_DIR/$t" ]; then
            echo "missing binary: $SCRIPT_DIR/$t" >&2
            missing=1
        fi
    done
    if [ "$missing" -ne 0 ]; then
        echo "error: build or copy the binaries first" >&2
        exit 2
    fi

    rc=0
    for t in $PROGS; do
        echo "== $t =="
        "$SCRIPT_DIR/$t" || rc=$?
        echo
    done
    exit "$rc"
}

case "$ACTION" in
    build)
        build_tests
        ;;
    stage)
        stage_tests "$@"
        ;;
    run)
        run_tests
        ;;
    clean)
        make -C "$SCRIPT_DIR" clean
        ;;
    list)
        for t in $PROGS; do
            echo "$t"
        done
        ;;
    help|-h|--help)
        usage
        ;;
    *)
        echo "error: unknown mode '$ACTION'" >&2
        usage >&2
        exit 2
        ;;
esac
