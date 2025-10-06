#pragma once

#include <cstdint>
#include <vector>

struct MemoryEntry {
    unsigned int stamp;
    std::vector<uint8_t> storedData;

    MemoryEntry() {}

    MemoryEntry( unsigned int stamp, size_t size )
    : stamp( stamp ){
        storedData.resize( size );
    }
};