#ifndef _future_h_
#define _future_h_

#include "atomic.h"
#include "semaphore.h"

template <typename T>
class Future {
    Semaphore isReady;
    T value;
public:
    Future() : isReady(0), value() {
    }
    void set(T value) {
        this->value = value;
        isReady.up();
    }
    T get() {
        isReady.down();
        isReady.up();
        return value;
    }
};

#endif
