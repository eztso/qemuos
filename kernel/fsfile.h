#ifndef FSFILE_H
#define FSFILE_H

#include "file.h"
#include "refs.h"
#include "bobfs.h"
class FsFile : public File {
    StrongPtr<Node> file;
public:
    FsFile(StrongPtr<Node> file) : file(file) {}
    bool isFile() override { return file->isFile(); }
    bool isDirectory() override { return file->isDirectory(); }
    off_t seek(off_t offset) { return offset; }
    off_t size() { return file->getSize(); }

    ssize_t readAll(uint32_t offset, void* buffer, size_t n) {
        uint32_t bytesRead = file->readAll(offset, buffer, n);
        return bytesRead;
    }
    ssize_t writeAll(uint32_t offset, void* buffer, size_t n) {
        uint32_t bytesWritten = file->writeAll(offset, buffer, n);
        return bytesWritten;
    }
    bool std()
    {
        return false;
    }
};


#endif