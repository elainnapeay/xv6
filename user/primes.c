#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void getPrime(int fd)
{
    int num;
    int prime;
    
    //create a new pipe to connect this process with its child.
    int p[2];
    pipe(p);

    //retrieve the first number from the first pipe and set it as the prime and print. If the pipe is empty, end the process.
    if(read(fd, &num, 2) > 0)
    {
        prime = num;
        printf("prime %d\n", prime);
    }else
    {
        exit(0);
    }

    //reads through the first pipe, if the number is not divisible by the prime, write to the new pipe.
    while(read(fd, &num, 2) > 0)
    {
        if(num % prime != 0)
        {
            write(p[1], &num, 2);
        }
    }

    //write end of the pipe is no longer need, can be closed.
    close(p[1]);

    //if forked process is a child, pass pipe into function. If parent, read end of the pipe is no longer need. Wait for child process to exit.
    if(fork() == 0)
    {
        getPrime(p[0]);
    }else
    {
        close(p[0]);
        wait(0);
    }
}

int main()
{
    int p[2];
    pipe(p);

    for(int i = 2; i <= 35; i++)
    {
        write(p[1], &i, 2);
    }

    //write end of the pipe is no longer need, can be closed.
    close(p[1]);

    //passes read end of pipe as argument.
    getPrime(p[0]);

    exit(0);
}