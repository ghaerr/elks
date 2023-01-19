/* constructor, destructor and atexit tests */
#include <stdlib.h>
#include <unistd.h>

#define msg(str)    write(STDOUT_FILENO, str, sizeof(str) - 1)

__attribute__((constructor(201)))
void constructor_1(void)
{
    msg("Constructor 1\n");
}   

__attribute__((constructor(202)))
void constructor_2(void)
{
    msg("Constructor 2\n");
}   

__attribute__((constructor(203)))
void constructor_3(void)
{
    msg("Constructor 3\n");
}   

__attribute__((destructor(201)))
void destructor_1(void)
{
    msg("Destructor 1\n");
}

__attribute__((destructor(202)))
void destructor_2(void)
{
    msg("Destructor 2\n");
}
__attribute__((destructor(203)))
void destructor_3(void)
{
    msg("Destructor 3\n");
}   

void atexit_1(void)
{
    msg("Atexit 1\n");
}

void atexit_2(void)
{
    msg("Atexit 2\n");
}

void atexit_3(void)
{
    msg("Atexit 3\n");
}

int main(int ac, char **av)
{
    atexit(atexit_1);
    atexit(atexit_2);
    atexit(atexit_3);
}
