#pragma once

#include <shared_mutex>
#include "memoryEntry.hpp"
#include "../distribution/messaging/messageSender.hpp"
#include "sharedMemoryBase.hpp"
#include <map>

using namespace rofi::hal;
using namespace rofi::net;

class ReplicatedMemory : public SharedMemoryBase {
    std::map< int, MemoryEntry > _storage;
    mutable std::shared_mutex _mutex;

    bool saveData( int address, unsigned int stamp, uint8_t* data, size_t size, bool isLeader)
    {
        std::unique_lock lock( _mutex );
        
        auto entry = _storage.find( address );

        if ( entry != _storage.end() )
        {
            if ( isLeader && entry->second.stamp >= stamp )
            {
                return false;
            }
            
            entry->second.stamp = stamp;
            entry->second.stored_data.resize( size );
            std::memcpy( entry->second.stored_data.data(), data, size );
            
            return true;
        }

        auto res = _storage.emplace(address, MemoryEntry( stamp, size ) );
        if ( res.second )
        {
            std::memcpy( res.first->second.stored_data.data(), data, size );
            
            return true;
        }

        return false;
    }

public:
    virtual MemoryWriteResult writeData( uint8_t* data, size_t size, int address, bool isLeader ) override
    {
        MemoryWriteResult result;
        result.metadataOnly = false;
        unsigned int timestamp = as< unsigned int >( data );
        
        result.success = saveData( address, timestamp, data + sizeof( unsigned int ), size - sizeof( unsigned int ), isLeader );
        result.propagationType = MemoryPropagationType::SEND_TO_ALL;
        
        return result;
    }

    /// @brief Reads data from an address in the shared memory.
    /// @param address - An address of the requested data.
    /// @return Data in uint8_t* format. Nullptr if no data found under the address.
    virtual MemoryReadResult readData( int address ) override
    {
        MemoryReadResult result;
        result.success = false;

        std::shared_lock lock( _mutex );
        
        auto entry = _storage.find( address );
        if (entry == _storage.end())
        {
            return result;
        }

        result.success = true;
        result.raw_data = entry->second.stored_data;

        return result;
    }

    virtual MemoryWriteResult removeData( int address, bool ) override
    {
        MemoryWriteResult result;
        result.success = false;

        std::shared_lock lock( _mutex );

        if ( _storage.erase( address ) > 0 )
        {
            result.success = true;
            result.propagationType = MemoryPropagationType::SEND_TO_ALL;
        }

        return result;
    }
    
    virtual void clear() override
    {
        std::shared_lock lock( _mutex );
        _storage.clear();
    }

    // This implementation only allows to read the timestamp with the stamp key.
    virtual MemoryReadResult readMetadata( int address, std::string key ) override
    {
        MemoryReadResult result;
        result.success = false;

        if ( key != std::string( "stamp" ) )
        {
            return result;
        }

        std::shared_lock lock( _mutex );
        auto entry = _storage.find( address );
        
        if ( entry == _storage.end() )
        {
            return result;
        }

        result.raw_data.resize( sizeof( unsigned int ) );
        std::memcpy( result.raw_data.data(), &(entry->second.stamp), sizeof( unsigned int ) );
        result.success = true;

        return result; 
    }

    // This implementation does not allow to store metadata.
    virtual MemoryWriteResult writeMetadata( int, std::string, uint8_t*, size_t, bool ) override 
    {
        MemoryWriteResult result;
        result.success = false;
        result.metadataOnly = true;
        return result;
    }

    virtual MemoryWriteResult removeMetadata( int, std::string, bool ) override
    {
        MemoryWriteResult result;
        result.success = false;
        result.metadataOnly = true;
        return result;
    }

    virtual std::vector< uint8_t > serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int address, bool )
    {
        unsigned int timestamp = 0;
        auto stored = _storage.find( address );

        if ( stored != _storage.end() )
        {
            timestamp = stored->second.stamp;
        }

        timestamp++;

        std::vector< uint8_t > buffer;
        buffer.resize( sizeof( unsigned int ) + dataSize );
        std::memcpy( buffer.data(), &timestamp, sizeof( unsigned int ) );
        std::memcpy( buffer.data() + sizeof( unsigned int ), data, dataSize );

        return buffer;
    }
};