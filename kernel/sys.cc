#include "sys.h"
#include "stdint.h"
#include "idt.h"
#include "debug.h"
#include "machine.h"
#include "u8250.h"
#include "bobfs.h"
#include "threads.h"
#include "elf.h"
#include "vector.h"
#include "string.h"
#include "hashmap.h"
#include "fsfile.h"

extern g_FS* sysFS;
extern VMMInfo *vmm_info;
PIDManager* g_pid = nullptr;

bool isOpenFile(uint32_t fd)
{
	StrongPtr<PCB> activePCB = active()->threadPCB;
	return PCB::isFile(fd) && !(activePCB->fdArray[fd].isNull()) ;
}

bool isUnallowedAddress(uint32_t addr)
{
	const uint32_t sharedKernelEnd = 0x80000000;
	const uint32_t sharedKernelStart = 0xF0000000;
	return addr < sharedKernelEnd || addr >= sharedKernelStart;
}

class sys_call
{
	StrongPtr<PCB> activePCB;
	uint32_t* userStack;
public:
	sys_call(uint32_t* userStack) : activePCB(active()->threadPCB), userStack(userStack) {}

	/* never returns, rc is the exit code */
	void exit()
	{
		/*** Arguments ***/
		int32_t rc = userStack[1];
		int32_t curPid = activePCB->pid;

		g_pid->exitPID(curPid, rc);
		stop();
	}
	
	// FIX FOR INVALID BUF MEMORY ADDRESSES
	/* writes up to 'nbytes' to file, returns number of files written */
	ssize_t write()
	{
		/*** Arguments ***/
		uint32_t fd = userStack[1];
		char* buf = (char*)userStack[2];
		uint32_t nbytes = userStack[3];

		/*** Unpermitted address ***/
		if(isUnallowedAddress((uint32_t) buf)) { return -1; }

		/*** Error ***/
		if(!isOpenFile(fd)) { return -1; }

		/*** Get and write to file ***/
		StrongPtr<File> file = activePCB->fdArray[fd]->file;
		uint32_t offset = activePCB->fdArray[fd]->offset;
		nbytes = file->writeAll(offset, buf, nbytes);

		/*** Increment if file ***/
		if(!file->std())
		{
			activePCB->fdArray[fd]->offset += nbytes;
		}

		return nbytes;
	}

	/* fork */
	int32_t fork(uint32_t userPC, uint32_t userESP)
	{
		Thread* p_thread = active();
		int32_t childPID = g_pid->nextPID();

		/*** Out of PIDs ***/
		if(childPID == -1) { return -1; }

		/*** Future to stall parent ***/
		Future<int32_t> sync;

		/*** Create child process ***/
		thread([p_thread, userPC, userESP, childPID, &sync](){

			/*** Set mappings ***/
			uint32_t *pd = p_thread->threadPCB->addressSpace->getPD();
			uint32_t P = 1;
		    for (uint32_t i0 = 512; i0 < 960; i0++) {
		        uint32_t pde = pd[i0];
		        if (pde & P) {
		            uint32_t *pt = (uint32_t*) (pde & 0xfffff000);
		            for (uint32_t i1 = 0; i1 < 1024; i1++) {
		                uint32_t pte = pt[i1];
		                if (pte & P) {
		                    uint32_t pa = pte & 0xfffff000;
		                    uint32_t va = (i0 << 22) | (i1 << 12);
		                    // memcpy((void*)va, (void*)pa, 4096);
		                    pt[i1] &= ~(0x2);
		                    active()->threadPCB->addressSpace->pmap(va, pa, true, false);
		                    vmm_info->inc(pa);
		                }
		            }
		        }
		    }

		    // AddressSpace::compareAddressSpace(p_thread->threadPCB->addressSpace, active()->threadPCB->addressSpace);
		    
		    /*** Copy over values for childPCB and set PID ***/
		    StrongPtr<PCB> childPCB = active()->threadPCB;
		    childPCB->pid = childPID;
		    childPCB->parent = p_thread->threadPCB;
		    for(uint32_t i = 0; i < PCB::max_num; i++)
		    {
		    	StrongPtr<OpenFile> parentFile = p_thread->threadPCB->fdArray[i];	    	
		    	if(!parentFile.isNull())
		    	{
		    		childPCB->fdArray[i] = StrongPtr<OpenFile> {new OpenFile(parentFile->file, parentFile->offset) };
		    	}
		    	childPCB->semaphores[i] = p_thread->threadPCB->semaphores[i];
		    }

		    /*** Parent can go now ***/
		    sync.set(1);
		    switchToUser(userPC, userESP, 0);
		});
		sync.get();
		return childPID;
	}

