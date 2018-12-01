#ifndef _BOBFS_H_
#define _BOBFS_H_

#include "refs.h"
#include "ide.h"
#include "hashmap.h"
#include "u8250.h"

class BobFS;
class Node;


class Bitmap {
    BobFS* fs;
    uint32_t offset;
public:
    Bitmap(BobFS *fs, uint32_t offset);
    void clear(void);
    void set(int32_t index);
    void clear(int32_t index);
    int32_t find(void);
};


class Node {
    StrongPtr<BobFS> fs;
    uint32_t inumber;
    uint32_t offset;
    uint32_t getBlockNumber(uint32_t blockIndex);
    InterruptSafeLock NodeLock;

public:
    static constexpr uint32_t SIZE = 16;
    static constexpr uint16_t DIR_TYPE = 1;
    static constexpr uint16_t FILE_TYPE = 2;

    Node(StrongPtr<BobFS> fs, uint32_t inumber);

    uint16_t getType(void);
    uint16_t getLinks(void);
    uint32_t getSize(void);
    uint32_t getDirect(void);
    uint32_t getIndirect(void);
    uint32_t getInum(void) const;

    void setType(uint16_t type);
    void setLinks(uint16_t type);
    void setSize(uint32_t type);
    void setDirect(uint32_t type);
    void setIndirect(uint32_t type);

    int32_t read(uint32_t offset, void* buffer, uint32_t n);
    int32_t readAll(uint32_t offset, void* buffer, uint32_t n);

    int32_t write(uint32_t offset, const void* buffer, uint32_t n);
    int32_t writeAll(uint32_t offset, const void* buffer, uint32_t n);

    StrongPtr<Node> newNode(const char* name, uint32_t type);
    StrongPtr<Node> newFile(const char* name);
    StrongPtr<Node> newDirectory(const char* name);
    StrongPtr<Node> findNode(const char* name);

    bool isFile(void);
    bool isDirectory(void);

    void linkNode(const char* name, StrongPtr<Node> file);

    void dump(const char* name);

    static StrongPtr<Node> get(StrongPtr<BobFS> fs, uint32_t index) {
        StrongPtr<Node> n { new Node(fs,index) };
        return n;
    }
};


class BobFS {
    StrongPtr<Ide> device;
    uint32_t dataBitmapBase;
    uint32_t inodeBitmapBase;
    uint32_t inodeBase;
    uint32_t dataBase;
    Bitmap *inodeBitmap;
    Bitmap *dataBitmap;

    uint32_t allocateBlock(void);
public:

    static constexpr uint32_t BLOCK_SIZE = 1024;

    BobFS(StrongPtr<Ide> device);
    virtual ~BobFS();
    static StrongPtr<BobFS> mkfs(StrongPtr<Ide> device);
    static StrongPtr<BobFS> mount(StrongPtr<Ide> device);

    static StrongPtr<Node> root(StrongPtr<BobFS> fs);
    static StrongPtr<Node> find(StrongPtr<BobFS> fs, const char* path);

    friend class Node;
    friend class Bitmap;
};

typedef struct g_FS
{
    StrongPtr<BobFS> fs;
    StrongPtr<Node> root;
    Hashmap<int32_t, StrongPtr<File>> openFiles;
    g_FS(StrongPtr<BobFS>fs) : fs(fs), root(BobFS::root(fs)) {}
}g_FS;

#endif
