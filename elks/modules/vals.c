#include "symchop.pos"

main(){

   printf("dd bs=%d if=serial.D of=serial.X skip=1\n", _module_data);
   printf("mv serial.X serial.D\n");
   printf("dd bs=%d if=serial.T of=serial.X skip=1\n", _module_init);
   printf("mv serial.X serial.T\n");
   exit(0);
}
