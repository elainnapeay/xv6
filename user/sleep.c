#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char **argv)
{
  int ticks0;
  if(argc < 2){
    fprintf(2, "Error: Enter a number\n");
    exit(1);
  }
  for(ticks0=1; ticks0<argc; ticks0++)
    sleep(atoi(argv[ticks0]));
  exit(0);
}

