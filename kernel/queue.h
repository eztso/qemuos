#ifndef _queue_h_
#define _queue_h_

#include "atomic.h"

template <typename T>
class Queue {
    T* first = nullptr;
    T* last = nullptr;
    InterruptSafeLock lock;
public:
    Queue() : first(nullptr), last(nullptr), lock() {}

    void add(T* t) {
        InterruptSafeLocker x(lock);
        t->next = nullptr;
        if (first == nullptr) {
            first = t;
        } else {
            last->next = t;
        }
        last = t;
    }

    void addFront(T* t) {
        InterruptSafeLocker x(lock);
        t->next = first;
        first = t;
        if (last == nullptr) last = first;
    }

    T* remove() {
        InterruptSafeLocker x(lock);
        if (first == nullptr) {
            return nullptr;
        }
        auto it = first;
        first = it->next;
        if (first == nullptr) {
            last = nullptr;
        }
        return it;
    }
};

#endif
