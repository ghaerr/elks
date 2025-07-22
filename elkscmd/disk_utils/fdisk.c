/*
 *  fdisk.c  Disk partitioning program.
 *  Copyright (C) 1997  David Murn
 *      Updated by Greg Haerr March 2020
 *
 *  This program is distributed under the GNU General Public Licence, and
 *  as such, may be freely distributed.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef _M_I86
#include <arch/hdreg.h>
#endif

struct partition
{
    unsigned char boot_ind;     /* 0x80 - active */
    unsigned char head;         /* starting head */
    unsigned char sector;       /* starting sector */
    unsigned char cyl;          /* starting cylinder */
    unsigned char sys_ind;      /* What partition type */
    unsigned char end_head;     /* end head */
    unsigned char end_sector;   /* end sector */
    unsigned char end_cyl;      /* end cylinder */
    unsigned short start_sect;  /* starting sector counting from 0 */
    unsigned short start_sect_hi;
    unsigned short nr_sects;    /* nr of sectors in partition */
    unsigned short nr_sects_hi;
};

/*
 * Seperate structure than hd_geometry for host compilation.
 */
struct geometry {
    unsigned char heads;
    unsigned char sectors;
    unsigned short cylinders;
    unsigned long start;
};

#define DEFAULT_DEV         "/dev/hda"
#define PARTITION_MINORS    0x07    /* minor device number range of valid partitions */
#define PARTITION_TYPE      0x80    /* ELKS, Old Minix*/
#define PARTITION_START     0x01be  /* offset of partition table in MBR*/
#define PARTITION_END       0x01fd  /* end of partition 4 in MBR*/

#define MODE_EDIT 0
#define MODE_LIST 1
#define MODE_SIZE 2
#define CMDLEN    8

int pFd;
char dev[80];
unsigned char MBR[512];

typedef struct {
  int cmd;
  char *help;
  void (*func)();
} Funcs;

struct geometry geometry;

void quit();
void list_part();
void del_part();
void add_part();
void help();
void write_out();
void list_types();
void set_boot();
void set_type();
void list_partition(char *dev);

#define sects_per_cyl ((unsigned long)(geometry.heads * geometry.sectors))


/* Available commands and their respective functions */
static Funcs funcs[] = {
    { 'b',"Set bootable flag",     set_boot },
    { 'd',"Delete partition",      del_part },
    { 'l',"List partition types",  list_types },
    { 'n',"Create new partion",    add_part },
    { 'p',"Print partition table", list_part },
    { 'q',"Quit fdisk",            quit },
    { 't',"Set partition type",    set_type },
    { 'w',"Write partition table", write_out },
    { '?',"Help",                  help },
    {  0,  NULL,                   NULL }
};


void list_types(void)   /* FIXME - Should make this more flexible */
{
    printf(
        " 0 Empty                  3c PartitionMagic recovery 85 Linux extended\n"
        " 1 FAT12                  40 Venix 80286             86 NTFS volume set\n"
        " 2 XENIX root             41 PPC PReP Boot           87 NTFS volume set\n"
        " 3 XENIX usr              42 SFS                     93 Amoeba\n"
        " 4 FAT16 <32M             4d QNX4.x                  94 Amoeba BBT\n"
        " 5 Extended               4e QNX4.x 2nd part         a0 IBM Thinkpad hibernate\n"
        " 6 FAT16                  4f QNX4.x 3rd part         a5 BSD/386\n"
        " 7 HPFS/NTFS              50 OnTrack DM              a6 OpenBSD\n"
        " 8 AIX                    51 OnTrack DM6 Aux1        a7 NeXTSTEP\n"
        " 9 AIX bootable           52 CP/M                    b7 BSDI fs\n"
        " a OS/2 Boot Manager      53 OnTrack DM6 Aux3        b8 BSDI swap\n"
        " b Win9x FAT32            54 OnTrack DM6             c1 DR-DOS/sec FAT-12\n"
        " c Win9x FAT32 (LBA)      55 EZ-Drive                c4 DR-DOS/sec FAT-16 <32M\n"
        " e Win9x FAT16 (LBA)      56 Golden Bow              c6 DR-DOS/sec FAT-16\n"
        " f Win9x Extended (LBA)   5c Priam Edisk             c7 Syrinx\n"
        "10 OPUS                   61 SpeedStor               db CP/M / CTOS / ...\n"
        "11 Hide FAT12             63 GNU HURD or SysV        e1 DOS access\n"
        "12 Compaq diagnostics     64 Novell Netware 286      e3 DOS R/O\n"
        "14 Hide FAT16 <32M        65 Novell Netware 386      e4 SpeedStor\n"
        "16 Hide FAT16             70 DiskSecure Multi-Boot   eb BeOS fs\n"
        "17 Hide HPFS/NTFS         75 PC/IX                   f1 SpeedStor\n"
        "18 AST Windows swapfile   80 ELKS / Old Minix        f2 DOS secondary\n"
        "1b Hide Win9x FAT32       81 New Minix / Old Linux   f4 SpeedStor\n"
        "1c Hide Win9x FAT32 (LBA) 82 Linux swap              fd Linux raid autodetect\n"
        "1e Hide Win9x FAT16 (LBA) 83 New Linux               fe LANstep\n"
        "24 NEC DOS                84 OS/2 Hide C:            ff BBT\n"
    );
}

