/*
 * Precision timer routines
 * 2 Aug 2024 Greg Haerr
 */

#define CONFIG_PREC_TIMER   0   /* =1 to include %k precision timer printk format */
#define TIMER_TEST          0   /* =1 to include timer_*() test routines */

/* all routines return pticks = 0.8381 usecs */
unsigned int get_time_10ms(void);   /* < 10ms measurements */
unsigned int get_time_50ms(void);   /* < 50ms measurements */
unsigned long get_time(void);       /* < 1 hr measurements */

/* timer test routines */
void timer_10ms(void);
void timer_50ms(void);
void timer_4s(void);
void timer_test(void);
