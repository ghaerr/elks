// f"ugt Bin"arfiles zusammen
//
// Christian Mardm"oller  (chm@kdt.de) 
//
// Version 1.0 
// Version 1.1  11/99   Korrektur Checksumme
//----------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXQ  10


#define im(a,u,o)  (((a)>=(u)) && ((a)<=(o)))


struct t_quellen {
   char *daten;
   unsigned long skipaout;
   unsigned long offs,lg;
}; 

struct t_check {
   int gefordert;
   unsigned long start,ende;
};


unsigned long aoutsize(char *aout,unsigned long lg)
 {
    
   unsigned long gr; 

    gr = *(unsigned char*) &(aout[0x04])
       + *(unsigned long*) &(aout[0x08])
       + *(unsigned long*) &(aout[0x0c]);
    if (gr > lg) {
       gr = lg;
       printf("  Wrong a.out table?\n");
    }
    return(gr);   
 }




int main(int argcnt, char **arg)
 {
    FILE *ff;
    unsigned long romgr,offs;
    char *rom; 
    struct t_quellen quellen[MAXQ];
    struct t_check check;
    unsigned long l,i,nr,erstername;
    int strip,skip;
    signed long init;

    if (argcnt < 5) { 
       printf("mkurlader [-r init] [-c start size] target.bin basis_rom [-s][-a] *.bin adr [*.bin adr]\n");
       printf("   -a  skip a.out header (0x20 Bytes)\n"); 
       printf("   -c  insert checksumme at start(seg)+size(kBytes)\n");
       printf("   -r  add resetvector \"jmpf basis_rom*0x10+init\" to target finle at ffff0\n");
       printf("   -s  strips symbols from a.out file\n");
       printf("  adr must above basis_rom\n");
       printf("  mkurlader -p 0000 003ff  rom.bin e000  arch/i86/boot/setup e000 arch/i86/tools/system e040\n");
       return(-1);
    }

    for (l=0;l<MAXQ;l++) quellen[l].daten = NULL;
    init = -1;
    check.gefordert = 0;


// Quellen laden
    i = 1;
    if (strcmp(arg[i],"-r") == 0) {
       sscanf(arg[i+1],"%lx",&init);
       i += 2;
    }
    if (strcmp(arg[i],"-c") == 0) {
       sscanf(arg[i+1],"%lx",&check.start);
       check.start *= 0x10;
       sscanf(arg[i+2],"%ld",&check.ende);
       check.ende *= 1024;
       check.gefordert = 1000;
       i += 3;
    }
    erstername = i;
    i += 2;
    nr = 0;
    while (i<argcnt) {
       if (strcmp(arg[i],"-s") == 0) {
          strip = 1;
          i++;
       }
       else strip = 0; 
       if (strcmp(arg[i],"-a") == 0) {
          skip = 1;
          i++;
       }
       else skip = 0;
       ff = fopen(arg[i],"rb");
       if (!ff) {
          printf("Can't open file %s\n",arg[i]);
          return -1;
       }
       fseek(ff,0,SEEK_END);
       quellen[nr].lg = ftell(ff);
       fseek(ff,0,SEEK_SET);
       quellen[nr].skipaout = (skip)?0x20:0;
       //printf("%lx\n",quellen[nr].lg);
       quellen[nr].daten = (char*)malloc(quellen[nr].lg);
       fread(quellen[nr].daten,1,quellen[nr].lg,ff);
       fclose(ff);
       if (arg[i+1] == NULL) {
          printf("ERROR: No base address for %s!\n",arg[i]);
          return(-1);
       }
       sscanf(arg[i+1],"%lx",&quellen[nr].offs);
       if (strip) {
          quellen[nr].lg = aoutsize(quellen[nr].daten,quellen[nr].lg);
       }
       printf("  %s: %lxh Bytes %s%s @%04lx\n",arg[i],quellen[nr].lg - quellen[nr].skipaout,(strip)?"(strip)":"",(skip)?"(- a.out)":"",quellen[nr].offs);
       i += 2;  // n"achste Quelle
       nr++;
    }


// Ziel anlegen
    i = erstername;
    ff = fopen(arg[i],"wb");
    if (!ff) {
       printf("Can't generate file %s\n",arg[i]);
       return -1;
    }
    sscanf(arg[i+1],"%lx",&offs);
    offs *= 0x10;
    //printf("%lx\n",offs);


// Gr"osze bestimmen
    romgr = 0;
    i = 0;
    while (quellen[i].daten != NULL) {
       quellen[i].offs *= 0x10;
       l = quellen[i].offs + (quellen[i].lg - quellen[i].skipaout) - offs;
       //printf("%ld: qoffs %lx + lg %lx - skip %lx - offs %lx = l %lx\n",i,quellen[i].offs,quellen[i].lg,quellen[i].skipaout, offs,l);
       if ((signed long)l < 0) {
          printf("Bereichsfehler in Nr. %ld (offs < basis)!\n",i);
          fclose(ff);
          return(-1);
       }
       if (l>romgr) romgr = l;
       i++;
    }

    printf("--> %s: %lxh Bytes @%04lx\n",arg[erstername],romgr,offs/0x10);
    if ((offs + romgr > 0xfffff) || ((init>=0) && (offs+romgr >= 0xffff0))) {
       printf("ERROR: ROM-Image too big!\n");
       fclose(ff);
       return -1;
    }
    if (init >= 0) {
       romgr = 0xffff5-offs;
    }
    if (check.gefordert) {
       i = check.ende + check.start -offs;
       if (i > romgr) romgr = i;
    }

// Zusammensetzen
    rom = NULL;
    //printf("romgr: %lx\n",romgr);
    if (romgr < 0x100000) rom = (char*)malloc(romgr);
    if (rom == NULL) {
       printf("malloc: no %ld Bytes RAM\n",romgr);
       return(-1);
    }
    for (l=0;l<romgr;rom[l++] = 0xff);   // Speicher initialisieren

    nr = 0;
    while (quellen[nr].daten != NULL) {
       if (check.gefordert && (check.start == quellen[nr].offs)) check.gefordert = nr+1;
       i = 0;
       l = 0;   // kein fehler
       if ((init>=0) && (quellen[nr].offs+quellen[nr].lg >= 0xffff0)) l = 1000;
       while (!l && (quellen[i].daten != NULL)) {
          if (i != nr) {
              if (im(quellen[nr].offs, quellen[i].offs, quellen[i].offs+quellen[i].lg - quellen[nr].skipaout)) {
                 printf("    # %x in [%x..%x]\n",quellen[nr].offs, quellen[i].offs, quellen[i].offs+quellen[i].lg - quellen[nr].skipaout);
                 l = i+1;   // "uberschneidung
              }
          }
          i++;
       }
       if (l) {
          printf("\x1b[7;34m");
          if (l==1000) printf("!! Segmentoverrun (%ld <-> Resetvektor)!\x1b[0m\n",nr);
          else printf("!! Segmentoverrun (%ld <-> %ld)!\x1b[0m\n",nr,l-1);
       }
       else {
          l = quellen[nr].offs - offs;
          memcpy(&(rom[l]), quellen[nr].daten + quellen[nr].skipaout , quellen[nr].lg - quellen[nr].skipaout );
       }
       nr++;
    }


// Resetvektor einf"ugen
    if (init>= 0) {
       rom[0xffff0-offs] = 0xea;  // jmpf
       l = offs + init;
       l = ((l&0xfff00)<<12) + (l&0x000ff);
       *(long*) &(rom[0xffff1-offs]) = l;
       printf("  RESET nach %04x:%04x (%05lx)\n",(unsigned)l>>16,(unsigned)l&0xffff,offs+init);
    }

// Pr"ufsumme berechnen
    if (check.gefordert) {
       check.ende += check.start -1;
       //printf("%lx %lx\n",check.start, check.ende);
       check.start -= offs;
       check.ende -= offs;
       if (check.ende > romgr) {
          printf("!! checksumme not in image file\n");
          return(-1);
       }
       else {
          if ((rom[check.start] != 0x55) || (rom[check.start+1] != (char)0xaa)) {
             printf("!! No ROM-Signature at %05lx\n",check.start);
          }
          else {
             i = 0;
             while (quellen[i].daten != NULL) {
                if (i+1 != check.gefordert) {
                   if (im(check.start+offs, quellen[i].offs, quellen[i].offs+quellen[i].lg - quellen[i].skipaout) ||
                       im(check.ende+offs, quellen[i].offs, quellen[i].offs+quellen[i].lg - quellen[i].skipaout)) {
                          printf("!! checksum over more then one file\n");
                          break;
                   }
                }
                i++;
             }
             if (((check.ende - check.start +1) % 512) != 0)
                printf("!!  Area of checksum use half paragraphs (Start: %lx, End: %lx => %lx)\n",check.start, check.ende+1, check.ende - check.start +1);
             else {
                rom[check.start+2] = (unsigned char) ((check.ende - check.start + 511L) / 512L);
                printf("  Chksum Nr %d - Length in 512 bytes blocks: %02x   ",check.gefordert-1,rom[check.start+2]&0xff);
                l = 0;
                rom[check.ende] = 0;
                for (i=check.start;i<check.ende;i++) l += rom[i] & 0xff;
                rom[check.ende] = -l;
                printf("[%02x @%05lx]\n",rom[check.ende]&0xff,check.ende);
                if ((unsigned char)rom[check.start+2] < 3) printf("!! Some BIOSes have problems with so small devices\n");
                i = 0;
                while (quellen[i].daten != NULL) {
                   //printf("%lx %lx %lx\n",check.ende, quellen[i].offs - offs, quellen[i].lg - quellen[i].skipaout);
                   if (im(check.ende, quellen[i].offs -  offs, quellen[i].lg - quellen[i].skipaout)) {
                       printf("\x1b[7;34m");
                       printf("!! checksum is in changed values of image %d.\x1b[0m\n",i);
                       break;
                   }
                   i++;
                }
             }
          }
       }
    }


// Wegschreiben
    fwrite(rom,1,romgr,ff);
    fclose(ff);

    return 0;
 }



