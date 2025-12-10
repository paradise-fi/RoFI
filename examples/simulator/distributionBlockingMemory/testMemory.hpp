#pragma once

#include <shared_mutex>
#include "distributedMemoryBase.hpp"
#include <map>
#include "address.cpp"

using namespace rofi::hal;
using namespace rofi::net;

#pragma once

#include <cstdint>
#include <vector>

struct TestMemoryEntry {
    std::vector<uint8_t> storedData;

    TestMemoryEntry() {}

    TestMemoryEntry( size_t size )
    {
        storedData.resize( size );
    }
};

class TestMemory : public DistributedMemoryBase {
    std::map< int, TestMemoryEntry > _storage;
    mutable std::shared_mutex _mutex;
    int _moduleId;

    bool saveData( int address, uint8_t* data, size_t size, bool)
    {
        std::unique_lock lock( _mutex );
        
        auto entry = _storage.find( address );

        if ( entry != _storage.end() )
        {
            entry->second.storedData.resize( size );
            std::memcpy( entry->second.storedData.data(), data, size );
            
            return true;
        }

        auto res = _storage.emplace(address, TestMemoryEntry( size ) );
        if ( res.second )
        {
            std::memcpy( res.first->second.storedData.data(), data, size );
            
            return true;
        }

        return false;
    }

    int getAddressStarterDigit( int address ) const
    {
        int result = 0;
        while ( address > 0 )
        {
            result = address % 10;
            address /= 10;
        }
        return result;
    }

public:
    TestMemory( int moduleId ) : _moduleId( moduleId ) {}

    virtual MemoryStorageBehavior storageBehavior() const override
    {
        return MemoryStorageBehavior::LocalFirstStorage;
    }

    virtual MemoryWriteResult writeData( uint8_t* data, size_t size, int address, bool isLeader ) override
    {
        int starterDigit = getAddressStarterDigit( address );
        
        MemoryWriteResult result;
        result.metadataOnly = false;

        if ( isLeader )
        {
            // Find the module's address and send write order.
            auto moduleAddress = createAddress( starterDigit );
            std::cout << "I am the leader and I have determined that address " << address << " belongs to " << moduleAddress << std::endl;
            result.success = true;
            result.propagationTarget = moduleAddress;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            return result;
        }

        result.propagationType = MemoryPropagationType::ONE_TARGET;
        
        if ( starterDigit == _moduleId )
        {
            std::cout << "I am the data holder for address " << address << ", writing..." << std::endl; 
            result.success = saveData( address, data, size, isLeader );
            result.propagationType = MemoryPropagationType::NONE;
        }
        else
        {
            std::cout << "I am not the data holder for address " << address << ", going to request leader to find the data." << std::endl;
            result.success = true;
            // By default, this will now target the leader.
            result.propagationTarget = std::nullopt;
        }
        
        return result;
    }

    /// @brief Reads data from an address in the shared memory.
    /// @param address - An address of the requested data.
    /// @return Data in uint8_t* format. Nullptr if no data found under the address.
    virtual MemoryReadResult readData( int address, bool isLeader ) const override
    {
        std::cout << "Inside Memory Implementation trying to read " << address << std::endl;
        int starterDigit = getAddressStarterDigit( address );
        MemoryReadResult result;
        result.success = false;
        
        if ( isLeader )
        {
            // Find the module's address and send read order.
            auto target = createAddress( starterDigit );
            std::cout << "I am the leader and I have routed " << address << " to be held by " << target << std::endl;
            result.success = true;
            result.requestRemoteRead = true;
            result.readTarget = target;
            return result;
        }
        
        result.requestRemoteRead = false;

        if ( starterDigit != _moduleId )
        {
            std::cout << "I do not hold the data on address " << address << ", requesting leader to route this data." << std::endl;
            result.requestRemoteRead = true;
            return result;
        }

        std::shared_lock lock( _mutex );
        
        auto entry = _storage.find( address );
        if ( entry == _storage.end() )
        {
            std::cout << "Data under address " << address << " not found under this module." << std::endl;
            return result;
        }

        std::cout << "Data under address " << address << " found in this module's memory." << std::endl;
        result.success = true;
        result.rawData = entry->second.storedData;
        return result;
    }

    virtual MemoryWriteResult removeData( int address, bool isLeader ) override
    {
        std::cout << "Inside Memory Implementation - Responding to Remove Request" << std::endl;
        int starterDigit = getAddressStarterDigit( address );
        MemoryWriteResult result;
        result.success = false;

        if ( isLeader )
        {
            // Find the module's address and send write order.
            auto target = createAddress( starterDigit );
            std::cout << "I am the leader and have routed " << address << " to " << target << std::endl;
            result.success = true;
            result.propagationTarget = target;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            return result;
        }
        
        if ( getAddressStarterDigit( address ) != _moduleId )
        {
            std::cout << "I am not the data holder of " << address << ", nor am I the leader. I will request the leader to find the data holder." << std::endl;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            result.success = true;
            return result;
        }

        std::shared_lock lock( _mutex );

        if ( _storage.erase( address ) > 0 )
        {
            std::cout << "I am the data holder of " << address << " and have erased the data." << std::endl;
            result.success = true;
            result.propagationType = MemoryPropagationType::NONE;
        }

        return result;
    }
    
    virtual void clear() override
    {
        std::shared_lock lock( _mutex );
        _storage.clear();
    }

    // No metadata.
    virtual MemoryReadResult readMetadata( int, const std::string&, bool ) const override
    {
        MemoryReadResult result;
        result.success = false;
        return result; 
    }

    // This implementation does not allow to store metadata.
    virtual MemoryWriteResult writeMetadata( int, const std::string&, uint8_t*, size_t, bool ) override 
    {
        MemoryWriteResult result;
        result.success = false;
        result.metadataOnly = true;
        return result;
    }

    virtual MemoryWriteResult removeMetadata( int, const std::string&, bool ) override
    {
        MemoryWriteResult result;
        result.success = false;
        result.metadataOnly = true;
        return result;
    }

    virtual std::vector< uint8_t > serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int, bool ) const override
    {
        std::vector< uint8_t > buffer;
        buffer.resize( dataSize );
        std::memcpy( buffer.data(), data, dataSize );

        return buffer;
    }
};