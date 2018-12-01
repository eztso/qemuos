#include "semaphore.h"

#include "threads.h"
#include "queue.h"
#include "atomic.h"

Semaphore::Semaphore(uint32_t count) : count(count), lock(), waiting() {
}

void Semaphore::up(void) {
    bool was = lock.lock();
    auto t = waiting.remove();
    if (t != nullptr) {
        lock.unlock(was);
        schedule(t);
    } else {
        count ++;
        lock.unlock(was);
    }
}

void Semaphore::down(void) {
    bool was = lock.lock();

    if (count == 0) {
        auto me = active();
        me->leaveMeAlone = 1;
        waiting.add(me);
        lock.unlock(was);
        block(me);
    } else {
        count --;
        lock.unlock(was);
    }
}
