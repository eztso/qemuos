#include "bobfs.h"

#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <strings.h>

// Minimal BobFS

uint8_t zero_1024[BobFS::BLOCK_SIZE];

////////////
// Bitmap //
////////////

Bitmap::Bitmap(BobFS* fs, uint32_t offset) :
    fs(fs), offset(offset)
{
}

void Bitmap::clear(void) {
    fs->device->writeAll(offset,zero_1024,BobFS::BLOCK_SIZE);
}

void Bitmap::set(int32_t index) {
    uint32_t wordOffset = (index/32)*4;
    uint32_t bitIndex = index%32;

    uint32_t word;
 
    fs->device->readAll(offset+wordOffset,&word,sizeof(word));

    word |= (1 << bitIndex);

    fs->device->writeAll(offset+wordOffset,&word,sizeof(word));
}

void Bitmap::clear(int32_t index) {
    uint32_t wordOffset = (index/32) * 4; 
    uint32_t bitIndex = index%32;

    uint32_t word;
 
    fs->device->readAll(offset+wordOffset,&word,sizeof(word));

    word &= ~(1 << bitIndex);

    fs->device->writeAll(offset+wordOffset,&word,sizeof(word));
}

int32_t Bitmap::find(void) {
    for (uint32_t i=0; i<BobFS::BLOCK_SIZE; i += 4) {
        uint32_t word;
        fs->device->readAll(offset+i,&word,sizeof(word));
        if ((~word) != 0) {
            uint32_t mask = 1;
            for (uint32_t j=0; j<32; j++) {
                if ((word & mask) == 0) {
                    word |= mask;
                    fs->device->writeAll(offset+i,&word,sizeof(word));
                    return i * 8 + j;
                }
                mask = mask * 2;
            }
        }
    }
    return -1;
}

//////////
// Node //
//////////

Node::Node(StrongPtr<BobFS> fs, uint32_t inumber) : fs(fs), inumber(inumber) {
    offset = fs->inodeBase + inumber * SIZE;
}

uint16_t Node::getType(void) {
    uint16_t x;
    fs->device->readAll(offset+0, &x, 2);
    return x;
}

bool Node::isDirectory(void) {
    return getType() == DIR_TYPE;
}

bool Node::isFile(void) {
    return getType() == FILE_TYPE;
}

