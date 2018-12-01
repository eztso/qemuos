#include "init.h"

#include "debug.h"
#include "config.h"
#include "u8250.h"
#include "smp.h"
#include "machine.h"
#include "kernel.h"
#include "heap.h"
#include "threads.h"
#include "pit.h"
#include "idt.h"
#include "vmm.h"
#include "tss.h"
#include "sys.h"

struct Stack {
    static constexpr int BYTES = 4096;
    uint32_t bytes[BYTES] __attribute__ ((aligned(16)));
};

PerCPU<Stack> stacks;

static bool smpInitDone = false;

extern "C" uint32_t pickKernelStack(void) {
    return (uint32_t) &stacks.forCPU(smpInitDone ? SMP::me() : 0).bytes[Stack::BYTES];
}

static Atomic<uint32_t> howManyAreHere(0);

extern "C" void kernelInit(void) {
    uint32_t me = SMP::started.fetch_add(1);


    if (me == 0) {
        U8250 uart;
        Debug::init(&uart);
        Debug::debugAll = false;
        Debug::printf("\n| What just happened? Why am I here?\n");

        /* discover configuration */
        configInit(&kConfig);
        Debug::printf("| totalProcs %d\n",kConfig.totalProcs);
        Debug::printf("| memSize 0x%x %dMB\n",
            kConfig.memSize,
            kConfig.memSize / (1024 * 1024));
        Debug::printf("| localAPIC %x\n",kConfig.localAPIC);
        Debug::printf("| ioAPIC %x\n",kConfig.ioAPIC);

        /* initialize the heap */
        heapInit((void*)0x100000,1 << 20);


        /* initialize physmem and paging */
        VMM::init(0x200000, kConfig.memSize - 2 * 1024 * 1024);

        /* initialize system calls */
        SYS::init();

        /* initialize the thread module */
        threadsInit();

        /* initialize LAPIC */
        SMP::init(true);
        smpInitDone = true;
  
        IDT::init();
        Pit::calibrate(1000);

        // The reset EIP has to be
        //     - divisible by 4K (required by LAPIC)
        //     - PPN must fit in 8 bits (required by LAPIC)
        //     - consistent with mbr.S
        for (uint32_t id = 1; id < kConfig.totalProcs; id++) {
            Debug::printf("| initialize %d\n",id);
            SMP::ipi(id, 0x4500);
            Debug::printf("| reset %d\n",id);
            Debug::printf("|      eip:0x%x\n",resetEIP);
            SMP::ipi(id, 0x4600 | (((uintptr_t)resetEIP) >> 12));
            while (SMP::started <= id);
        }
    } else {
        SMP::init(false);
    }

    // Initialize the PIT
    Pit::init();

    auto id = SMP::me();

    Debug::printf("| loading TSS for %d\n",id);
    tss[id].ss0 = kernelSS;

    ltr(tssDescriptorBase + id * 8);

    Debug::printf("| %d enabling interrupts, I'm scared\n",id);
    sti();

    auto myOrder = howManyAreHere.add_fetch(1);
    if (myOrder == kConfig.totalProcs) {
        thread([] {
            kernelMain();
            Debug::panic("| kernelMain returned\n");
        });
    }
    stop();
    

}
