#include "testlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* TODO self-register tests */
void test_error_strerror();
void test_inet_aton_ntoa();
void test_inet_gethostbyname();
void test_malloc_alloca();
void test_malloc_calloc();
void test_malloc_malloc_free();
void test_malloc_realloc();
void test_math_abs();
void test_math_floor();
void test_math_pow();
void test_math_sqrt();
void test_misc_basename();
void test_misc_crypt();
void test_misc_dirname();
void test_misc_getcwd();
void test_misc_strtol();
void test_regex_regcomp();
void test_regex_expandwildcards();
void test_stdio_fgets_boundary();
void test_stdio_init();
void test_stdio_seek();
void test_string_memchr();
void test_string_strcat();
void test_string_strcmp();
void test_string_strcpy();
void test_string_strlen();
void test_string_strncat();
void test_string_strncmp();
void test_string_strncpy();
void test_system_dirent();
void test_system_ioctl();
void test_system_reboot();
void test_system_sleep();
void test_system_umount();
void test_system_ustatfs();
void test_time_gettimeofday();
void test_time_gmtime();
void test_time_mktime();

void usage(char **argv)
{
	fprintf(stderr, "Usage: %s [-a] [-b] [-f] [-v]\n", argv[0]);
	fprintf(stderr, "\t-a\tAbort on first failure\n");
	fprintf(stderr, "\t-d\tBreak into debugger for each failure\n");
	fprintf(stderr, "\t-f\tFork to run test in child process\n");
	fprintf(stderr, "\t-v\tVerbose\n");

	exit(1);
}

int main(int argc, char **argv)
{
	int i;

	for (i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-a"))
			testlib_abortOnFail = 1;
		else if (!strcmp(argv[i], "-d"))
			testlib_debugOnFail = 1;
		else if (!strcmp(argv[i], "-f"))
			testlib_forkTest = 1;
		else if (!strcmp(argv[i], "-v"))
			testlib_verbose = 1;
		else
			usage(argv);
	}

	testfn_t tests[38];
	i = 0;
	tests[i++] = test_error_strerror;
	tests[i++] = test_inet_aton_ntoa;
	tests[i++] = test_inet_gethostbyname;
	tests[i++] = test_malloc_alloca;
	tests[i++] = test_malloc_calloc;
	tests[i++] = test_malloc_malloc_free;
	tests[i++] = test_malloc_realloc;
	tests[i++] = test_math_abs;
	tests[i++] = test_math_floor;
	tests[i++] = test_math_pow;
	tests[i++] = test_math_sqrt;
	tests[i++] = test_misc_basename;
	tests[i++] = test_misc_crypt;
	tests[i++] = test_misc_dirname;
	tests[i++] = test_misc_getcwd;
	tests[i++] = test_misc_strtol;
	tests[i++] = test_regex_regcomp;
	tests[i++] = test_regex_expandwildcards;
	tests[i++] = test_stdio_fgets_boundary;
	tests[i++] = test_stdio_init;
	tests[i++] = test_stdio_seek;
	tests[i++] = test_string_memchr;
	tests[i++] = test_string_strcat;
	tests[i++] = test_string_strcmp;
	tests[i++] = test_string_strcpy;
	tests[i++] = test_string_strlen;
	tests[i++] = test_string_strncat;
	tests[i++] = test_string_strncmp;
	tests[i++] = test_string_strncpy;
	tests[i++] = test_system_dirent;
	tests[i++] = test_system_ioctl;
	tests[i++] = test_system_reboot;
	tests[i++] = test_system_sleep;
	tests[i++] = test_system_umount;
	tests[i++] = test_system_ustatfs;
	tests[i++] = test_time_gettimeofday;
	tests[i++] = test_time_gmtime;
	tests[i++] = test_time_mktime;

	testlib_runTestCases(&tests[0], &tests[i]);

	return testlib_report();
}
