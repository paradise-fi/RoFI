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
    std::map< int, int > _metadata;
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
            std::cout << "[TestMemory] I am the leader and I have determined that address " << address << " belongs to " << moduleAddress << std::endl;
            result.success = true;
            result.propagationTarget = moduleAddress;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            return result;
        }

        result.propagationType = MemoryPropagationType::ONE_TARGET;
        
        if ( starterDigit == _moduleId )
        {
            std::cout << "[TestMemory] I am the data holder for address " << address << ", writing..." << std::endl; 
            result.success = saveData( address, data, size, isLeader );
            result.propagationType = MemoryPropagationType::NONE;
        }
        else
        {
            std::cout << "[TestMemory] I am not the data holder for address " << address << ", going to request leader to find the data." << std::endl;
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
        std::cout << "[TestMemory] Inside Memory Implementation trying to read " << address << std::endl;
        int starterDigit = getAddressStarterDigit( address );
        MemoryReadResult result;
        result.success = false;
        
        if ( isLeader )
        {
            // Find the module's address and send read order.
            auto target = createAddress( starterDigit );
            std::cout << "[TestMemory] I am the leader and I have routed " << address << " to be held by " << target << std::endl;
            result.success = true;
            result.requestRemoteRead = true;
            result.readTarget = target;
            return result;
        }
        
        result.requestRemoteRead = false;

        if ( starterDigit != _moduleId )
        {
            std::cout << "[TestMemory] I do not hold the data on address " << address << ", requesting leader to route this data." << std::endl;
            result.requestRemoteRead = true;
            return result;
        }

        std::shared_lock lock( _mutex );
        
        auto entry = _storage.find( address );
        if ( entry == _storage.end() )
        {
            std::cout << "[TestMemory] Data under address " << address << " not found under this module." << std::endl;
            return result;
        }

        std::cout << "[TestMemory] Data under address " << address << " found in this module's memory." << std::endl;
        result.success = true;
        result.rawData = entry->second.storedData;
        return result;
    }

    virtual MemoryWriteResult removeData( int address, bool isLeader ) override
    {
        std::cout << "[TestMemory] Inside Memory Implementation - Responding to Remove Request" << std::endl;
        int starterDigit = getAddressStarterDigit( address );
        MemoryWriteResult result;
        result.success = false;

        if ( isLeader )
        {
            // Find the module's address and send write order.
            auto target = createAddress( starterDigit );
            std::cout << "[TestMemory] I am the leader and have routed " << address << " to " << target << std::endl;
            result.success = true;
            result.propagationTarget = target;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            return result;
        }
        
        if ( getAddressStarterDigit( address ) != _moduleId )
        {
            std::cout << "[TestMemory] I am not the data holder of " << address << ", nor am I the leader. I will request the leader to find the data holder." << std::endl;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            result.success = true;
            return result;
        }

        std::shared_lock lock( _mutex );

        if ( _storage.erase( address ) > 0 )
        {
            std::cout << "[TestMemory] I am the data holder of " << address << " and have erased the data." << std::endl;
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
    virtual MemoryReadResult readMetadata( int address, const std::string& key, bool isLeader ) const override
    {
        std::cout << "[TestMemory] Inside Memory Implementation trying to read metadata at " << address << std::endl;
        MemoryReadResult result;
        result.success = false;
        result.requestRemoteRead = false;
        
        if ( key != "metadata" )
        {
            std::cout << "[TestMemory] Invalid key " << key << ". You may only use the \"metadata\" key in this implementation." << std::endl;
            return result;
        }

        if ( !isLeader )
        {
            result.requestRemoteRead = true;
            result.success = true;
            return result;
        }
        
        // Find the module's address and send read order.
        address = getAddressStarterDigit( address );
        std::shared_lock lock( _mutex );
        auto entry = _metadata.find( address );
        if ( entry == _metadata.end() )
        {
            std::cout << "[TestMemory] Failed to read metadata" << std::endl;
            return result;
        }

        // auto target = createAddress( starterDigit );
        std::cout << "[TestMemory] I am the leader and I have found this metadata under " << address << ", it is: " << entry->second << std::endl;
        result.success = true;
        result.rawData.resize( sizeof( int ) );
        std::memcpy( result.rawData.data(), &(entry->second), sizeof( int ) );
        return result;
    }

    /// @brief This is a VERY SIMPLIFIED version of writing metadata that naively only takes one form of metadata and treats its data as integers.

    virtual MemoryWriteResult writeMetadata( int address, const std::string& key, uint8_t* data, size_t, bool isLeader ) override 
    {
        MemoryWriteResult result;
        result.metadataOnly = true;
        result.success = false;

        if ( key != "metadata" )
        {
            std::cout << "[TestMemory] Invalid key " << key << ". You may only use the \"metadata\" key in this implementation." << std::endl;
            return result;
        }

        address = getAddressStarterDigit( address );
        if ( !isLeader )
        {
            std::cout << "[TestMemory] I am not the leader, I am not writing metadata and will request writing from the leader." << std::endl;
            result.success = true;
            // By default, this will now target the leader.
            result.propagationTarget = std::nullopt;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
            return result;
        }

        std::shared_lock lock( _mutex );
        std::cout << "[TestMemory] Storing metadata at address " << address << std::endl;
        result.success = true;
        _metadata.emplace( address, as< int >( data ) );
        result.propagationType = MemoryPropagationType::NONE;
        result.metadataOnly = true;
        return result;
    }

    virtual MemoryWriteResult removeMetadata( int address, const std::string& key, bool isLeader ) override
    {
        MemoryWriteResult result;
        result.metadataOnly = true;
        result.success = false;

        if ( key != "metadata" )
        {
            std::cout << "[TestMemory] Invalid key " << key << ". You may only use the \"metadata\" key in this implementation." << std::endl;
            return result;
        }
        
        address = getAddressStarterDigit( address );
        
        if ( !isLeader )
        {
            std::cout << "[TestMemory] I am not the leader, I am not removing metadata and will request removal from the leader." << std::endl;
            result.success = true;
            // By default, this will now target the leader.
            result.propagationTarget = std::nullopt;
            result.propagationType = MemoryPropagationType::ONE_TARGET;
        }

        std::shared_lock lock( _mutex );

        if ( _metadata.erase( address ) > 0 )
        {
            std::cout << "[TestMemory] I am the data holder of " << address << " metadata and have erased the data." << std::endl;
            result.success = true;
            result.propagationType = MemoryPropagationType::NONE;
        }

        return result;
    }

    virtual std::vector< uint8_t > serializeDataToMemoryFormat( uint8_t* data, size_t dataSize, int, bool ) const override
    {
        std::vector< uint8_t > buffer;
        if ( dataSize > 0 )
        {
            buffer.resize( dataSize );
            std::memcpy( buffer.data(), data, dataSize );
        }

        return buffer;
    }
};