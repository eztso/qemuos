#include "debug.h"
#include "ide.h"
#include "bobfs.h"
#include "elf.h"
#include "hashmap.h"
#include "threads.h"
void kernelStart(void) {
}

StrongPtr<Node> checkFile(const char* name, StrongPtr<Node> node) {
    if (node.isNull()) {
    }
    if (node->isDirectory()) {
    }
    return node;
}

StrongPtr<Node> getFile(StrongPtr<Node> node, const char* name) {
    return checkFile(name,node->findNode(name));
}

StrongPtr<Node> checkDir(const char* name, StrongPtr<Node> node) {
    if (node.isNull()) {
    }
    if (!node->isDirectory()) {
    }
    return node;
}

StrongPtr<Node> getDir(StrongPtr<Node> node, const char* name) {
    return checkDir(name,node->findNode(name));
}

void kernelMain(void) {
    StrongPtr<Ide> d { new Ide(3) };
    // Debug::printf("*** 0 mounting drive d\n");
    auto fs = BobFS::mount(d);
    auto root = checkDir("/",BobFS::root(fs));
    auto sbin = getDir(root,"sbin");
    auto init = getFile(sbin,"init");
    g_pid = new PIDManager();
    uint32_t e = ELF::load(init);
    switchToUser(e,0xeffff000,0);
}

void kernelTerminate(void) {
}
