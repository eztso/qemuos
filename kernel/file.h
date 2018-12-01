#ifndef _FILE_H_
#define _FILE_H_

#include "stdint.h"

class File {
public:
    virtual ~File() {}
    virtual bool isFile() = 0;
    virtual bool isDirectory() = 0;
    virtual off_t size() = 0;
    virtual off_t seek(off_t offset) = 0;
    virtual ssize_t readAll(uint32_t offset, void* buf, size_t size) = 0;
    virtual ssize_t writeAll(uint32_t offset, void* buf, size_t size) = 0;
    virtual bool std() = 0;
};

#endif
