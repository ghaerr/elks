#include <linuxmt/mm.h>
#include <linuxmt/errno.h>
#include <linuxmt/string.h>
#include <linuxmt/sysctl.h>

#include <linuxmt/trace.h>
#include <linuxmt/kernel.h>

struct sysctl {
    const char *name;
    int *value;
};

static int malloc_debug;
static int net_debug;

struct sysctl sysctl[] = {
    { "kern.debug",         &dprintk_on         },  /* debug (^P) on/off */
    { "kern.strace",        &tracing            },  /* strace=1, kstack=2 */
    { "kern.console",       (int *)&dev_console },  /* console */
    { "malloc.debug",       &malloc_debug       },
    { "net.debug",          &net_debug          },
};

static char ctlname[CTL_MAXNAMESZ];

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
