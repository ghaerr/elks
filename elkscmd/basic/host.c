#include "host.h"
#include "basic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include <linuxmt/config.h>

__STDIO_PRINT_FLOATS;		// link in libc printf float support

unsigned char mem[MEMORY_SIZE];
#define TOKEN_BUF_SIZE    64
static unsigned char tokenBuf[TOKEN_BUF_SIZE];

static FILE *infile;

void host_cls() {
	printf("\033[H\033[2J");
}

void host_moveCursor(int x, int y) {
	printf("\033[%d;%dH", y, x);
}

void host_showBuffer() {
	fflush(stdout);
}

void host_outputString(char *str) {
	printf("%s", str);
}

void host_outputChar(char c) {
	printf("%c", c);
}

void host_outputLong(long num) {
	printf("%ld", num);
}

char *host_floatToStr(float f, char *buf) {
#if 1
	sprintf(buf, "%g", (double)f);
    return buf;
#else
    // floats have approx 7 sig figs
    float a = fabs(f);
    if (f == 0.0f) {
        buf[0] = '0'; 
        buf[1] = 0;
    }
    else if (a<0.0001 || a>1000000) {
        // this will output -1.123456E99 = 13 characters max including trailing nul
        dtostre(f, buf, 6, 0);
    }
    else {
        int decPos = 7 - (int)(floor(log10(a))+1.0f);
        dtostrf(f, 1, decPos, buf);
        if (decPos) {
            // remove trailing 0s
            char *p = buf;
            while (*p) p++;
            p--;
            while (*p == '0') {
                *p-- = 0;
            }
            if (*p == '.') *p = 0;
        }   
    }
    return buf;
#endif
}

void host_outputFloat(float f) {
    char buf[32];
    host_outputString(host_floatToStr(f, buf));
}

void host_newLine() {
	printf("\n");
}

char *host_readLine() {
	static char buf[80];

	if (!fgets(buf, sizeof(buf), infile))
		return NULL;
	buf[strlen(buf)-1] = 0;
	return buf;
}

char host_getKey() {
#if 1
	return 0;
#else
    char c = inkeyChar;
    inkeyChar = 0;
    if (c >= 32 && c <= 126)
        return c;
    else return 0;
#endif
}

int host_ESCPressed() {
#if 0
    while (keyboard.available()) {
        // read the next key
        inkeyChar = keyboard.read();
        if (inkeyChar == PS2_ESC)
            return true;
    }
#endif
    return false;
}

void host_outputFreeMem(unsigned int val)
{
	printf("%u bytes free\n", val);
}

/* LONG_MAX is not exact as a double, yet LONG_MAX + 1 is */
#define LONG_MAX_P1 ((LONG_MAX/2 + 1) * 2.0)

/* small ELKS floor function using 32-bit longs */
double host_floor(double x)
{
  if (x >= 0.0) {
    if (x < LONG_MAX_P1) {
      return (double)(long)x;
    }
    return x;
  } else if (x < 0.0) {
    if (x >= LONG_MIN) {
      long ix = (long) x;
      return (ix == x) ? x : (double)(ix-1);
    }
    return x;
  }
  return x; // NAN
}

void host_sleep(long ms) {
    usleep(ms * 1000);
}


#ifdef CONFIG_ARCH_8018X
/**
 * NOTE: This only works on MY 80C188EB board, needs to be more
 * generalized using the 8018x's GPIOs instead!
 */
#include "arch/io.h"

static uint8_t the_leds = 0xfe;
void host_digitalWrite(int pin,int state) {
    if (pin > 7) {
        return;
    }

    uint8_t new_state = 1 << pin;
    if (state == 0) {
        the_leds |= new_state;
    } else {
        the_leds &= ~new_state;
    }
    /* there's a latch on port 0x2002 where 8 leds are connected to */
    outb(the_leds, 0x2002);
}

int host_digitalRead(int pin) {
    if (pin > 7) {
        if (pin == 69) {
            /* buffer on port 0x2003, bit #3 is the switch */
            return inb(0x2003) & (1 << 3) ? 1 : 0;
        }
        return 0;
    }

    /* there's a buffer on port 0x2001 where an 8 switches dip switch is conncted to */
    uint8_t b = inb(0x2001);
    uint8_t bit = 1 << pin;

    if (b & bit) {
        return 1;
    }
	return 0;
}
#else

