#include "host.h"
#include "basic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

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
    //delay(ms);
}

void host_digitalWrite(int pin,int state) {
    //digitalWrite(pin, state ? HIGH : LOW);
}

int host_digitalRead(int pin) {
    //return digitalRead(pin);
	return 0;
}

int host_analogRead(int pin) {
    //return analogRead(pin);
	return 0;
}

void host_pinMode(int pin,int mode) {
    //pinMode(pin, mode);
}

void host_saveProgram(int autoexec) {
#if 0
    EEPROM.write(0, autoexec ? MAGIC_AUTORUN_NUMBER : 0x00);
    EEPROM.write(1, sysPROGEND & 0xFF);
    EEPROM.write(2, (sysPROGEND >> 8) & 0xFF);
    for (int i=0; i<sysPROGEND; i++)
        EEPROM.write(3+i, mem[i]);
#endif
}

void host_loadProgram() {
#if 0
    // skip the autorun byte
    sysPROGEND = EEPROM.read(1) | (EEPROM.read(2) << 8);
    for (int i=0; i<sysPROGEND; i++)
        mem[i] = EEPROM.read(i+3);
#endif
}

#if 0
void writeExtEEPROM(unsigned int address, byte data) 
{
  if (address % 32 == 0) host_click();
  rtc.start((EXTERNAL_EEPROM_ADDR<<1)|I2C_WRITE);
  rtc.write((int)(address >> 8));   // MSB
  rtc.write((int)(address & 0xFF)); // LSB
  rtc.write(data);
  rtc.stop();
  delay(5);
}
 
byte readExtEEPROM(unsigned int address) 
{
  rtc.start((EXTERNAL_EEPROM_ADDR<<1)|I2C_WRITE);
  rtc.write((int)(address >> 8));   // MSB
  rtc.write((int)(address & 0xFF)); // LSB
  rtc.restart((EXTERNAL_EEPROM_ADDR<<1)|I2C_READ);
  byte b = rtc.read(true);
  rtc.stop();
  return b;
}

// get the EEPROM address of a file, or the end if fileName is null
unsigned int getExtEEPROMAddr(char *fileName) {
    unsigned int addr = 0;
    while (1) {
        unsigned int len = readExtEEPROM(addr) | (readExtEEPROM(addr+1) << 8);
        if (len == 0) break;
        
        if (fileName) {
            int found = true;
            for (int i=0; i<=strlen(fileName); i++) {
                if (fileName[i] != readExtEEPROM(addr+2+i)) {
                    found = false;
                    break;
                }
            }
            if (found) return addr;
        }
        addr += len;
    }
    return fileName ? EXTERNAL_EEPROM_SIZE : addr;
}

void host_directoryExtEEPROM() {
    unsigned int addr = 0;
    while (1) {
        unsigned int len = readExtEEPROM(addr) | (readExtEEPROM(addr+1) << 8);
        if (len == 0) break;
        int i = 0;
        while (1) {
            char ch = readExtEEPROM(addr+2+i);
            if (!ch) break;
            host_outputChar(readExtEEPROM(addr+2+i));
            i++;
        }
        addr += len;
        host_outputChar(' ');
    }
    host_outputFreeMem(EXTERNAL_EEPROM_SIZE - addr - 2);
}

int host_removeExtEEPROM(char *fileName) {
    unsigned int addr = getExtEEPROMAddr(fileName);
    if (addr == EXTERNAL_EEPROM_SIZE) return false;
    unsigned int len = readExtEEPROM(addr) | (readExtEEPROM(addr+1) << 8);
    unsigned int last = getExtEEPROMAddr(NULL);
    unsigned int count = 2 + last - (addr + len);
    while (count--) {
        byte b = readExtEEPROM(addr+len);
        writeExtEEPROM(addr, b);
        addr++;
    }
    return true;    
}

int host_loadExtEEPROM(char *fileName) {
    unsigned int addr = getExtEEPROMAddr(fileName);
    if (addr == EXTERNAL_EEPROM_SIZE) return false;
    // skip filename
    addr += 2;
    while (readExtEEPROM(addr++)) ;
    sysPROGEND = readExtEEPROM(addr) | (readExtEEPROM(addr+1) << 8);
    for (int i=0; i<sysPROGEND; i++)
        mem[i] = readExtEEPROM(addr+2+i);
}

int host_saveExtEEPROM(char *fileName) {
    unsigned int addr = getExtEEPROMAddr(fileName);
    if (addr != EXTERNAL_EEPROM_SIZE)
        host_removeExtEEPROM(fileName);
    addr = getExtEEPROMAddr(NULL);
    unsigned int fileNameLen = strlen(fileName);
    unsigned int len = 2 + fileNameLen + 1 + 2 + sysPROGEND;
    if ((long)EXTERNAL_EEPROM_SIZE - addr - len - 2 < 0)
        return false;
    // write overall length
    writeExtEEPROM(addr++, len & 0xFF);
    writeExtEEPROM(addr++, (len >> 8) & 0xFF);
    // write filename
    for (int i=0; i<strlen(fileName); i++)
        writeExtEEPROM(addr++, fileName[i]);
    writeExtEEPROM(addr++, 0);
    // write length & program    
    writeExtEEPROM(addr++, sysPROGEND & 0xFF);
    writeExtEEPROM(addr++, (sysPROGEND >> 8) & 0xFF);
    for (int i=0; i<sysPROGEND; i++)
        writeExtEEPROM(addr++, mem[i]);
    // 0 length marks end
    writeExtEEPROM(addr++, 0);
    writeExtEEPROM(addr++, 0);
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
