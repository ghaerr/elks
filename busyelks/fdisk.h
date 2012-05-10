#define MODE_EDIT 0
#define MODE_LIST 1
#define MODE_SIZE 2

#define CMDLEN 8

#define DEFAULT_DEV "/dev/hda"

void quit();
void list_part();
void del_part();
void add_part();
void help();
void write_out();
void list_types();
void list_part();
void set_boot();
void set_type();

void list_partition(char *dev);

char dev[256]; /* FIXME - should be a #define'd number from header file */
int pFd;
unsigned char partitiontable[512];

typedef struct {
  int cmd;
  char *help;
  void (*func)();
} Funcs;

struct hd_geometry geometry;