	/* returns semaphore descriptor */
	int32_t sem()
	{
		/*** Arguments ***/
		uint32_t initialVal = userStack[1];

		for(uint32_t semId = 0; semId < PCB::max_num; semId++)
		{
			if(activePCB->semaphores[semId].isNull())
			{
				activePCB->semaphores[semId] = StrongPtr<Semaphore>{new Semaphore(initialVal)};
				return semId + PCB::sem_off;
			}
		}
		return -1;
	}

	/* semaphore up */
	int32_t up()
	{
		/*** Arguments ***/
		uint32_t s = userStack[1];

		/*** Not semaphore ***/
		if(s < PCB::sem_off || s >= PCB::child_off ) { return -1; }

		uint32_t sIdx = s - PCB::sem_off;
		
		/*** Not open semaphore ***/
		if(activePCB->semaphores[sIdx].isNull()) { return -1; }

		activePCB->semaphores[sIdx]->up();
		return 1;
	}

	/* semaphore down */
	int32_t down()
	{
		/*** Arguments ***/
		uint32_t s = userStack[1];

		/*** Not semaphore ***/
		if(s < PCB::sem_off || s >= PCB::child_off ) { return -1; }

		uint32_t sIdx = s - PCB::sem_off;

		/*** Not open semaphore ***/
		if(activePCB->semaphores[sIdx].isNull()) { return -1; }

		activePCB->semaphores[sIdx]->down();
		return 1;
	}

	/* closes either a file or a semaphore or disowns a child process */
	int32_t close()
	{
		/*** Arguments ***/
		uint32_t desc = userStack[1];

		if(PCB::isFile(desc))
		{
			uint32_t fdIdx = desc;

			/*** Already closed ***/
			if(activePCB->fdArray[fdIdx].isNull()) { return -1; }

			activePCB->fdArray[fdIdx].reset();
		}
		else if (PCB::isSemaphore(desc))
		{
			uint32_t semIdx = desc - PCB::sem_off;

			/*** Already closed ***/
			if(activePCB->semaphores[semIdx].isNull()) { return -1; }

			activePCB->semaphores[semIdx].reset();
		}
		else 
		{
			/*** Already closed ***/
			if(g_pid->closePID(desc) == -1) { return -1; }
		}
		return 0;
	}

	/* shutdown */
	void shutdown()
	{
		Debug::shutdown();
	}

	/* wait */
	int32_t wait()
	{
		/*** Arguments ***/
		uint32_t childPID = userStack[1];
		uint32_t* buf = (uint32_t*)(userStack[2]);
		/*** Unpermitted address ***/
		if(isUnallowedAddress((uint32_t) buf)) { return -1; }
		
		/*** Invalid ID ***/
		if(!PCB::isChild(childPID)) { return -1; }

		/*** Child doesn't exist ***/
		if(g_pid->waitPID(childPID, buf) == -1) { return -1; }
		return *buf;
	}

	/* execl */
	int32_t execl()
	{
		/*** Arguments ***/
		char* filePath = (char*) userStack[1];
		
		/*** Find executable ***/
		StrongPtr<Node> file = BobFS::find(sysFS->fs, filePath);

		/*** Get string args ***/
		vector<string> argStrings;
		for(int i = 2; userStack[i] != '\0'; i++) 
		{ 
			string s ((char*)(userStack[i]));
			argStrings.push_back(s);
		}
		uint32_t argc = argStrings.size();

		/*** Start setting up our stack***/
		uint32_t newESP = (uint32_t)userStack;
        
        /*** Protect the kernel ***/
		// if(ELF::load(file) == 0) { return -1; }

		/*** Delete previous mappings ***/
		AddressSpace* tmp = active()->threadPCB->addressSpace;
	    active()->threadPCB->addressSpace = new AddressSpace(false);
	    active()->threadPCB->addressSpace->activate();
	    delete tmp;

	    /*** PROTECT KERNEL? ***/
	    /*** load the executable ***/
    	uint32_t e = ELF::load(file);

    	/*** Cancer stack manipulation below ***/
    	/*** Copy over the string args and store pointers ***/
    	vector<uint32_t> argPtrs;
		for(uint32_t i = 0; i < argc; i++)
		{
			newESP = newESP - argStrings[i].length() - 1;
			memcpy((void*) newESP, argStrings[i].c_str(), argStrings[i].length() + 1);
			argPtrs.push_back(newESP);
		}

		/*** Copy over the pointers contiguously ***/
		for (int32_t i = argc - 1; i >= 0; i--)
		{
			newESP = newESP - 4;
			*((uint32_t*)newESP) = argPtrs[i];
		}

		/*** argv is pointer to pointer array ***/
		uint32_t argv = newESP;

		/*** Push argv and argc ***/
		newESP = newESP - 4;
		*((uint32_t*)newESP) = argv;

		newESP = newESP - 4;
		*((uint32_t*)newESP) = argc;
  		
		// start the executable
    	switchToUser(e,newESP,0);
		return 0;
	}

