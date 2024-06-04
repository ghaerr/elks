/* host dependent definitions */

#define DISK_FUNCTIONS			1	/* compile in DELETE/DIR/LIST/SAVE */

#ifdef __WATCOMC__
#define MATH_FUNCTIONS			0	/* compile in COS, SIN, TAN, etc */
#define MATH_INCLUDE_POW		0	/* compile in large POW function */
#define MATH_DBL_PRECISION		0	/* 0=use 4-byte floats vs 8-byte double */
#define MATH_CORRECT_NEAR_ZERO	0	/* adjust trig routines returning -0, +/-EPSILON */
#else
#define MATH_FUNCTIONS			1	/* compile in COS, SIN, TAN, etc */
#define MATH_INCLUDE_POW		1	/* compile in large POW function */
#define MATH_DBL_PRECISION		0	/* 0=use 4-byte floats vs 8-byte double */
#define MATH_CORRECT_NEAR_ZERO	1	/* adjust trig routines returning -0, +/-EPSILON */
#endif

#if MATH_DBL_PRECISION
#define MATH_PRECISION	14
#define EPSILON         0.00000000000001
#define float double
#define FABS(x)		fabs(x)
#define COS(x)		adjust(cos(x))
#define SIN(x)		adjust(sin(x))
#define TAN(x)		tan(x)
#define ACOS(x)		acos(x)
#define ASIN(x)		asin(x)
#define ATAN(x)		atan(x)
#define EXP(x)		exp(x)
#define LOG(x)		log(x)
#define POW(x,y)	pow(x,y)
#else
#define MATH_PRECISION	6
#define EPSILON         0.000001
#define FABS(x)		fabsf(x)
#define COS(x)		adjust(cosf(x))
#define SIN(x)		adjust(sinf(x))
#define TAN(x)		tanf(x)
#define ACOS(x)		acosf(x)
#define ASIN(x)		asinf(x)
#define ATAN(x)		atanf(x)
#define EXP(x)		expf(x)
#define LOG(x)		logf(x)
#define POW(x,y)	powf(x,y)
#endif

#if MATH_CORRECT_NEAR_ZERO
float adjust(float f);
#else
#define adjust(f)	(f)
#endif

#define PROGMEM
#define pgm_read_word(x)        (x)
#define pgm_read_byte_near(x)	(x)

#define _MK_FP(seg,off) ((void __far *)((((unsigned long)(seg)) << 16) | (off)))

void host_cls();
void host_showBuffer();
void host_moveCursor(int x, int y);
void host_outputString(char *str);
void host_outputChar(char c);
void host_outputLong(long num);
void host_outputFloat(float f);
char *host_floatToStr(float f, char *buf);
float host_floor(float x);
void host_newLine();
char *host_readLine();
int host_getKey();
int host_breakPressed();
void host_outputFreeMem(unsigned int val);
void host_sleep(long ms);

#if DISK_FUNCTIONS
int host_directoryListing();
int host_saveProgramToFile(char *fileName, int autoexec);
int host_loadProgramFromFile(char *fileName);
int host_removeFile(char *fileName);
#endif

/* architecture specific */
void host_digitalWrite(int pin,int state);
int host_digitalRead(int pin);
int host_analogRead(int pin);
void host_pinMode(int pin, int mode);

void host_mode(int mode);
void host_color(int fgc, int bgc);
void host_plot(int x, int y);
void host_draw(int x, int y);
void host_circle(int x, int y, int r);

void host_outb(int port, int value);
void host_outw(int port, int value);
int host_inpb(int port);
int host_inpw(int port);

int host_peekb(int offset, int segment);
void host_pokeb(int offset, int segment, int value);
