/* Merge a.out object files into a ROM boot "Image"
 * (typically "setup" & "system")
 *
 * Written by Christian Mardm"oller  (chm@kdt.de) 
 * Some German->English edits by Jody Bruchon <jody@jodybruchon.com>
//------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


#define MAXQ  10

#define im(a,u,o)  (((a)>=(u)) && ((a)<=(o)))

/* structure with data and information of the source files */
struct t_source {
    char *data;
    uint32_t skipaout;
    uint32_t offs, lg;
};

/* information about the checksum area */
struct t_check {
    int sum;
    uint32_t start, end;
};

//----------------------------------------------------
// calculate the size of an a.out binary
//
uint32_t aoutsize(char *aout, uint32_t lg)
{
    uint32_t gr;

    gr = *(unsigned char *) &(aout[0x04])
	+ *(uint32_t *) &(aout[0x08])
	+ *(uint32_t *) &(aout[0x0c]);
    if (gr > lg) {
	gr = lg;
	printf("  Wrong a.out table?\n");
    }
    return gr;
}


//----------------------------------------------------
//
int main(int argcnt, char **arg)
{
    FILE *ff;
    uint32_t romgr;	// size of target binary
    uint32_t offs;		// base address of eprom in memory space
    char *rom;			// buffer for target date
    struct t_source source[MAXQ];	// data of the source files 
    struct t_check check;	// info about the checksum area
    uint32_t l, i, nr;
    unsigned firstname;		// nummber of first source in argument string
    int strip;			// local bool: strip the symbol table form a.out
    int skip;			// local bool: skip a.out header
    int32_t init;		// if > 0 address of reset vector

    if (argcnt < 5) {
	printf
	    ("mkbootloader [-r init] [-c start size] target.bin basis_rom [-s][-a] *.bin adr [*.bin adr]\n");
	printf("   -a  skip a.out header (0x20 Bytes)\n");
	printf("   -c  insert checksum at start(seg)+size(kBytes)\n");
	printf
	    ("   -r  add resetvector \"jmpf basis_rom*0x10+init\" to target finle at ffff0\n");
	printf("   -s  strips symbols from a.out file\n");
	printf("  adr must above basis_rom\n");
	printf("  mkbootloader -p 0000 003ff  rom.bin e000  arch/i86/boot/setup e000 arch/i86/tools/system e040\n");
	return -1;
    }

    for (l = 0; l < MAXQ; l++)
	source[l].data = NULL;
    init = -1;
    check.sum = 0;

// load sources
    i = 1;
    if (strcmp(arg[i], "-r") == 0) {
	sscanf(arg[i + 1], "%x", &init);
	i += 2;
    }
    if (strcmp(arg[i], "-c") == 0) {
	sscanf(arg[i + 1], "%x", &check.start);
	check.start *= 0x10;
	sscanf(arg[i + 2], "%d", &check.end);
	check.end *= 1024;
	check.sum = 1000;
	i += 3;
    }
    firstname = i;
    i += 2;
    nr = 0;
    while (i < argcnt) {
	if (strcmp(arg[i], "-s") == 0) {
	    strip = 1;
	    i++;
	} else
	    strip = 0;
	if (strcmp(arg[i], "-a") == 0) {
	    skip = 1;
	    i++;
	} else
	    skip = 0;
	ff = fopen(arg[i], "rb");
	if (!ff) {
	    printf("Can't open file %s\n", arg[i]);
	    return -1;
	}
	fseek(ff, 0, SEEK_END);
	source[nr].lg = ftell(ff);
	fseek(ff, 0, SEEK_SET);
	source[nr].skipaout = (skip) ? 0x20 : 0;
	source[nr].data = (char *) malloc(source[nr].lg);
	fread(source[nr].data, 1, source[nr].lg, ff);
	fclose(ff);
	if (arg[i + 1] == NULL) {
	    printf("ERROR: No base address for %s!\n", arg[i]);
	    return -1;
	}
	sscanf(arg[i + 1], "%x", &source[nr].offs);
	if (strip) {
	    source[nr].lg = aoutsize(source[nr].data, source[nr].lg);
	}
	printf("  %s: %xh Bytes %s%s @%04x\n", arg[i],
	       source[nr].lg - source[nr].skipaout, (strip) ? "(strip)" : "",
	       (skip) ? "(- a.out)" : "", source[nr].offs);
	i += 2;			// n"achste Quelle
	nr++;
    }

// generate target file
    i = firstname;
    ff = fopen(arg[i], "wb");
    if (!ff) {
	printf("Can't generate file %s\n", arg[i]);
	return -1;
    }
    sscanf(arg[i + 1], "%x", &offs);
    offs *= 0x10;

// calculate size of target file
    romgr = 0;
    i = 0;
    while (source[i].data != NULL) {
	source[i].offs *= 0x10;
	l = source[i].offs + (source[i].lg - source[i].skipaout) - offs;
	//printf("%ld: qoffs %lx + lg %lx - skip %lx - offs %lx = l %lx\n",i,source[i].offs,source[i].lg,source[i].skipaout, offs,l);
	if ((int32_t) l < 0) {
	    printf("Out of bound in #%u (offset < base)!\n", i);
	    fclose(ff);
	    return -1;
	}
	if (l > romgr)
	    romgr = l;
	i++;
    }

    printf("--> %s: %xh Bytes @%04x\n", arg[firstname], romgr, offs / 0x10);
    if ((offs + romgr > 0xfffff) || ((init >= 0) && (offs + romgr >= 0xffff0))) {
	printf("ERROR: ROM-Image too big!\n");
	fclose(ff);
	return -1;
    }
    if (init >= 0) {
	romgr = 0xffff5 - offs;
    }
    if (check.sum) {
	i = check.end + check.start - offs;
	if (i > romgr)
	    romgr = i;
    }
// merge the binaries
    rom = NULL;
    if (romgr < 0x100000)
	rom = (char *) malloc(romgr);
    if (rom == NULL) {
	printf("malloc: no %u Bytes RAM\n", romgr);
	return -1;
    }
    for (l = 0; l < romgr; rom[l++] = 0xff);	// storage initialization

    nr = 0;
    while (source[nr].data != NULL) {
	if (check.sum && (check.start == source[nr].offs))
	    check.sum = nr + 1;
	i = 0;
	l = 0;			// this is duplicated on purpose
	if ((init >= 0) && (source[nr].offs + source[nr].lg >= 0xffff0))
	    l = 1000;
	while (!l && (source[i].data != NULL)) {
	    if (i != nr) {
		if (im
		    (source[nr].offs, source[i].offs,
		     source[i].offs + source[i].lg - source[nr].skipaout)) {
		    printf("    # %x in [%x..%x]\n", source[nr].offs,
			   source[i].offs,
			   source[i].offs + source[i].lg -
			   source[nr].skipaout);
		    l = i + 1;	// "uberschneidung
		}
	    }
	    i++;
	}
	if (l) {
	    printf("\x1b[7;34m");
	    if (l == 1000)
		printf("Segment overrun (%u <-> Resetvektor)!\x1b[0m\n",
		       nr);
	    else
		printf("Segment overrun (%u <-> %u)!\x1b[0m\n", nr, l - 1);
	} else {
	    l = source[nr].offs - offs;
	    memcpy(&(rom[l]), source[nr].data + source[nr].skipaout,
		   source[nr].lg - source[nr].skipaout);
	}
	nr++;
    }


// add the reset vector
    if (init >= 0) {
	rom[0xffff0 - offs] = 0xea;	// jmpf
	l = offs + init;
	l = ((l & 0xfff00) << 12) + (l & 0x000ff);
	*(uint32_t *) &(rom[0xffff1 - offs]) = l;
	printf("  RESET nach %04x:%04x (%05x)\n", l >> 16,
	       l & 0xffff, offs + init);
    }

// calculate the checksum
    if (check.sum) {
	check.end += check.start - 1;
	check.start -= offs;
	check.end -= offs;
	if (check.end > romgr) {
	    printf("checksum not in image file\n");
	    return -1;
	} else {
	    if ((rom[check.start] != 0x55)
		|| (rom[check.start + 1] != (char) 0xaa)) {
		printf("No ROM-Signature at %05x\n", check.start);
	    } else {
		i = 0;
		while (source[i].data != NULL) {
		    if (i + 1 != check.sum) {
			if (im
			    (check.start + offs, source[i].offs,
			     source[i].offs + source[i].lg -
			     source[i].skipaout)
			    || im(check.end + offs, source[i].offs,
				  source[i].offs + source[i].lg -
				  source[i].skipaout)) {
			    printf("checksum over more then one file\n");
			    break;
			}
		    }
		    i++;
		}
		if (((check.end - check.start + 1) % 512) != 0)
		    printf
			("Area of checksum use half paragraphs (Start: %x, End: %x => %x)\n",
			 check.start, check.end + 1,
			 check.end - check.start + 1);
		else {
		    rom[check.start + 2] =
			(unsigned char) ((check.end - check.start + 511L) /
					 512L);
		    printf
			("  Chksum Nr %d - Length in 512 bytes blocks: %02x   ",
			 check.sum - 1, rom[check.start + 2] & 0xff);
		    l = 0;
		    rom[check.end] = 0;
		    for (i = check.start; i < check.end; i++)
			l += rom[i] & 0xff;
		    rom[check.end] = -l;
		    printf("[%02hhx @%05x]\n", rom[check.end] & 0xff,
			   check.end);
		    if ((unsigned char) rom[check.start + 2] < 3)
			printf
			    ("Some BIOSes have problems with so small devices\n");
		    i = 0;
		    while (source[i].data != NULL) {
			if (im
			    (check.end, source[i].offs - offs,
			     source[i].lg - source[i].skipaout)) {
			    printf("\x1b[7;34m");
			    printf
				("checksum is in changed values of image %d.\x1b[0m\n",
				 i);
			    break;
			}
			i++;
		    }
		}
	    }
	}
    }

// write the target file
    fwrite(rom, 1, romgr, ff);
    fclose(ff);

    return 0;
}
