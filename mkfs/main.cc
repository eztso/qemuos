#include "bobfs.h"

#include "ide.h"
#include "refs.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

class Free {
    void* ptr;
public:
    Free(void* ptr) : ptr(ptr) {}
    ~Free() { if (ptr != nullptr) free(ptr); }
};

void one(StrongPtr<Node> bobDir, const char* dirName, DIR* dir) {
    while (true) {
        auto e = readdir(dir);
        if (e == nullptr) break;
        if (strcmp(e->d_name,".") == 0) continue;
        if (strcmp(e->d_name,"..") == 0) continue;

        char* newName = (char*)malloc(strlen(dirName) + strlen(e->d_name) + 2);
        Free freeIt(newName);
        sprintf(newName,"%s/%s",dirName,e->d_name);

        if (e->d_type == DT_REG) {
            printf("F %s\n",newName);
            auto outFile = bobDir->newFile(e->d_name);

            off_t outOffset = 0;

            char buffer[1024];
            int fd = open(newName,O_RDONLY);
            if (fd == -1) {
                perror("open file");
                exit(-1);
            }
            while (true) {
                ssize_t n = read(fd,buffer,sizeof(buffer));
                if (n < 0) {
                    perror("read");
                    exit(-1);
                }
                if (n == 0) break;

                outFile->writeAll(outOffset,buffer,n);
                outOffset += n;
            }
        } else if (e->d_type == DT_DIR) {
            printf("D %s\n",newName);
            auto newDir = bobDir->newDirectory(e->d_name);
            auto dd = opendir(newName);
            if (dd == nullptr) {
                perror("opening sub-directory\n");
                exit(-1);
            }
            one(newDir,newName,dd);
            closedir(dd);
        }
    }
}

int main(int argc, const char* argv[]) {
    if (argc != 3) {
        fprintf(stderr,"usage: %s <outfile> <rootDir>\n",argv[0]);
        exit(-1);
    }

    int fd = open(argv[1],O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd == -1) {
        perror("create output file");
        exit(-1);
    }

    StrongPtr<Ide> drive { new Ide(fd) };
    auto fs = BobFS::mkfs(drive);

    DIR* dir = opendir(argv[2]);
    if (dir == 0) {
        perror("can't open directory");
        exit(-1);
    }

    auto root = BobFS::root(fs);

    one(root,argv[2],dir);

    return 0;
}