void quit(void)
{
    exit(0);
}

void list_part(void)
{
    if (*dev != 0)
        list_partition(NULL);
}

void add_part(void)
{
    unsigned long start_sect, nr_sects;
    unsigned char *oset;
    int part, scyl, ecyl;
    struct partition p;
    char buf[32];

    p.boot_ind = 0;
    printf("Create new partition:\n");
    for (part = 0; part < 1 || part > 4;) {
        printf("Enter partition number (1-4): ");
        fflush(stdout);
        fgets(buf, 31, stdin);
        part = atoi(buf);
        if (*buf=='\n')
            return;
    }

    oset = MBR + PARTITION_START + ((part - 1) * 16);

    printf("Total cylinders: %d\n", geometry.cylinders);
    for (scyl = geometry.cylinders; scyl < 0 || scyl > geometry.cylinders-1;) {
        printf("First cylinder (%d-%d): ", 0, geometry.cylinders-1);
        fflush(stdout);
        fgets(buf, 31, stdin);
        scyl = atoi(buf);
        if (*buf == '\n')
            return;
    }

#if 1
    /* skip cylinder 0 head 0 sectors, use head 1 sector 1 (standard)*/
    p.head      = (scyl == 0) ? 1 : 0;
    p.sector    = 1 + ((scyl >> 2) & 0xc0);
#else
    /* don't skip any cylinder 0 sectors, use cylinder 0 sector 2*/
    p.head      = 0;
    p.sector    = ((scyl == 0) ? 2: 1) + ((scyl >> 2) & 0xc0);
#endif
    p.cyl       = (scyl & 0xff);
    p.sys_ind   = PARTITION_TYPE;

    for (ecyl = geometry.cylinders; ecyl < scyl || ecyl > geometry.cylinders-1;) {
        printf("Ending cylinder (%d-%d): ", 0, geometry.cylinders-1);
        fflush(stdout);
        fgets(buf, 31, stdin);
        ecyl = atoi(buf);
        if (*buf == '\n')
            return;
    }

    p.end_head  = geometry.heads - 1;
    p.end_sector= geometry.sectors + ((ecyl >> 2) & 0xc0);
    p.end_cyl   = ecyl & 0xff;

    start_sect = sects_per_cyl * (unsigned long)scyl;
    if (scyl == 0)
        start_sect = geometry.sectors;

    p.start_sect        = start_sect & 0xffff;
    p.start_sect_hi     = start_sect >> 16;

    nr_sects = sects_per_cyl * (unsigned long)(ecyl - scyl + 1);
    if (scyl == 0)
        nr_sects -= geometry.sectors;

    p.nr_sects          = nr_sects & 0xffff;
    p.nr_sects_hi       = nr_sects >> 16;

    printf("Adding partition %d\n", part);
    memcpy(oset, &p, 16);
}

