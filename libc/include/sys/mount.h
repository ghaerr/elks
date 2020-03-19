/* sys/mount.h*/

int mount(const char *dev, const char *dir, const char *type, int flags);
int umount(const char *dir);

int reboot(int magic1, int magic2, int magic3);
