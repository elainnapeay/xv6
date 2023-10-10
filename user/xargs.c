#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(void) {
    char line[256]; // Buffer to store input line
    int i = 0;
    char c;

    while (1) {
        int n = read(0, &c, 1); // Read one character from standard input

        if (n <= 0 || c == '\n') {
            line[i] = '\0'; // Null-terminate the input line
            if (i > 0) {
                // Fork a child process to execute the command
                int pid = fork();

                if (pid < 0) {
                    fprintf(2, "Fork failed\n");
                    exit(1);
                }

                if (pid == 0) {
                    // Child process
                    char *args[4]; // Argument array for exec
                    args[0] = "sh"; // Shell
                    args[1] = "-c"; // Flag to run a command
                    args[2] = line; // Command to execute
                    args[3] = 0; // Null-terminate the argument array

                    exec("/bin/sh", args); // Execute the command
                    fprintf(2, "Exec failed\n");
                    exit(1);
                } else {
                    // Parent process
                    wait(0); // Wait for the child process to complete
                }
            }
            if (n <= 0) {
                // Exit if end of input or error reading input
                break;
            }
            i = 0; // Reset the input line index for the next line
        } else {
            line[i] = c; // Store the character in the input line buffer
            i++;
        }
    }

    exit(0);
}
