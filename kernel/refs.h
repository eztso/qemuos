#ifndef _REFS_H_
#define _REFS_H_

#include "machine.h"
#include "debug.h"
#include "atomic.h"

class ReferenceCounter {
    int refCount;
public:
    ReferenceCounter(): refCount(0) {};
    int dec() { return --refCount; }
    int inc() { return ++refCount; }
    int count() { return refCount; }
};



template <typename T>
class StrongPtr {
    T* ptr;
    ReferenceCounter* ref;
public:
    StrongPtr() : ptr(nullptr), ref(nullptr) {}

    explicit StrongPtr(T* src_ptr) : ptr(src_ptr), ref(new ReferenceCounter()) {
        ref->inc();
    }

    StrongPtr(const StrongPtr& src) : ptr(src.ptr), ref(src.ref){
        ref->inc();
    }

    StrongPtr(StrongPtr&& src) : ptr(src.ptr), ref(src.ref){
        src.ptr = nullptr;
        src.ref = nullptr;
        //ref->inc();
        // if(src.ref != nullptr)
        // {
        //  if(src.ref->dec() <= 0)
        //  {
        //      delete src.ref;
        //      src.ref = nullptr;
        //      delete src.ptr;
        //      src.ptr = nullptr;
        //  }
        // }
    }

    ~StrongPtr() {
        if(ref != nullptr)
        {
            if(ref->dec() <= 0)
            {
                delete ref;
                ref = nullptr;
                delete ptr;
                ptr = nullptr;
            }
        }
    }

    T* operator -> ()  {
        return ptr;
    }

    bool isNull() const {
        return ptr == nullptr;
    }

    void reset() {
        if(ref != nullptr)
        {
            if(ref->dec() == 0)
            {
                delete ref;
                delete ptr;
            }
            ptr = nullptr;
            ref = nullptr;
        }
    }
    
    // this will make a good test case
    StrongPtr<T>& operator = (const StrongPtr& src) {
        if(this == &src) return *this;
        if(ref != nullptr)
        {
            if(ref->dec() == 0)
            {
                delete ref;
                delete ptr;
            }
        }
        ptr = src.ptr;
        ref = src.ref;
        ref->inc();
        return *this;
    }

    bool operator ==(const StrongPtr<T>& other) const {
        return ( this->ptr == other.ptr );
    }

    bool operator !=(const StrongPtr<T>& other) const {
        return ( this->ptr != other.ptr );
    }
};

#endif
