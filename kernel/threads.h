#ifndef _threads_h_
#define _threads_h_

#include "atomic.h"
#include "queue.h"
#include "heap.h"
#include "bobfs.h"
#include "semaphore.h"
#include "future.h"
#include "vmm.h"
#include "u8250.h"
#include "sys.h"

extern void threadsInit();

class PCB;

class PIDManager
{
private:
    Atomic<uint32_t> pid_c = 2000;
public:
    Hashmap<int32_t, StrongPtr<Future<int32_t>>> pid_map;

    int32_t nextPID()
    {
        // allocate PIDs using an incremented counter
        int32_t pid = pid_c.add_fetch(1);
        pid_map.insert(pid, StrongPtr<Future<int32_t>>{new Future<int32_t>()});
        return pid;
    }
    int32_t exitPID(int32_t pid, int32_t rc)
    {
        if(!pid_map.find(pid)) { return -1; }
        
        pid_map[pid]->set(rc);
        return 1;
    }
    int32_t waitPID(int32_t pid, uint32_t* buf)
    {
        if(!pid_map.find(pid)) { return -1; }
        if(pid_map[pid].isNull()) { return -1; }
        
        *buf = pid_map[pid]->get();
        return 1;
    }
    int32_t closePID(int32_t pid)
    {
        if(!pid_map.find(pid)) { return -1; }
        if(pid_map[pid].isNull()) { return -1; }
        
        pid_map[pid].reset();
        return 1;
    }
};

extern PIDManager* g_pid;



class OpenFile
{
public:
    StrongPtr<File> file;
    uint32_t offset;
    Mutex lock;
    OpenFile(StrongPtr<File> file, uint32_t offset) : file(file), offset(offset) {}
    int32_t writeAll(off_t offset,void* buf,uint32_t n)
    {
        MutexLocker locker(lock);
       int32_t bytesRead= this->file->writeAll(offset, buf, n);
        this->offset+=bytesRead;
        return bytesRead;
    }

    int32_t readAll(off_t offset,void* buf,uint32_t n)
    {
        MutexLocker locker(lock);
        int32_t bytesRead= this->file->readAll(offset, buf, n);
        this->offset+=bytesRead;
        return bytesRead;
    }

    int32_t getSize()
    {
        MutexLocker locker(lock);
        return this->file->size();
    }
    int32_t doSeek(off_t offset)
    {
        MutexLocker locker(lock);
        this->offset = offset;
        return this->offset;
    }
    int32_t increaseOff(off_t bytes) {
        MutexLocker locker(lock);
        this->offset+=bytes;
        return this->offset;

    }
    
    ~OpenFile() {
        MutexLocker locker(lock);
        file.reset();
    }
};

class PCB 
{
private:
    Mutex semLock;
    Mutex fileLock;
public:
    static const uint32_t max_num = 1000;
    static const uint32_t fd_off = 0;
    static const uint32_t sem_off = 1 * PCB::max_num;
    static const uint32_t child_off = 2 * PCB:: max_num;

    StrongPtr<OpenFile> fdArray[max_num];
    StrongPtr<Semaphore> semaphores[max_num];
    StrongPtr<PCB> parent;
    uint32_t pid;
    AddressSpace *addressSpace;
    bool isOpenFile(uint32_t fd)
{
    //StrongPtr<PCB> activePCB = this;
    return PCB::isFile(fd) && !(this->fdArray[fd].isNull()) ;
}

    static bool isFile(uint32_t num)
    {
    	return num < PCB::sem_off && num >= 0;
    }
    static bool isSemaphore(uint32_t num)
    {
    	return num >= PCB::sem_off && num < PCB::child_off;
    }
    static bool isChild(uint32_t num)
    {
        return num >= PCB::child_off;  
    }
    
        int32_t allocateSemaphore(int32_t count) {
        MutexLocker lock(semLock);
        for(uint32_t i=0;i<max_num;i++) {

             if(semaphores[i].isNull()) {
                semaphores[i] = StrongPtr<Semaphore> { new Semaphore(count)};
                return i + sem_off;
            }

        }

        return -1;

    }

    int32_t allocateFD(StrongPtr<File> file) {
        MutexLocker lock(fileLock);
        for(uint32_t i=0;i<max_num;i++) {

             if(fdArray[i].isNull()) {
                fdArray[i] = StrongPtr<OpenFile> { new OpenFile(file, 0)};
                return i;
            }

        }

        return -1;

    }

    PCB()
    {
        StrongPtr<File> u8250 { new U8250File(new U8250()) };
        fdArray[0] = StrongPtr<OpenFile> {new OpenFile(u8250, 0)};
        fdArray[1] = StrongPtr<OpenFile> {new OpenFile(u8250, 0)};
        fdArray[2] = StrongPtr<OpenFile> {new OpenFile(u8250, 0)};
        addressSpace = new AddressSpace(false);
    }

};


class Thread {
public:
    Thread* next = nullptr;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t leaveMeAlone = 0;
    StrongPtr<PCB> threadPCB { new PCB() };
    virtual void start() = 0;
    virtual uint32_t interruptEsp() = 0;
    Thread();
    virtual ~Thread() {};
};

template <typename T>
class ThreadImpl : public Thread {
    T work;
public:
    long stack[2048];
    virtual void start() override {
        work();
    }
    virtual uint32_t interruptEsp() override {
        return (uint32_t) &stack[2046];
    }
    ThreadImpl(T work) : work(work) {}
    virtual ~ThreadImpl() {
    }
};

extern void stop();

extern void entry();

extern void yield();
extern void block(Thread*);
extern void stop();
extern Thread* active();
extern void schedule(Thread* t);
extern Thread* doNotDisturb();
extern void reaper();

template <typename T>
void thread(T work) {
    reaper();
    auto thread = new ThreadImpl<T>(work);
    long *topOfStack = &thread->stack[2045];
    //if ((((unsigned long) topOfStack) % 16) == 8) topOfStack --;
    topOfStack[0] = 0x200;       // sti
    topOfStack[1] = 0;           // cr2
    topOfStack[2] = (long)entry;
    thread->esp = (long) topOfStack;
    schedule(thread);
}

class Disable {
    bool was;
public:
    Disable() : was(disable()) {}
    ~Disable() { enable(was); }
};


#endif