	/* opens a file, returns file descriptor */
	int32_t open()
	{
		/*** Arguments ***/
		char* filePath = (char*)userStack[1];
		/*** Look for the file ***/
		StrongPtr<Node> vNode = BobFS::find(sysFS->fs, filePath);

		/*** File doesn't exist ***/
		if(vNode.isNull()) { return -1; }

		/*** Do a cache lookup ***/
		uint32_t inum = vNode->getInum();
		StrongPtr<File> file;
		if(sysFS->openFiles.find(inum))
		{
			file = sysFS->openFiles[inum];
		}
		else
		{
			file = StrongPtr<File>{ new FsFile(vNode) };
			sysFS->openFiles.insert(inum, file);
		}

		/*** Find first available fd and add entry ***/
		for(uint32_t fd = 0; fd < PCB::sem_off; fd++)
		{
			if(activePCB->fdArray[fd].isNull())
			{
				activePCB->fdArray[fd] = StrongPtr<OpenFile>{new OpenFile(file, 0)};
				return fd;
			}
		}
		return -1;
	}

	/* returns number of bytes in file */
	ssize_t len()
	{
		/*** Arguments ***/
		uint32_t fd = userStack[1];

		/*** Error ***/
		if(!isOpenFile(fd)) { return -1; }

		return activePCB->fdArray[fd]->file->size();
	}

	/* reads up to nbytes from file, returns number of bytes read */
	ssize_t read()
	{
		/*** Arguments ***/
		uint32_t fd = userStack[1];
		void* buf = (void*) userStack[2];
		uint32_t nbytes = userStack[3];

		/*** Unpermitted address ***/
		if(isUnallowedAddress((uint32_t) buf)) { return -1; }

		/*** Error ***/
		if(!isOpenFile(fd)) { return -1; }

		/*** Get file and read ***/
		StrongPtr<File> file = activePCB->fdArray[fd]->file;
		uint32_t offset = activePCB->fdArray[fd]->offset;
		uint32_t bytesRead = file->readAll(offset, buf, nbytes);

		/*** Increment if file ***/
		if(!file->std())
		{
			activePCB->fdArray[fd]->offset += bytesRead;
		}

		return bytesRead;
	}

	/* seek to given offset in file */
	off_t seek()
	{
		/*** Arguments ***/
		uint32_t fd = userStack[1];
		uint32_t offset = userStack[2];
		
		/*** Error ***/
		if(!isOpenFile(fd)) { return -1; }

		activePCB->fdArray[fd]->offset = offset;
		return offset;
	}
};


extern "C" int sysHandler(uint32_t eax, uint32_t *frame) {
	uint32_t* userStack = (uint32_t*) frame[3];
	sys_call sys(userStack);
	switch(eax)
	{
		case 0:			sys.exit(); break;
		case 1:  return sys.write();
		case 2:  return sys.fork(frame[0], frame[3]);
		case 3:  return sys.sem();
		case 4:  return sys.up();
		case 5:  return sys.down();
		case 6:  return sys.close();
		case 7:   	    sys.shutdown(); break;
		case 8:  return sys.wait();
		case 9:  return sys.execl();
		case 10: return sys.open();
		case 11: return sys.len();
		case 12: return sys.read();
		case 13: return sys.seek(); 
		default: Debug::printf("*** default\n");

	}
    return 0;
}

void SYS::init(void) {
    IDT::trap(48,(uint32_t)sysHandler_,3);
}
