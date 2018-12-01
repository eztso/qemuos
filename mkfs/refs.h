#ifndef _REFS_H_
#define _REFS_H_

#include <cstdio>
#include "atomic.h"

template <typename T>
struct PtrState {
    Atomic<int> count;
    T* ptr;

    PtrState(T* ptr) : count(1), ptr(ptr) {
    }
};

template <typename T>
class StrongPtr {
    Atomic<PtrState<T>*> state;

    void ref(PtrState<T>* s) {
        if (s != nullptr) {
            s->count.add(1);
        }

        PtrState<T>* old = state.exchange(s);
       
        if (old != nullptr) {
            int x = old->count.fetch_add(-1);
            if (x == 1) {
                delete old->ptr;
                old->ptr = nullptr;
                delete old;
            }
        }
    }

public:
    StrongPtr() : state(nullptr) {
    }

    explicit StrongPtr(T* ptr) : state((ptr == nullptr) ? nullptr : new PtrState<T>(ptr)) {
    }

    StrongPtr(const StrongPtr<T>& src) : state(nullptr) {
        ref(src.state.get());
    }

    ~StrongPtr() {
        ref(nullptr);
    }

    void show() const {
        printf("*** state = %x, count = %d\n",state.get(), (state.get() == nullptr) ? -1 : state.get()->count);
    }

    T* operator -> ()  { return state.get()->ptr; }

    T& operator * () { return *(state.get()->ptr); }

    bool isNull() const { return state.get() == nullptr; }

    void reset() {
        ref(nullptr);
    }

    StrongPtr<T>& operator = (const StrongPtr& src) {
        ref(src.state.get());
        return *this;
    }

    bool operator ==(const StrongPtr<T>& other) const {
        return state.get() == other.state.get();
    }

    bool operator !=(const StrongPtr<T>& other) const {
        return state.get() != other.state.get();
    }
};

#endif
