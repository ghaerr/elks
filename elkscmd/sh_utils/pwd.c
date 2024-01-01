#include <limits.h>
#include <unistd.h>
#include <string.h>

int main(int ac, char **av)
{
    char wd[PATH_MAX];

    if (getcwd(wd, sizeof(wd)) == NULL) {
        write(STDOUT_FILENO, "Cannot get current directory\n", 29);
        return 1;
    }
    write(STDOUT_FILENO, wd, strlen(wd));
    write(STDOUT_FILENO, "\n", 1);
    return 0;
}
