#include "debug.h"
#include "libk.h"
#include "machine.h"
#include "smp.h"
#include "config.h"
#include "atomic.h"

OutputStream<char> *Debug::sink = 0;
bool Debug::debugAll = false;

void Debug::init(OutputStream<char> *sink) {
    Debug::sink = sink;
}

static SpinLock lock;

void Debug::vprintf(const char* fmt, va_list ap) {
    if (sink) {
        lock.lock();
        K::vsnprintf(*sink,1000,fmt,ap);
        lock.unlock();
    }
}

void Debug::printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt,ap);
    va_end(ap);
}

SpinLock silent;

void Debug::vpanic(const char* fmt, va_list ap) {
    silent.lock();
    vprintf(fmt,ap);
    printf("\n| processor %d halting\n*** 2000000000 shutdown\n",SMP::me());
    outb(0xf4,0x00);
    while (true) {
        asm volatile ("hlt");
    }
}

void Debug::panic(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vpanic(fmt,ap);
    va_end(ap);
}

void Debug::vdebug(const char* fmt, va_list ap) {
    if (debugAll || flag) {
       printf("[%s] ",what);
       vprintf(fmt,ap);
       printf("\n");
    }
}

void Debug::debug(const char* fmt, ...) {
    if (debugAll || flag) {
        va_list ap;
        va_start(ap, fmt);
        vdebug(fmt,ap);
        va_end(ap);
    }
}

void Debug::missing(const char* file, int line) {
    panic("*** Missing code at %s:%d",file,line);
    shutdown();
}
