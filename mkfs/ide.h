#ifndef _IDE_H_
#define _IDE_H_

#include "stdint.h"
#include <unistd.h>

struct Ide {
    int drive;

    Ide(int drive) : drive(drive) {}

    ssize_t read(off_t offset, void* buffer, size_t n);
    ssize_t write(off_t offset, const void* buffer, size_t n);


    void readAll(off_t offset, void* buffer, size_t n);
    void writeAll(off_t offset, const void* buffer, size_t n);
};

#endif
