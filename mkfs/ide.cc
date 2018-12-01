#include "ide.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>

ssize_t Ide::read(off_t offset, void* buffer, size_t n) {

    auto off = lseek(drive,offset,SEEK_SET);
    if (off == -1) {
        perror("Ide::read lseek");
        exit(-1);
    }

    ssize_t cnt = ::read(drive,buffer,n);
    if (cnt < 0) {
        perror("");
        printf("%s:%d failed to read %ld @ %ld\n",__FILE__,__LINE__,(long)n,(long)offset);
        exit(-1);
    }
    return n;
}

void Ide::readAll(off_t offset, void* buffer_, size_t n) {

    char* buffer = (char*) buffer_;

    while (n > 0) {
        ssize_t cnt = read(offset,buffer,n);
        if (cnt < 0) {
            perror("");
            printf("%s:%d failed to read %ld @ %ld\n",__FILE__,__LINE__,(long)n, (long)offset);
            exit(-1);
        }
        if (cnt == 0) {
            bzero(buffer,n);
            return;
        }

        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
}

ssize_t Ide::write(off_t offset, const void* buffer, size_t n) {
    auto off = lseek(drive,offset,SEEK_SET);
    if (off == -1) {
        perror("Ide::read lseek");
        exit(-1);
    }
    return ::write(drive,buffer,n);
}

void Ide::writeAll(off_t offset, const void* buffer_, size_t n) {

    const char* buffer = (const char*) buffer_;

    while (n > 0) {
        ssize_t cnt = write(offset,buffer,n);
        if (cnt <= 0) {
            printf("%s:%d failed to write\n",__FILE__,__LINE__);
            exit(-1);
        }

        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
}

