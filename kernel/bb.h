#ifndef _BB_H_
#define _BB_H_

#include "stdint.h"
#include "mutex.h"
#include "semaphore.h"

template <typename T, uint32_t N>
class BB {
    T data[N];
    uint32_t head = 0;
    uint32_t tail = 0;
    Mutex mutex;
    Semaphore nEmpty { N };
    Semaphore nFull { 0 };
public:
    T get(void) {
        nFull.down();
        mutex.lock();
        T v = data[head];
        head = (head + 1) % N;
        mutex.unlock();
        nEmpty.up();
        return v;
    }

    void put(T v) {
        nEmpty.down();
        mutex.lock();
        data[tail] = v;
        tail = (tail + 1) % N;
        mutex.unlock();
        nFull.up();
    }
};
        


#endif
