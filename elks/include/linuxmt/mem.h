#ifndef __LINUXMT_MEM_H
#define __LINUXMT_MEM_H

#define DATASIZE 2048
#define TEXTSIZE 2048

#define MOD_INIT	0
#define MOD_TERM	1

#define MEM_GETMODTEXT	0
#define MEM_GETMODDATA	1
#define MEM_GETTEXTSIZ	2
#define MEM_GETUSAGE	3
#define MEM_GETTASK	4
#define MEM_GETDS	5
#define MEM_GETCS	6
#define MEM_GETHEAP	7

struct mem_usage {
	unsigned int free_memory;
	unsigned int used_memory;
};

#ifdef CONFIG_MODULES

extern char module_data[];
extern int module_init(void);

#endif

#endif
