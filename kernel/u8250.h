#ifndef _U8250_H_
#define _U8250_H_

/* 8250 */

#include "io.h"
#include "file.h"
#include "refs.h"

class U8250 : public OutputStream<char> {
public:
    U8250() { }
    virtual void put(char ch);
    virtual char get();
};

class U8250File : public File {
    U8250 *it;
public:
    U8250File(U8250* it) : it(it) {}
    bool isFile() override { return true; }
    bool isDirectory() override { return false; }
    off_t seek(off_t offset) { return offset; }
    off_t size() { return 0x7FFFFFFF; }
    ssize_t readAll(uint32_t offset, void* buffer, size_t n) {
        for(uint32_t i = 0; i < n; i ++)
        {
            *(((char*)buffer) + i) = it->get();
        }
        return n;        
    }
    ssize_t writeAll(uint32_t offset, void* buffer, size_t n) {
        for(uint32_t i = 0; i < n; i ++)
        {
            it->put(((char*)buffer)[i]);
        }
        return n;
    }
    bool std()
    {
        return true;
    }
};


#endif
