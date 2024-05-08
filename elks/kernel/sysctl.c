#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>

#define MAXCTLNAME      32
static char ctlname[MAXCTLNAME];

struct sysctl {
    const char *name;
    int *value;
};

int debug_malloc = 1;
int debug_tcp = 200;

struct sysctl sysctl[] = {
    { "debug.malloc",       &debug_malloc,  },
    { "debug.tcp",          &debug_tcp,     },
};

#define ARRAYLEN(a)     (sizeof(a)/sizeof(a[0]))

int sys_sysctl(int *name, int *oldval, int *newval)
{
    int error, n;
    struct sysctl *sc;

    n = strlen_fromfs(name, MAXCTLNAME) + 1;
    if (n >= MAXCTLNAME) return -EFAULT;
    error = verified_memcpy_fromfs(ctlname, name, n);
    if (error) return error;

    for (sc = sysctl;; sc++) {
        if (sc >= &sysctl[ARRAYLEN(sysctl)]) return -ENOTDIR;
        if (!strcmp(sc->name, ctlname))
            break;
    }

    if (oldval) {
            error = verify_area(VERIFY_WRITE, oldval, sizeof(int));
            if (error) return error;
            put_user(*sc->value, oldval);
    }
    if (newval) {
            error = verify_area(VERIFY_READ, newval, sizeof(int));
            if (error) return error;
            *sc->value = get_user(newval);
    }
    return 0;
}
