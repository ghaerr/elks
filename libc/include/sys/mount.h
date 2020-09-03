/* sys/mount.h*/

#define MS_RDONLY	    1	/* mount read-only */
#define MS_NOSUID	    2	/* ignore suid and sgid bits */
#define MS_NODEV	    4	/* disallow access to device special files */
#define MS_NOEXEC	    8	/* disallow program execution */
#define MS_SYNCHRONOUS	   16	/* writes are synced at once */
#define MS_REMOUNT	   32	/* alter flags of a mounted FS */

int mount(const char *dev, const char *dir, const char *type, int flags);
int umount(const char *dir);

int reboot(int magic1, int magic2, int magic3);
