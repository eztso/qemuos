#ifndef _EVENT_H_
#define _EVENT_H_

#include "semaphore.h"

class Event {
    Semaphore s { 0 };
public:
    void signal() {
        s.up();
    }
    void wait() {
        s.down();
        s.up();
    }
};

template <typename T>
class EventWithValue {
	Semaphore s { 0 };
	T v;
public:
	void signal(T v) {
		this->v = v;
		s.up();
	}
	T wait() {
		s.down();
		s.up();
		return v;
	}
};


#endif
