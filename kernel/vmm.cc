#include "vmm.h"
#include "machine.h"
#include "idt.h"
#include "libk.h"
#include "mutex.h"
#include "config.h"
#include "threads.h"
#include "atomic.h"
#include "debug.h"

VMMInfo *info = nullptr;

void VMM::init(uint32_t start, uint32_t size) {
    Debug::printf("| physical range 0x%x 0x%x\n",start,start+size);
    info = new VMMInfo;
    info->avail = start;
    info->limit = start + size;
    info->firstFree = nullptr;

    // TODO: add needed locks
    // MISSING();

    // must be last
    info->sharedAddressSpace = new AddressSpace(true);

    /* register the page fault handler */
    IDT::trap(14,(uint32_t)pageFaultHandler_,3);
}


uint32_t VMM::alloc() {
	InterruptSafeLocker vmmLocker(info->vmmLock);
    uint32_t p;

    if (info->firstFree != nullptr) {
        p = (uint32_t) info->firstFree;
        info->firstFree = info->firstFree->next;
    } else {
        if (info->avail == info->limit) {
            Debug::panic("no more frames");
        }
        p = info->avail;
        info->avail += FRAME_SIZE;
    }

    bzero((void*)p,FRAME_SIZE);


    return p;
}

void VMM::free(uint32_t p) {
	InterruptSafeLocker vmmLocker(info->vmmLock);
    VMMNode* n = (VMMNode*) p;    
    n->next = info->firstFree;
    info->firstFree = n;

}

/****************/
/* AddressSpace */
/****************/

void AddressSpace::activate() {
    vmm_on((uint32_t)pd);
}

AddressSpace::AddressSpace(bool isShared) : lock() {
    pd = (uint32_t*) VMM::alloc();
    if (isShared) {
        for (uint32_t va = VMM::FRAME_SIZE;
            va < info->limit;
            va += VMM::FRAME_SIZE
        ) {
            pmap(va,va,false,true);
        }
        pmap(kConfig.localAPIC,kConfig.localAPIC,false,true);
        pmap(kConfig.ioAPIC,kConfig.ioAPIC,false,true);
        for (int i=0; i<512; i++) {
            uint32_t pde = pd[i];
            if (pde == 0) {
                pd[i] = VMM::alloc() | U | W | P;
            }
        }
        for (int i = (0xf << 6); i < 1024; i++) {
            uint32_t pde = pd[i];
            if (pde == 0) {
                pd[i] = VMM::alloc() | U | W | P;
            }
        }
    } else {
        for (int i=0; i<1024; i++) {
            pd[i] = info->sharedAddressSpace->pd[i];
        }
    }
}

AddressSpace::~AddressSpace() {
    for (int i0 = 0; i0 < 1024; i0++) {
        if (info->sharedAddressSpace->pd[i0] != 0) continue;

        uint32_t pde = pd[i0];
        if (pde & P) {
            uint32_t *pt = (uint32_t*) (pde & 0xfffff000);
            for (uint32_t i1 = 0; i1 < 1024; i1++) {
                uint32_t pte = pt[i1];
                if (pte & P) {
                    uint32_t pa = pte & 0xfffff000;
                    VMM::free(pa);
                }
            }
            VMM::free((uint32_t) pt);
        }
    }
    VMM::free((uint32_t) pd);
}

/* precondition: table is locked */
uint32_t& AddressSpace::getPTE(uint32_t va) {
    uint32_t i0 = (va >> 22) & 0x3ff;
    if ((pd[i0] & P) == 0) {
        pd[i0] = VMM::alloc() | 7; /* UWP */
    }
    uint32_t* pt = (uint32_t*) (pd[i0] & 0xfffff000);
    return pt[(va >> 12) & 0x3ff];
}

void AddressSpace::pmap(uint32_t va, uint32_t pa, bool forUser, bool forWrite) {
    invlpg(va);
    uint32_t &pte = getPTE(va);

    if ((pte & 1) == 0) 
        pte = (pa & 0xfffff000) | (forUser ? U : 0) | (forWrite ? W : 0) | 1;


}

void AddressSpace::handlePageFault(uint32_t va_) {
    uint32_t va = (va_ / VMM::FRAME_SIZE) * VMM::FRAME_SIZE;

    lock.lock();

    bool forUser = (va >= 0x80000000) && (va < 0xF0000000);

    if ((getPTE(va) & 1)  == 0) {
        uint32_t pa = VMM::alloc();
        pmap(va,pa,forUser,true);
    }

    lock.unlock();

}

InterruptSafeLock pfLock;
extern "C" void vmm_pageFault(uintptr_t va, uintptr_t *saveState) {
    //Debug::printf("| page fault @ %x\n",va);
	InterruptSafeLocker pfLocker(pfLock);
    active()->threadPCB->addressSpace->handlePageFault(va);
}

bool AddressSpace::compareAddressSpace(AddressSpace* child, AddressSpace* parent) {
    for (int i = 512; i < 960; ++i) {
        auto childPDE = (uint32_t*) (child->pd[i] & 0xfffff000);
        auto parentPDE = (uint32_t*) (parent->pd[i] & 0xfffff000);
        if((child->pd[i] % 2 == 1) && (parent->pd[i] % 2 == 1)) {
            for (uint32_t j = 0; j < 1024; ++j) {
                auto childPTE = (uint32_t *) (childPDE[j] & 0xfffff000);
                auto parentPTE = (uint32_t *) (parentPDE[j] & 0xfffff000);
                uint32_t vaToMap = (i << 22) | (j << 12);
                if ((childPDE[j] % 2 == 1) && (parentPDE[j] % 2 == 1)) {
                    for (uint32_t k = 0; k < 1024; ++k) {
                        if (childPTE[k] != parentPTE[k]) {
                            Debug::panic(
                                    "VM copying error child value at VA %x. Child value is %d and parent value is %d\n",
                                    vaToMap, childPTE[k], parentPTE[k]);
                            return false;
                        }
                    }
                } else if ((childPDE[j] % 2 == 1) != (parentPDE[j] % 2 == 1))
                    Debug::panic("valid bits don't match at PT level \n");
            }
        } else if((child->pd[i] % 2 == 1) != (parent->pd[i] % 2 == 1))
            Debug::panic("valid bits don't match at PD level \n");
    }
    Debug::printf("Address space match\n");
    return true;

}