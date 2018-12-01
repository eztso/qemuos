#include "elf.h"
#include "machine.h"
#include "refs.h"
#include "bobfs.h"

uint32_t ELF::load(StrongPtr<Node> file) {

	const uint32_t sharedKernelEnd = 0x80000000;
	const uint32_t sharedKernelStart = 0xF0000000;

    ElfHeader hdr;

    file->readAll(0,&hdr,sizeof(ElfHeader));

    uint32_t hoff = hdr.phoff;

    for (uint32_t i=0; i<hdr.phnum; i++) {
        ProgramHeader phdr;
        file->readAll(hoff,&phdr,sizeof(ProgramHeader));
        hoff += hdr.phentsize;
        if (phdr.type == 1) {

            char *p = (char*) phdr.vaddr;

            /*** Protect the kernel ***/
        	if((uint32_t)p < sharedKernelEnd || (uint32_t)p >= sharedKernelStart) return 0;

            uint32_t memsz = phdr.memsz;
            uint32_t filesz = phdr.filesz;

            Debug::printf("vaddr:%x memsz:0x%x filesz:0x%x fileoff:%x\n",
                p,memsz,filesz,phdr.offset);
            file->readAll(phdr.offset,p,filesz);
            bzero(p + filesz, memsz - filesz);
        }
     }
	/*** Protect the kernel ***/
    if(hdr.entry < sharedKernelEnd || hdr.entry >= sharedKernelStart) return 0;

    return hdr.entry;
}
