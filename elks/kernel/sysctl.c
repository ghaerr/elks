#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/sysctl.h>

static char ctlname[CTL_MAXNAMESZ];

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

int sys_sysctl(int op, char *name, int *value)
{
    int error, n;
    struct sysctl *sc;

    if (op >= CTL_LIST) {
        if (!name)
            return -EFAULT;
        if (op >= ARRAYLEN(sysctl))
            return -ENOTDIR;
        sc = &sysctl[op];
        return verified_memcpy_tofs(name, (void *)sc->name, strlen(sc->name)+1);
    }

    n = strlen_fromfs(name, CTL_MAXNAMESZ) + 1;
    if (n >= CTL_MAXNAMESZ) return -EFAULT;
    error = verified_memcpy_fromfs(ctlname, name, n);
    if (error) return error;
    error = verfy_area(value, sizeof(int));
    if (error) return error;

    for (sc = sysctl;; sc++) {
        if (sc >= &sysctl[ARRAYLEN(sysctl)]) return -ENOTDIR;
        if (!strcmp(sc->name, ctlname))
            break;
    }

    if (op == CTL_GET)
            put_user(*sc->value, value);
    else if (op == CTL_SET)
            *sc->value = get_user(value);
    else
        return -EINVAL;
    return 0;
}
