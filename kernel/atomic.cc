#include "atomic.h"
#include "machine.h"

bool InterruptSafeLock::lock() {
    while (true) {
        bool wasDisabled = disable();
        if (!taken.exchange(true)) return wasDisabled;
        enable(wasDisabled);
    }
}

void InterruptSafeLock::unlock(bool wasDisabled) {
    taken.set(false);
    enable(wasDisabled);
}
