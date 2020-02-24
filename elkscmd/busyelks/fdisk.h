#if !defined(FDISK_H)
#define	FDISK_H

#define MODE_EDIT 0
#define MODE_LIST 1
#define MODE_SIZE 2

#define CMDLEN 8

#define DEFAULT_DEV "/dev/hda"

typedef struct {
  int cmd;
  char *help;
  void (*func)();
} Funcs;

#endif
