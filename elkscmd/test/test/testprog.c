#include <stdio.h>

int main() {
  __u16 i;
  
  i=10;
  
  if (i > -1) {  
      printf("\n %d is greater than -1\n",i);
    } else {
      printf("\n %d is smaller than 0\n",i);
    }

  if (i >= 0) {
      printf("\n %d is greater than -1\n",i);
    } else {
      printf("\n %d is smaller than 0\n",i);
    }
  return 0;
}
 