void set_boot(void)
{
    int part, i;
    char buf[32];

    printf("Set bootable flag\n");
    for (part = 0; part < 1 || part > 4;) {
        printf("Which partition (1-4): ");
        fflush(stdout);
        fgets(buf, 31, stdin);
        part = atoi(buf);
        if (*buf=='\n')
            return;
    }

    /* clear any boot flags*/
    for (i=0; i<4; i++)
        MBR[PARTITION_START + (i * 16)] = 0x00;

    MBR[PARTITION_START + ((part - 1) * 16)] = 0x80;
}

int atohex(char *s)
{
    int n, r;

    n = 0;
    if (!isxdigit(*s))
        return 256;
    while (*s) {
        if (*s >= '0' && *s <= '9')
            r = *s - '0';
        else if (*s >= 'A' && *s <= 'F')
            r = *s - 'A' + 10;
        else if (*s >= 'a' && *s <= 'f')
            r = *s - 'a' + 10;
        else
            break;
        n = 16 * n + r;
        s++;
    }
    return n;
}

void set_type()  /* FIXME - Should make this more flexible */
{
    int part, type, a;
    char buf[32];

    printf("Set partition type:\n\n");
    for (part=0;part<1 || part>4;) {
        printf("Which partition to toggle(1-4): ");
        fflush(stdout);
        fgets(buf,31,stdin);
        part=atoi(buf);
        if (*buf=='\n')
            return;
    }
    a = PARTITION_START + ((part - 1) * 16);
    type=256;
    while (1) {
        printf("Set partition type (l for list, q to quit): ");
        fflush(stdout);
        fgets(buf,31,stdin);
        if (isxdigit(*buf)) {
            type=atohex(buf) % 256;
            break;
        } else {
            switch (*buf) {

                case 'l':
                    list_types();
                    break;

                case 'q':
                    return;

                default:
                    printf("Invalid: %c\n", *buf);
                    break;
            }
        }
    }
    MBR[a+4] = type;
}

void del_part()
{
    int part;
    char buf[32];

    printf("Delete partition\n");
    for (part = 0; part<1 || part>4;) {
        printf("Which partition (1-4): ");
        fflush(stdout);
        fgets(buf, 31, stdin);
        part = atoi(buf);
        if (*buf == '\n')
            return;
    }
    printf("Deleting partition %d\n",part);
    memset(MBR + PARTITION_START + ((part - 1) * 16), 0, 16);
}

void write_out()
{
    int i;

    if (lseek(pFd, 0L, SEEK_SET) != 0)
        printf("error: cannot seek to offset 0\n");
    else {
        MBR[510] = 0x55;
        MBR[511] = 0xAA;
        i = write(pFd,MBR,512);
        sync();
        if (i != 512) {
            printf("Error writing partition table to %s (%d)\n", errno);
        } else
            printf("Partition table written to %s\n",dev);
    }
}

void help()
{
    Funcs *tmp;

    printf("Key Description\n");
    for (tmp = funcs; tmp->cmd; tmp++)
        printf("%c   %s\n", tmp->cmd, tmp->help);
}

