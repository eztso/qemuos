#ifndef _VMM_H_
#define _VMM_H_

#include "stdint.h"
#include "atomic.h"
#include "mutex.h"


// The virtual memory interface
namespace VMM {
    constexpr uint32_t FRAME_SIZE = (1 << 12);

    // Called to initialize the available physical memory pool
    void init(uint32_t start, uint32_t size);

    /* how many page faults occured so far
          the counter sohuld be updated atomically
          the exact value will depend on event ordered
     */
    uint32_t pageFaultCount();

    /* allocate a frame
           nullptr -> no frames availabe
           always 4K aligned
           zero-filled
     */
    uint32_t alloc();

    /* free a frame */
    void free(uint32_t);
};

class AddressSpace {
    uint32_t *pd;
    Mutex lock;
    uint32_t& getPTE(uint32_t va);
public:
    // Present Bit
    static constexpr uint32_t P = 1;
 
    // Write bit
    static constexpr uint32_t W = 2;

    // User bit
    static constexpr uint32_t U = 4;

    AddressSpace(bool isShared = false);
    virtual ~AddressSpace();

    // Enter a mapping in this address space
    void pmap(uint32_t va, uint32_t pa, bool forUser, bool forWrite);

    // Handle a page fault for the given virtual address
    void handlePageFault(uint32_t va);

    // Make this address space active, should be called when context-switching
    void activate();

    uint32_t* getPD() { return this->pd; }

    static bool compareAddressSpace(AddressSpace* child, AddressSpace* parent);
};

struct VMMNode {
    VMMNode* next;
};

struct VMMInfo {
    VMMNode *firstFree;
    uint32_t avail;
    uint32_t limit;
    AddressSpace* sharedAddressSpace;
    InterruptSafeLock vmmLock;
};

#endif
