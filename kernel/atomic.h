#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include "machine.h"

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
    operator T () const {
        return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(ptr,inc,__ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(ptr,inc,__ATOMIC_SEQ_CST);
    }
    void set(T inc) {
        return __atomic_store_n(ptr,inc,__ATOMIC_SEQ_CST);
    }
    T get (void) const {
        return __atomic_load_n(ptr,__ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
         
        return ret;
    }
};

template <typename T>
class Atomic {
    volatile T value;
public:
	Atomic() : value() {}
    Atomic(T x) : value(x) {}
    Atomic<T>& operator= (T v) {
        __atomic_store_n(&value,v,__ATOMIC_SEQ_CST);
        return *this;
    }
    operator T () const {
        return __atomic_load_n(&value,__ATOMIC_SEQ_CST);
    }
    T fetch_add(T inc) {
        return __atomic_fetch_add(&value,inc,__ATOMIC_SEQ_CST);
    }
    T add_fetch(T inc) {
        return __atomic_add_fetch(&value,inc,__ATOMIC_SEQ_CST);
    }
    void set(T inc) {
        return __atomic_store_n(&value,inc,__ATOMIC_SEQ_CST);
    }
    T get (void) const {
        return __atomic_load_n(&value,__ATOMIC_SEQ_CST);
    }
    T exchange(T v) {
        T ret;
        __atomic_exchange(&value,&v,&ret,__ATOMIC_SEQ_CST);
        return ret;
    }
};


class SpinLock {
    Atomic<bool> taken;
public:
    SpinLock() : taken(false) {}
    void lock(void) {
        while (taken.exchange(true));
    }
    void unlock(void) {
        taken.set(false);
    }
};

/* Why is this a bad idea? Why am I doing it anyway? */
class InterruptSafeLock {
    Atomic<bool> taken;
public:
    InterruptSafeLock() : taken(false) {};
    bool lock();
    void unlock(bool was);
    template <typename F> void doit(F f) {
        auto was = disable();
        f();
        enable(was);
    }
};

class InterruptSafeLocker {
    InterruptSafeLock& theLock;
    bool was;
public:
    InterruptSafeLocker(InterruptSafeLock& it) : theLock(it),was(it.lock()) {}
    ~InterruptSafeLocker() { theLock.unlock(was); }
};
    

class Barrier {
public:
    Atomic<uint32_t> expected;
    Barrier(uint32_t n) : expected(n) {}
    void sync(void) {
        uint32_t a = expected.add_fetch(-1);
        while (a != 0) {
            a = expected.get();
        }
    }
};

#endif
