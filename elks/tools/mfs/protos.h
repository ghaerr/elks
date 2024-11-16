#if DEBUG
#define debug(...)      printf(__VA_ARGS)
#else
#define debug(...)
#endif
void pver();
void usage(const char *name);
void parse_opts(int *argc_p, char ***argv_p);
void do_cmd(struct minix_fs_dat *fs, int argc, char **argv);
int main(int argc, char **argv);
unsigned long get_free_bit(u8 *bmap, int bsize);
struct minix_fs_dat *new_fs(const char *fn, int magic, unsigned long fsize, int inodes);
struct minix_fs_dat *open_fs(const char *fn, int chk);
void cmd_sysinfo(struct minix_fs_dat *fs);
struct minix_fs_dat *close_fs(struct minix_fs_dat *fs);
void fatalmsg(const char *s, ...);
void die(const char *s, ...);
FILE *goto_blk(FILE *fp, unsigned int blk);
void *dofwrite(FILE *fp, void *buff, int cnt);
void *dofread(FILE *fp, void *buff, int cnt);
void *domalloc(unsigned long size, int elm);
int dogetuid();
int dogetgid();
u32 ino2_zone(struct minix_fs_dat *fs, struct minix2_inode *ino, u32 blk);
int ino_zone(struct minix_fs_dat *fs, struct minix_inode *ino, int blk);
void ino2_setzone(struct minix_fs_dat *fs, struct minix2_inode *ino, u32 blk, u32 zone);
void ino_setzone(struct minix_fs_dat *fs, struct minix_inode *ino, int blk, int zone);
void ino2_freezone(struct minix_fs_dat *fs, struct minix2_inode *ino, u32 blk);
void ino_freezone(struct minix_fs_dat *fs, struct minix_inode *ino, int blk);
int read_inoblk(struct minix_fs_dat *fs, int inode, u32 blk, u8 *buf);
void write_inoblk(struct minix_fs_dat *fs, int inode, u32 blk, u8 *buf);
void free_inoblk(struct minix_fs_dat *fs, int inode, u32 blk);
void trunc_inode(struct minix_fs_dat *fs, int inode, u32 sz);
int find_inode(struct minix_fs_dat *fs, const char *path);
void set_inode(struct minix_fs_dat *fs, int inode, int mode, int nlinks, int uid, int gid, u32 size, u32 atime, u32 mtime, u32 ctime, int clr);
void clr_inode(struct minix_fs_dat *fs, int inode);
void parse_mkfs(int argc, char **argv, int *magic_p, int *nblks_p, int *inodes_p);
void cmd_mkfs(char *filename, int argc, char **argv);
void cmd_genfs(char *filename, int argc, char **argv);
void cmd_addfs(char *filename, int argc, char **argv);
void cmd_boot(char *filename, int argc,char **argv);
void outent(const unsigned char *dp, int namlen);
void printsymlink(struct minix_fs_dat *fs, int inode);
void dols(struct minix_fs_dat *fs, const char *path,int flags);
void cmd_ls(struct minix_fs_dat *fs, int argc, char **argv);
int domkdir(struct minix_fs_dat *fs, char *newdir, int mode);
void cmd_mkdir(struct minix_fs_dat *fs, int argc, char **argv, int mode);
void timefmt(const char *str, u32 usecs);
void outmode(int mode);
int dostat_inode(struct minix_fs_dat *fs, int inode);
void dormdir(struct minix_fs_dat *fs, const char *dir);
void cmd_rmdir(struct minix_fs_dat *fs, int argc, char **argv);
void dounlink(struct minix_fs_dat *fs, char *fpath);
void cmd_rm(struct minix_fs_dat *fs, int argc, char **argv);
int readfile(struct minix_fs_dat *fs, FILE *fp, const char *path, int type, int ispipe);
void cmd_cat(struct minix_fs_dat *fs, int argc, char **argv);
void cmd_extract(struct minix_fs_dat *fs, int argc, char **argv);
void cmd_readlink(struct minix_fs_dat *fs, int argc, char **argv);
void writefile(struct minix_fs_dat *fs, FILE *fp, int inode);
void writedata(struct minix_fs_dat *fs, u8 *blk, u32 cnt, int inode);
void dosymlink(struct minix_fs_dat *fs, char *source, char *lnkpath);
void dohardlink(struct minix_fs_dat *fs, char *source, char *lnkpath);
void cmd_ln(struct minix_fs_dat *fs, int argc, char **argv);
void cmd_mknode(struct minix_fs_dat *fs, int argc, char **argv);
void cmd_cp(struct minix_fs_dat *fs, int argc, char **argv);
int cmp_name(const char *lname, const char *dname, int nlen);
int ilookup_name(struct minix_fs_dat *fs, int inode, const char *lname, u32 *blkp, int *offp);
void dname_add(struct minix_fs_dat *fs, int dinode, const char *name, int inode);
void dname_rem(struct minix_fs_dat *fs, int dinode, const char *name);
int make_node(struct minix_fs_dat *fs, char *fpath, int mode, int uid, int gid, u32 size, u32 atime, u32 mtime, u32 ctime, int *dinode_p);

extern int opt_verbose;
extern int opt_keepuid;
extern int opt_fsbad_fatal;
extern char *toolname;

extern int opt_nocopyzero;
extern int opt_appendifexists;

extern char *optarg;
extern int opterr;
extern int optind;
int getoptX(int argc, char *const *argv, const char *shortopts);
