#ifndef _SEMAPHORE_H_
#define _SEMAPHORE_H_

#include "stdint.h"
#include "atomic.h"
#include "queue.h"

class Thread;

class Semaphore {
        uint32_t count;
        InterruptSafeLock lock;
	Queue<Thread> waiting;
public:
	Semaphore(const uint32_t count);
	void down();
	void up();
};


#endif

