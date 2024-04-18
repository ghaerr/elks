#ifndef __LINUXMT_MEM_H
#define __LINUXMT_MEM_H

#define MEM_GETTEXTSIZ	2
#define MEM_GETUSAGE	3
#define MEM_GETTASK	4
#define MEM_GETDS	5
#define MEM_GETCS	6
#define MEM_GETHEAP	7
#define MEM_GETUPTIME	8
#define MEM_GETFARTEXT  9
#define MEM_GETMAXTASKS 10

struct mem_usage {
	unsigned int free_memory;
	unsigned int used_memory;
};

#endif
