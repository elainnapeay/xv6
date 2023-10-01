#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

void 
find(char *path){
    int fd;
    struct dirent de;
    struct stat st;
    
    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
            continue;
        char buf[512];
        //printf("%s/%s\n", path, de.name);
        if(stat(buf, &st) < 0){
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        if(st.type == T_DIR){
            find(buf);
        }
        
        fprintf(2,"%s\n", buf);
        close(fd);

    }

}
int main (int argc, char *argv[]){
    if (argc != 2){
        fprintf(2,"Usage: %s <dir>\n");
        exit(1);
    }

    
    find(argv[1]);

    exit(0);
}