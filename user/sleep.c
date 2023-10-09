#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char **argv) {
  int ticks0;
  // Check arguments provided
  if (argc < 2) {
    fprintf(2, "Error: Enter a number\n");
    exit(1); // Exit the program with non-zero status code indicating an error
  } 
  for (ticks0 = 1; ticks0 < argc; ticks0++){
    // Check if negative number input 
    if (atoi(argv[ticks0]) < 0) {
      fprintf(2, "Error: Enter a positive number\n");
      exit(1);
    }
    // Print a message indicating the number of ticks the program is going to sleep for
    printf("sleeping for %d ticks\n", atoi(argv[ticks0]));
    // Sleep for the specified number of ticks (converted from the string argument)
    sleep(atoi(argv[ticks0]));
  }
  exit(0);
}