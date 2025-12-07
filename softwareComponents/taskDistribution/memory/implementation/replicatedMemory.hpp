#pragma once

#include <shared_mutex>
#include "memoryEntry.hpp"
#include "../distribution/messaging/messageSender.hpp"
#include "distributedMemoryBase.hpp"
#include <map>

using namespace rofi::hal;
using namespace rofi::net;

class ReplicatedMemory : public DistributedMemoryBase {
    std::map< int, MemoryEntry > _storage;
    mutable std::shared_mutex _mutex;

    bool saveData( int address, unsigned int stamp, uint8_t* data, size_t size, bool isLeader);

public:
    virtual MemoryStorageBehavior storageBehavior() const override;

    virtual MemoryWriteResult writeData( uint8_t* data, size_t size, int address, bool isLeader ) override;

    /// @brief Reads data from an address in the shared memory.
    /// @param address - An address of the requested data.
    /// @return Data in uint8_t* format. Nullptr if no data found under the address.
    virtual MemoryReadResult readData( int address, bool ) const override;

    virtual MemoryWriteResult removeData( int address, bool isLeader ) override;
    
    virtual void clear() override;

    // This implementation only allows to read the timestamp with the stamp key.
    virtual MemoryReadResult readMetadata( int address, const std::string& key, bool ) const override;

    // This implementation does not allow to store metadata.
    virtual MemoryWriteResult writeMetadata( int, const std::string&, uint8_t*, size_t, bool ) override;

    virtual MemoryWriteResult removeMetadata( int, const std::string&, bool ) override;

    virtual std::vector< uint8_t > serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int address, bool ) const override;
};