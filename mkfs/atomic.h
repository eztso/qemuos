#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <stdint.h>


template <typename T>
class AtomicPtr {
    volatile T *ptr;
public:
    AtomicPtr() : ptr(nullptr) {}
    AtomicPtr(T *x) : ptr(x) {}
    AtomicPtr<T>& operator= (T v) {
        __atomic_store_n(ptr,v,__ATOMIC_SEQ_CST);
        return *this;
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(ptr,inc,__ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(ptr,inc,__ATOMIC_SEQ_CST);
    }
    void add(T inc) {
        fetch_add(inc);
    }
    void set(T inc) {
        return __atomic_store_n(ptr,inc,__ATOMIC_SEQ_CST);
    }
    T get(void) const {
        return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
        __atomic_exchange(ptr,&v,&ret,__ATOMIC_SEQ_CST);
        return ret;
    }
};

template <typename T>
class Atomic {
    volatile T value;
public:
    Atomic(T x) : value(x) {}
    Atomic<T>& operator= (T v) {
        __atomic_store_n(&value,v,__ATOMIC_SEQ_CST);
        return *this;
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(&value,inc,__ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(&value,inc,__ATOMIC_SEQ_CST);
    }
    void add(T inc) {
        fetch_add(inc);
    }
    void set(T inc) {
        return __atomic_store_n(&value,inc,__ATOMIC_SEQ_CST);
    }
    T get(void) const {
        return __atomic_load_n(&value,__ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
        __atomic_exchange(&value,&v,&ret,__ATOMIC_SEQ_CST);
        return ret;
    }
};

#endif
