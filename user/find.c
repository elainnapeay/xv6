// #include "kernel/fs.h"
// #include "kernel/stat.h"
// #include "kernel/types.h"
// #include "user/user.h"

// // Function to extract the last part of a path (filename)
// char *fmtname(char *path) {
//   static char buf[DIRSIZ + 1]; // Buffer to store the filename
//   char *p;

//   // Find first character after last slash.
//   for (p = path + strlen(path); p >= path && *p != '/'; p--)
//     ;
//   p++;

//   // Return blank-padded name.
//   if (strlen(p) >= DIRSIZ)
//     return p;
//   // If the filename is shorter than DIRSIZ, copy it to the buffer 'buf',
//   // pad the remaining space with null characters, and return 'buf'.
//   memmove(buf, p, strlen(p)); // Copy the filename to 'buf'
//   memset(buf + strlen(p), '\0', DIRSIZ - strlen(p)); // Pad with null characters
//   return buf;
// }

// void recursiveFind(char *path, char *buf, int fd, struct stat st,
//                    char *search) {
//   char *p;
//   struct dirent de;
//   struct stat st2;

//   // Check if the combined path length is too long
//   if (strlen(path) + 1 + DIRSIZ + 1 > 512) {
//     printf("find: path too long\n");
//   }
//   // Copy the current path to the buffer and prepare for concatenation
//   strcpy(buf, path);
//   p = buf + strlen(buf);
//   *p++ = '/';

//   // Iterate through directory entries
//   while (read(fd, &de, sizeof(de)) == sizeof(de)) {
//     if (de.inum == 0)
//       continue;
//     // Concatenate the entry name to the current path
//     memmove(p, de.name, DIRSIZ);
//     p[DIRSIZ] = 0;
//     // Get the status of the concatenated path
//     if (stat(buf, &st) < 0) {
//       printf("find: cannot stat %s\n", buf);
//       continue;
//     }
//     // Check if the entry is a directory
//     if (st.type == T_DIR) {
//       // Ignore '.' and '..' entries to prevent infinite recursion
//       if (strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {
//         int fd2 = open(buf, 0);
//         if (fstat(fd, &st2) < 0) {
//           fprintf(2, "find: cannot stat %s\n", path);
//           close(fd);
//           return;
//         }
//         // Recursively search the subdirectory
//         recursiveFind(buf, buf, fd2, st2, search);
//         close(fd2);
//       }
//     }
//     // Check if the entry is a regular file and matches the search term
//     else if (st.type == T_FILE) {
//       if (strcmp(fmtname(buf), search) == 0) {
//         printf("%s\n", buf);
//       }
//     }
//   }
// }
// void find(char *path, char *search) {
//   char buf[512];
//   int fd;
//   struct stat st;

//   // Check if the path is a directory
//   if ((fd = open(path, 0)) < 0) {
//     fprintf(2, "find: cannot open %s\n", path);
//     return;
//   }
//   // Get the status of the path
//   if (fstat(fd, &st) < 0) {
//     fprintf(2, "find: cannot stat %s\n", path);
//     close(fd);
//     return;
//   }
//   // check if the if file is a dir
//   if (st.type == T_DIR) {
//     recursiveFind(path, buf, fd, st, search);
//   }
//   close(fd);
// }

// int main(int argc, char *argv[]) {
//   int i;

//   if (argc < 2) {
//     printf("Usage: find <path> <search>\n");
//   } else if (argc == 2) {
//     find(".", argv[1]); // If only one search term provided, search in the current directory
//   } else {
//     // Loop through the search terms and call find for each one
//     for (i = 2; i < argc; i++) {
//       find(argv[1], argv[i]);
//     }
//   }
//   exit(0);
// }

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *
fmtname(char *path)
{
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

void finddir(char *path, char *buf, int fd, struct stat st, char *search)
{
    char *p;
    struct dirent de;
    struct stat st2;

    if (strlen(path) + 1 + DIRSIZ + 1 > 512)
    {
        printf("find: path too long\n");
    }
    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if (de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if (stat(buf, &st) < 0)
        {
            printf("find: cannot stat %s\n", buf);
            continue;
        }
        if (st.type == T_DIR)
        {
            if (strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0)
            {
                int fd2 = open(buf, 0);
                if (fstat(fd, &st2) < 0)
                {
                    fprintf(2, "find: cannot stat %s\n", path);
                    close(fd);
                    return;
                }
                finddir(buf, buf, fd2, st2, search);
                close(fd2);
            }
        }
        else if (st.type == T_FILE)
        {
            if (strcmp(fmtname(buf), search) == 0)
            {
                printf("%s\n", buf);
            }
        }
    }
}
void find(char *path, char *search)
{
    char buf[512];
    int fd;
    struct stat st;

    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type == T_DIR)
    {
        finddir(path, buf, fd, st, search);
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc < 2)
    {
        printf("Usage: find <path> <search>\n");
    }
    else if (argc == 2)
    {
        find(".", argv[1]);
    }
    else
    {
        for (i = 2; i < argc; i++)
        {
            find(argv[1], argv[i]);
        }
    }
    exit(0);
}