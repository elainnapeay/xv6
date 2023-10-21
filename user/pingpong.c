#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char **argv)
{
  int p[2];
  char buf[10];
  int pid;
  pipe(p);  // Create a pipe    
  pid = fork(); // Create a child process
    if (pid == 0) { // Child process
        read(p[0], buf, 1); // Read from pipe
        printf("%d: received ping\n", getpid());
        write(p[1], "ping", 1); // Write to pipe
    } else { // Parent process
        write(p[1], "pong", 1); // Write to pipe
        wait(0); // Wait for child process to finish
        read(p[0], buf, 1); // Read from pipe
        printf("%d: received pong\n", getpid());
    }
    exit(0);
}