#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// Taken from ls.c
char *fmtname(char *path) {
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;

    p++;

    // Return blank-padded name.
    if (strlen(p) >= DIRSIZ)
        return p;

    memmove(buf, p, strlen(p));
    memset(buf + strlen(p), '\0', DIRSIZ - strlen(p));
    return buf;
}

// Recursively find files
// Takes a path, a buffer, a file descriptor, a struct stat, and a search string
void finddir(char *path, char *buf, int fd, struct stat st, char *search) {
    char *p;
    struct dirent de;
    struct stat st2;

    // Check if the path is too long
    if (strlen(path) + 1 + DIRSIZ + 1 > 512) {
        printf("find: path too long\n");
    }

    // Copy the path to the buffer and add a slash
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    // Read the directory entries
    while (read(fd, &de, sizeof(de)) == sizeof(de)) {
        if (de.inum == 0)
            continue;

        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;

        // Get the file status
        if (stat(buf, &st) < 0) {
            printf("find: cannot stat %s\n", buf);
            continue;
        }

        // If it is a directory, recursively call finddir
        if (st.type == T_DIR) {
            if (strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {
                int fd2 = open(buf, 0);
                if (fstat(fd, &st2) < 0) {
                    fprintf(2, "find: cannot stat %s\n", path);
                    close(fd);
                    return;
                }
                finddir(buf, buf, fd2, st2, search);
                close(fd2);
            }
        } 
        // If it is a file, check if it matches the search string
        else if (st.type == T_FILE) {
            if (strcmp(fmtname(buf), search) == 0) {
                printf("%s\n", buf);
            }
        }
    }
}

/**
 * This function prints the specified error message
 * and then terminates the program with a non-zero exit status.
 */
void errorExit(char *msg, char *path) {
    fprintf(2, "find: %s %s\n", msg, path);
    exit(1);
}

void find(char *path, char *search) {
    char buf[512];
    int fd; 
    struct stat st; 

    // Path must be a directory
    if ((fd = open(path, 0)) < 0) {
        errorExit("cannot open %s\n", path);
        return;
    }
    // Path must have stats
    if (fstat(fd, &st) < 0) {
        errorExit("cannot stat %s\n", path);
    }
    // Check if the file type is a directory
    if (st.type == T_DIR) {
        finddir(path, buf, fd, st, search);
    }
    close(fd);
}

int main(int argc, char *argv[]) {
    int i;

    if (argc < 2) {
        errorExit("needs file name", "");
    } else if (argc == 2) { // If no starting-point specified, '.' is assumed
        struct stat st;
        if (stat(argv[1], &st) < 0) {
            errorExit(argv[1], "does not exist");
        }
        if (st.type != T_DIR) {
            errorExit(argv[1], "is not a directory");
        }
        find(".", argv[1]);
    } else {
        for (i = 2; i < argc; i++) {
            find(argv[1], argv[i]);
        }
    }
    exit(0);
}
