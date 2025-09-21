#pragma once

#include <cstdint>
#include <vector>

struct MemoryEntry {
    unsigned int stamp;
    std::vector<uint8_t> stored_data;

    MemoryEntry() {}

    MemoryEntry( unsigned int stamp, size_t size )
    : stamp( stamp ){
        stored_data.resize( size );
    }
};