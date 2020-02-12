#if !defined(CMD_H)
#define	CMD_H

#if defined(__cplusplus)
extern "C" {
#endif

int	basename_main(int argc, char * argv[]);
int cal_main(int argc, char * argv[]);
int cat_main(int argc, char **argv);
int chgrp_main(int argc, char * argv[]);
int chmod_main(int argc, char * argv[]);
int chown_main(int argc, char * argv[]);
int cksum_main(int argc, char * argv[]);
int cmp_main(int argc, char * argv[]);
int cp_main(int argc, char * argv[]);
int cut_main(int argc, char * argv[]);
int date_main(int argc, char * argv[]);
int dd_main(int argc, char * argv[]);
int diff_main(int argc, char * argv[]);
int	dirname_main(int argc, char * argv[]);
int du_main(int argc, char * argv[]);
int echo_main(int argc, char * argv[]);
int ed_main(int argc, char * argv[]);

#if defined(__cplusplus)
}
#endif

#endif
