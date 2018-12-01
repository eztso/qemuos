#ifndef _IDE_H_
#define _IDE_H_

#include "stdint.h"

struct Ide {
    static constexpr uint32_t SectorSize = 512;
    uint32_t drive; /* 0 -> A, 1 -> B, 2 -> C, 3 -> D */

    Ide(uint32_t drive) : drive(drive) {}
    
    void readSector(uint32_t sector, void* buffer);
    void writeSector(uint32_t sector, const void* buffer);

    int32_t read(uint32_t offset, void* buffer, uint32_t n);
    int32_t write(uint32_t offset, const void* buffer, uint32_t n);


    int32_t readAll(uint32_t offset, void* buffer, uint32_t n);
    int32_t writeAll(uint32_t offset, const void* buffer, uint32_t n);
};

#endif
