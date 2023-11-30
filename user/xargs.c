#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"


#define NULL ((char*)0)

char* readline(){
    char buf[512], c;
    int pos = -1;
    while (read(0, &c, sizeof(char))) {
        if (c == '\n') {
            break;
        } else {
            buf[++pos] = c;
        }
    }
    // If there are no characters in the buffer, return NULL.
    if (pos == -1) {
        return NULL;
    }
    // Allocate memory for the line and copy it from the buffer.
    char *p = malloc((pos+1) * sizeof(char));
    memmove(p, buf, pos+1);
    return p;
}



int main (int argc, char *argv[]){

    if(argc < 3){
        fprintf(2, "Usage: xargs <command> <args>\n");
        exit(1);
    }


    char *argv_array[MAXARG];
    for(int i=1; i<argc; i++){
        argv_array[i-1] = argv[i];
    }
    char *r;
    while ((r = readline()) != NULL) {
        // Append the read line as an argument to the command.
        argv_array[argc-1] = r;

        if (fork() == 0) {
            // Child process: execute the command with the modified arguments.
            exec(argv_array[0], argv_array);
            free(r); // Free the memory allocated for the read line.
            exit(0);
        } else {
            // Parent process: wait for the child to complete the command.
            wait(0);
        }
    }	
exit(0);
}