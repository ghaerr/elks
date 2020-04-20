#include <unistd.h>

#define CLEARSEQ "\x1b[H\x1b[2J"

int main(int argc, char *argv[])
{
  write(STDOUT_FILENO, CLEARSEQ, 7);

  return 0;
}
