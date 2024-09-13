/*
 * Precision timer routines
 * 2 Aug 2024 Greg Haerr
 */

#define TIMER_TEST          0   /* =1 to include timer_*() test routines */

/* returns pticks in 0.838us resolution, 0.838 microseconds to 42.85 seconds  */
unsigned long get_ptime(void);
void init_ptime(void);

/* internal test routines */
void test_ptime_idle_loop(void);
void test_ptime_print(void);

#ifdef __GNUC__
/* silence GCC warning when %k used in printf - FIXME not ideal */
#pragma GCC diagnostic ignored "-Wformat"
#pragma GCC diagnostic ignored "-Wformat-extra-args"
#endif