bool streq(const char* a, const char* b) {
    int i = 0;

    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

StrongPtr<Node> Node::findNode(const char* name) {
    uint32_t sz = getSize();
    uint32_t offset = 0;

    while (offset < sz) {
        uint32_t ichild;
        readAll(offset,&ichild,4);
        offset += 4;
        uint32_t len;
        readAll(offset,&len,4);
        offset += 4;
        char* ptr = (char*) malloc(len+1);
        readAll(offset,ptr,len);
        offset += len;
        ptr[len] = 0;

        auto cmp = streq(name,ptr);
        free(ptr);

        if (cmp) {
            StrongPtr<Node> child = Node::get(fs,ichild);
            return child;
        }
    }

    StrongPtr<Node> nothing;
    return nothing;
}
    

void Node::setType(uint16_t type) {
    fs->device->writeAll(offset+0, &type, 2);
}

uint16_t Node::getLinks(void) {
    uint16_t x;
    fs->device->readAll(offset+2, &x, 2);
    return x;
}

void Node::setLinks(uint16_t links) {
    fs->device->writeAll(offset+2, &links, 2);
}

uint32_t Node::getSize(void) {
    uint32_t x;
    fs->device->readAll(offset+4, &x, 4);
    return x;
}

void Node::setSize(uint32_t size) {
    fs->device->writeAll(offset+4, &size, 4);
}

uint32_t Node::getDirect(void) {
    uint32_t x;
    fs->device->readAll(offset+8, &x, 4);
    return x;
}

void Node::setDirect(uint32_t direct) {
    fs->device->writeAll(offset+8, &direct, 4);
}

uint32_t Node::getIndirect(void) {
    uint32_t x;
    fs->device->readAll(offset+12, &x, 4);
    return x;
}

void Node::setIndirect(uint32_t indirect) {
    fs->device->writeAll(offset+12, &indirect, 4);
}

uint32_t Node::getBlockNumber(uint32_t blockIndex) {
    if (blockIndex == 0) {
        uint32_t x = getDirect();
        if (x == 0) {
            x = fs->allocateBlock();
            setDirect(x);
        }
        return x;
    } else {
        blockIndex -= 1;
        if (blockIndex >= BobFS::BLOCK_SIZE/4) return 0;
        uint32_t i = getIndirect();
        if (i == 0) {
            i = fs->allocateBlock();
            if (i == 0) return 0;
            setIndirect(i);
        }
        uint32_t x;
        const uint32_t xOffset = i * BobFS::BLOCK_SIZE + blockIndex*4;
        fs->device->readAll(xOffset,&x,sizeof(x));
        if (x == 0) {
            x = fs->allocateBlock();
            fs->device->writeAll(xOffset,&x,sizeof(x));
        }
        return x;
    }
}
            
ssize_t Node::write(uint32_t offset, const void* buffer, uint32_t n) {
    uint32_t blockIndex = offset / BobFS::BLOCK_SIZE;
    uint32_t start = offset % BobFS::BLOCK_SIZE;
    uint32_t end = start + n;
    if (end > BobFS::BLOCK_SIZE) end = BobFS::BLOCK_SIZE;
    uint32_t count = end - start;

    uint32_t blockNumber = getBlockNumber(blockIndex);

    fs->device->writeAll(
               blockNumber*BobFS::BLOCK_SIZE+start,
               buffer,
               count);

    uint32_t newSize = offset + count;
    uint32_t oldSize = getSize();
    if (newSize > oldSize) {
        setSize(newSize);
    }
    return count;
}

void Node::writeAll(uint32_t offset, const void* buffer_, uint32_t n) {

    char* buffer = (char*) buffer_;

    while (n > 0) {
        int32_t cnt = write(offset,buffer,n);
        if (cnt <= 0) {
            printf("%s:%d failed to write\n",__FILE__,__LINE__);
            exit(-1);
        }

        n -= cnt;
        offset += cnt;
        buffer += cnt;
    }
}

ssize_t Node::read(uint32_t offset, void* buffer, uint32_t n) {
    uint32_t sz = getSize();
    if (sz <= offset) return 0;

    uint32_t remaining = sz - offset;
    if (remaining < n) n = remaining;

    uint32_t blockIndex = offset / BobFS::BLOCK_SIZE;
    uint32_t start = offset % BobFS::BLOCK_SIZE;
    uint32_t end = start + n;
    if (end > BobFS::BLOCK_SIZE) end = BobFS::BLOCK_SIZE;
    uint32_t count = end - start;

    uint32_t blockNumber = getBlockNumber(blockIndex);

    return fs->device->read(blockNumber*BobFS::BLOCK_SIZE+start, buffer, count);
}

void Node::readAll(uint32_t offset, void* buffer_, uint32_t n) {

    char* buffer = (char*) buffer_;

    while (n > 0) {
        int32_t cnt = read(offset,buffer,n);
        if (cnt < 0) {
            printf("%s:%d failed to read\n",__FILE__,__LINE__);
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

void Node::linkNode(const char* name, StrongPtr<Node> node) {
    node->setLinks(1+node->getLinks());
    uint32_t offset = getSize();
    writeAll(offset,&node->inumber,4);
    uint32_t len = strlen(name);
    writeAll(offset+4,&len,sizeof(len));
    writeAll(offset+4+sizeof(len),name,len);
}

StrongPtr<Node> Node::newNode(const char* name, uint32_t type) {
    int32_t idx = fs->inodeBitmap->find();
    if (idx < 0) {
        printf("newNode, out of inodes\n");
    } 
    StrongPtr<Node> node = Node::get(fs,idx);
    node->setType(type);
    node->setSize(0);
    node->setLinks(0);    
    node->setDirect(0);
    node->setIndirect(0);

    linkNode(name,node);
    return node;
}

StrongPtr<Node> Node::newFile(const char* name) {
    return newNode(name, FILE_TYPE);
}

StrongPtr<Node> Node::newDirectory(const char* name) {
    return newNode(name, DIR_TYPE);
}

void Node::dump(const char* name) {
    uint32_t type = getType();
    switch (type) {
    case DIR_TYPE:
        printf("*** 0 directory:%s(%d)\n",name,getLinks());
        {
            uint32_t sz = getSize();
            uint32_t offset = 0;

            while (offset < sz) {
                uint32_t ichild;
                readAll(offset,&ichild,4);
                offset += 4;
                uint32_t len;
                readAll(offset,&len,4);
                offset += 4;
                char* ptr = (char*) malloc(len+1);
                readAll(offset,ptr,len);
                offset += len;
                ptr[len] = 0;              
                
                StrongPtr<Node> child = Node::get(fs,ichild);
                child->dump(ptr);
                free(ptr);
            }
        }
        break;
    case FILE_TYPE:
        printf("*** 0 file:%s(%d,%d)\n",name,getLinks(),getSize());
        break;
    default:
         printf("unknown i-node type %d\n",type);
    }
}


///////////
// BobFS //
///////////

BobFS::BobFS(StrongPtr<Ide> device) :
    device(device),
    dataBitmapBase(BLOCK_SIZE),
    inodeBitmapBase(2 * BLOCK_SIZE),
    inodeBase(3 * BLOCK_SIZE),
    dataBase(3 * BLOCK_SIZE + Node::SIZE * BLOCK_SIZE * 8)
{
    dataBitmap = new Bitmap(this,dataBitmapBase);
    inodeBitmap = new Bitmap(this,inodeBitmapBase);
}

BobFS::~BobFS() {
    delete dataBitmap;
    delete inodeBitmap;
}

StrongPtr<Node> BobFS::root(StrongPtr<BobFS> fs) {
    uint32_t rootIndex;
    fs->device->readAll(8,&rootIndex,sizeof(rootIndex));
    return Node::get(fs,rootIndex);
}

uint32_t BobFS::allocateBlock(void) {
    int32_t index = dataBitmap->find();
    if (index == -1) {
        printf("failed to allocate block\n");
        exit(-1);
    }
    uint32_t blockIndex = dataBase / BLOCK_SIZE + index;
    device->writeAll(blockIndex * BLOCK_SIZE, zero_1024, BLOCK_SIZE);
    return blockIndex;
}

StrongPtr<BobFS> BobFS::mount(StrongPtr<Ide> device) {
    StrongPtr<BobFS> fs { new BobFS(device) };
    return fs;
}

StrongPtr<BobFS> BobFS::mkfs(StrongPtr<Ide> device) {
    device->writeAll(0,zero_1024,BLOCK_SIZE);
    device->writeAll(0,"BOBFS439",8);

    uint32_t root = 42;
    device->writeAll(8,&root,sizeof(root));    

    StrongPtr<BobFS> fs { new BobFS(device) };

    fs->inodeBitmap->clear();
    fs->inodeBitmap->set(root);
    fs->dataBitmap->clear();

    StrongPtr<Node> rootNode = Node::get(fs,root);

    rootNode->setType(Node::DIR_TYPE);
    rootNode->setSize(0);
    rootNode->setLinks(1);
    rootNode->setDirect(0);
    rootNode->setIndirect(0);

    return fs;
}

StrongPtr<Node> BobFS::find(StrongPtr<BobFS> fs, const char* path) {
    char* part = (char*) malloc(256);
    uint32_t idx = 0;
    auto current = root(fs);

    while (true) {
        while (path[idx] == '/') idx++;
        if (current.isNull() || (path[idx] == 0)) {
             free(part);
             return current;
        }
        uint32_t i = 0;
        while (true) {
            char c = path[idx];
            if ((c == 0) || (c == '/')) break;
            idx++;
            part[i++] = c;
        }
        part[i] = 0;
        current = current->findNode(part);
    }
}

