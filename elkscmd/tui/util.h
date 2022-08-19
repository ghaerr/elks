/* See LICENSE file for copyright and license details. */
#undef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))

#define LEN(x) (sizeof(x) / sizeof(*(x)))

#undef PATH_MAX
#undef LINE_MAX
#undef NAME_MAX

#define PATH_MAX    80
#define LINE_MAX    80

#if ELKS
#define NAME_MAX    15
#define NAME_COLS   20
#else
#define NAME_MAX    79
#define NAME_COLS   32
#endif

#undef strlcat
size_t strlcat(char *, const char *, size_t);
#undef strlcpy
size_t strlcpy(char *, const char *, size_t);
int strverscmp(const char *, const char *);
char *realpath(const char *path, char resolved[PATH_MAX]);