void host_digitalWrite(int pin,int state) {
}

int host_digitalRead(int pin) {
    return 0;
}
#endif

int host_analogRead(int pin) {
    //return analogRead(pin);
	return 0;
}

void host_pinMode(int pin,int mode) {
    //pinMode(pin, mode);
}

#ifdef FS_ACCESS_ALLOWED

#include <dirent.h>

void host_directoryListing() {
	DIR		*dirp;
	struct	dirent	*dp;

    dirp = opendir(".");
    if (dirp == NULL) {
        host_outputString("Error reading dir .\n");
        return;
    }

    while ((dp = readdir(dirp)) != NULL) {
        char *dot = strrchr(dp->d_name, '.'); /* Find last '.', if there is one */
        if (dot && (strcasecmp(dot, ".bas") == 0))
        {
            host_outputString(dp->d_name);
            host_newLine();
        }
    }

    closedir(dirp);
}

int host_removeFile(char *fileName) {
    if (unlink(fileName) < 0) {
        host_outputString("Could not remove ");
        host_outputString(fileName);
        host_newLine();
        return false;
    }
    return true;    
}

int host_loadProgramFromFile(char *fileName) {
    FILE* fh = fopen(fileName, "rb");

    if (fh < 0) {
        sprintf(stderr, "Error opening file '%s'\n", fileName);
        return false;
    }

    fseek(fh, 0, SEEK_END);
    long fsize = ftell(fh);
    if (fsize > sizeof(mem)) {
        sprintf(stderr, "File too large to fit in memory\n");
        fclose(fh);
        return false;
    }
    fseek(fh, 0, SEEK_SET);

    if (fread(mem, 1, sizeof(mem), fh) == fsize) {
        sysPROGEND = fsize;
        fclose(fh);
        return true;
    }

    sprintf(stderr, "Error reading file '%s'\n", fileName);

    fclose(fh);
    return false;
}

int host_saveProgramToFile(char *fileName) {
    FILE* fh = fopen(fileName, "wb");

    if (fh < 0) {
        sprintf(stderr, "Error opening file '%s'\n", fileName);
        return false;
    }

    if (fwrite(mem, 1, sysPROGEND, fh) != sysPROGEND) {
        sprintf(stderr, "Error writing file\n");
        fclose(fh);
        return false;
    }

    fclose(fh);
    return true;
}
#endif

// BASIC

int loop() {
    int ret = ERROR_NONE;

    lineNumber = 0;
    char *input = host_readLine();
    if (!input) return ERROR_EOF;

    if (!strcmp(input, "mem")) {
        host_outputFreeMem(sysVARSTART - sysPROGEND);
        return ERROR_NONE;
    }
    ret = tokenize((unsigned char*)input, tokenBuf, TOKEN_BUF_SIZE);

    /* execute the token buffer */
    if (ret == ERROR_NONE)
        ret = processInput(tokenBuf);

    if (ret != ERROR_NONE) {
        host_newLine();
        if (lineNumber != 0) {
            host_outputLong(lineNumber);
            host_outputChar('-');
        }
        printf("%s\n", errorTable[ret]);
    }
    return ret;
}

int main(int ac, char **av) {
	reset();

	printf("Sinclair BASIC\n");
	host_outputFreeMem(sysVARSTART - sysPROGEND);
	printf("\n");
	//printf("float = %d\n", sizeof(float));
	//printf("LONG_MAX = %ld,%ld\n", LONG_MAX, LONG_MIN);
	//printf("3.1415926 = %f\n", strtod("3.1415926", 0));
	//printf("3.1415926 = %f\n", (double)3.1415926);
	//printf("2e5 = %g\n", strtod("2e5", 0));
	//printf("floor(3.1415926) = %f\n", host_floor(3.1415926));
	//printf("floor(-3.1415926) = %f\n", host_floor(-3.1415926));

	infile = stdin;
	if (ac > 1) {
		infile = fopen(av[1], "r");
		if (!infile) {
			printf("Can't open %s\n", av[1]);
			exit(1);
		}
		while (loop() == ERROR_NONE)
			continue;
		infile = stdin;
	}
    
	for(;;)
		if (loop() == ERROR_EOF)
			break;
	return 0;
}
