#include "debug.h"
#include "smp.h"

#include "threads.h"
#include "vmm.h"
#include "tss.h"

Thread::Thread() : addressSpace(new AddressSpace(false)) {
}

class InitialThread : public Thread {
public:
    InitialThread() { }
    virtual void start() override {
         Debug::printf("*** should never happen\n");
    }
    virtual uint32_t interruptEsp() override {
         return 0;
    }
};

Queue<Thread> readyQ;
Queue<Thread> zombieQ;
Thread** activeThreads;

void reaper() {
    Thread* firstAlone = nullptr;
    while(true) {
        auto th = zombieQ.remove();
        if (th == nullptr) return;
        if (th->leaveMeAlone) {
            if (th == firstAlone) return;
            if (firstAlone == nullptr) firstAlone = th;
            zombieQ.add(th);
        } else {
            delete th;
        }
    }
}

void threadsInit() {
    activeThreads = new Thread*[kConfig.totalProcs];
    for (uint32_t i=0; i<kConfig.totalProcs; i++) {
        activeThreads[i] = new InitialThread();
    }

    // The reaper
    thread([]() {
        while(true) {
            reaper();
            yield();
        }
    });
}


bool isDisabled() {
    uint32_t oldFlags = getFlags();
    return (oldFlags & 0x200) == 0;
}

bool disable() {
    bool wasDisabled = isDisabled();
    if (!wasDisabled)
        cli();
    return wasDisabled;
}

void enable(bool wasDisabled) {
    if (!wasDisabled)
        sti();
}

void schedule(Thread* t) {
    readyQ.add(t);
}

Thread* active() {
    Disable x;
    return activeThreads[SMP::me()];
}

void entry() {
    Thread* me = active();
    me->start();
    stop();
}

extern "C" void contextSwitch(uint32_t*,uint32_t*);

void block(Thread* me) {
again:
    auto next = readyQ.remove();

    if (next == me) {
        // It is possible for a thread to run into itself, can you see why?
        // block for something, get interrupted, the thing happens, get here
        me->leaveMeAlone = 0;
        return;
    }

    if (next == nullptr) goto again;

    if (next->leaveMeAlone) {
        readyQ.add(next);
        goto again;
    }
    
    next->leaveMeAlone = 1;

    {
        Disable x;
        activeThreads[SMP::me()] = next;
    }
 
    tss[SMP::me()].esp0 = next->interruptEsp();
    next->addressSpace->activate();
    contextSwitch(&me->ebx,&next->ebx);
}

void yield() {
    auto was = disable();
    auto me = activeThreads[SMP::me()];
    enable(was);
    if (me->leaveMeAlone) return;
    me->leaveMeAlone = 1;     // we check then we set with 
                              // interrupts enabled and no spin lock
                              // is this a race condition?
    readyQ.add(me);
    block(me);
}

void stop() {
    auto was = disable();
    auto me = activeThreads[SMP::me()];
    enable(was);
    me->leaveMeAlone = 1;
    zombieQ.add(me);
    block(me);
}