void list_partition(char *devname)
{
    int i, fd = -1;
    int bad_mbr = 0;
    unsigned char table[512];

    if (devname!=NULL) {
        if ((fd=open(devname,O_RDONLY))==-1) {
            printf("Error opening %s\n",devname);
            exit(1);
        }
        if ((i=read(fd,table,512))!=512) {
            printf("Unable to read first 512 bytes from %s, only read %d bytes\n",
                   devname,i);
            exit(1);
        }
    } else
        memcpy(table,MBR,512);
    printf("                           START              END          SECTOR\n");
    printf("Device           #:ID   Cyl Head Sect    Cyl Head Sect  Start   Size\n\n");
    for (i=0; i<4; i++) {
        struct partition *p = (struct partition *)&table[PARTITION_START + (i<<4)];
        unsigned long start_sect = p->start_sect | ((unsigned long)p->start_sect_hi << 16);
        unsigned long nr_sects = p->nr_sects | ((unsigned long)p->nr_sects_hi << 16);
        char device[32];

        strcpy(device, devname? devname: dev);
        if (device[0] == '/') {
                char *p = &device[strlen(device)];
                *p++ = '1' + i;
                *p = 0;
        }
        if (p->boot_ind != 0x00 && p->boot_ind != 0x80)
            bad_mbr++;
        printf("%-15s %c%d:%02x  %4d  %3d%5d   %4d  %3d%5d %6lu %6lu\n",
            device,
            p->boot_ind==0?' ':(p->boot_ind==0x80?'*':'?'),
            i+1,                                         /* #*/
            p->sys_ind,                                  /* Partition type */
            p->cyl | ((p->sector & 0xc0) << 2),          /* Start cylinder */
            p->head,                                     /* Start head */
            p->sector & 0x3f,                            /* Start sector */
            p->end_cyl | ((p->end_sector & 0xc0) << 2),  /* End cylinder */
            p->end_head,                                 /* End head */
            p->end_sector & 0x3f,                        /* End sector */
            start_sect,                                  /* Size*/
            nr_sects);                                   /* Sector count */
    }
    if (bad_mbr)
        printf("\nWarning: Invalid MBR!\n");
    if (fd >= 0)
        close(fd);
}

int main(int argc, char **argv)
{
    int i;
    int mode = MODE_EDIT;
    struct stat stat;

    dev[0] = 0;
    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            switch(*(argv[i] + 1)) {
            case 'l':
                mode = MODE_LIST;
                break;
            default:
                goto usage;
            }
        } else {
            if (*dev != 0) goto usage;
            else strcpy(dev, argv[i]);
        }
    }

    if (argc == 1)
        strcpy(dev,DEFAULT_DEV);


    if (mode == MODE_LIST) {
        if (*dev == 0)
            strcpy(dev,DEFAULT_DEV);
        list_partition(dev);
        return 0;
    }

    if (mode == MODE_EDIT) {
        Funcs *tmp;
        char buf[CMDLEN];

        if ((pFd = open(dev, O_RDWR)) < 0) {
            printf("Error opening %s (%d)\n", dev, errno);
            return 1;
        }
        if (fstat(pFd, &stat) < 0 || !S_ISBLK(stat.st_mode) ||
            (stat.st_rdev & PARTITION_MINORS)) {
            printf("Block device %s is a partition: use non-partitioned device name\n",
                dev);
            return 1;
        }
        if (read(pFd,MBR,512) != 512) {
            printf("Unable to read boot sector from %s (%d)\n", dev, errno);
            return 1;
        }

        {
#ifdef _M_I86
        struct hd_geometry hdgeometry;
        if (ioctl(pFd, HDIO_GETGEO, &hdgeometry)) {
            printf("Error reading geometry for %s\n", dev);
            return 1;
        }
        geometry.sectors = hdgeometry.sectors;
        geometry.heads = hdgeometry.heads;
        geometry.cylinders = hdgeometry.cylinders;
        geometry.start = hdgeometry.start;
#else
        //FIXME read from FAT VBR or ELKS EPB
        geometry.sectors = 63;
        geometry.heads = 16;
        geometry.cylinders = 63;
        geometry.start = 1;                     /* start sector*/
#endif
        }

        printf("Using %s\nGeometry: %d cylinders, %d heads, %d sectors.\n",
                dev, geometry.cylinders,geometry.heads,geometry.sectors);

        /* Don't proceed if any geometry component is bad */
        if (geometry.heads == 0 || geometry.cylinders == 0 || geometry.sectors == 0) {
            printf("Error: invalid geometry for %s\n", dev);
            return 1;
        }

        while (!feof(stdin)) {
            printf("Command (? for help): ");
            fflush(stdout);
            *buf = 0;
            if (fgets(buf,CMDLEN-1,stdin)) {
                printf("\n");
                for (tmp=funcs; tmp->cmd; tmp++)
                    if (*buf==tmp->cmd) {
                        tmp->func();
                        break;
                    }
            }
        }
    }
    return 0;

usage:
    fprintf(stderr, "Usage: fdisk [-l] [/dev/{hda,hdb}]\n");
    return 1;
}
