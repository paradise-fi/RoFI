#pragma once
#include "../memory/sharedMemoryBase.hpp"

class MemoryService
{
    std::unique_ptr< SharedMemoryBase > _memory;
    
public:
    bool isMemoryRegistered()
    {
        return _memory != nullptr;
    }

    /// @brief Removes / deregisters the shared memory implementation from MemoryService.
    /// @return True if the memory implementation was deregistered, otherwise false.
    bool deleteMemory()
    {
        if ( _memory == nullptr )
        {
            return false;
        }

        _memory.reset();
        return true;
    }

    /// @brief Registers a shared memory implementation in the memory service. Only one implementation may be registered at a time.
    /// @tparam Memory
    /// @param memory The SharedMemoryBase implementation to be registered.
    /// @return True if memory was succesfully registered, otherwise false.
    template< std::derived_from< SharedMemoryBase > Memory >
    bool useMemory( std::unique_ptr< Memory > memory )
    {
        if (_memory != nullptr )
        {
            return false;
        }

        _memory = std::move( memory );
        return true;
    } 

    /// @brief Save data in memory.
    /// @param data The data to be stored.
    /// @param size The size of the data.
    /// @param address The address to store the data at
    /// @return True if data was stored succesfully, otherwise false.
    bool saveData( uint8_t* data, int size, int address )
    {
        return _memory->store( data, size, address );
    }

    /// @brief Reads data from specified address in memory.
    /// @tparam T the expected data type of the result
    /// @param address The memory address to be read
    /// @param out Out parameter for the memory value
    /// @return True if value was successfully read, otherwise false.
    template< typename T >
    bool readData( int address, T& out )
    {
        std::vector< uint8_t > data = _memory->read( address );

        if ( data.size() < sizeof( T ) )
        {
            return false;    
        }

        std::memcpy( &out, data.data(), sizeof( T ) );
        return true;
    }

    /// @brief Remove data at specified address
    /// @param address The address of the data in memory
    /// @return True if data was removed, otherwise false
    bool removeData( int address )
    {
        return _memory->remove( address );
    }

    /// @brief Remove all data from memory.
    void clearMemory()
    {
        return _memory->clear();
    }

    template< typename T >
    bool readMetadata( int address, std::string key, T& out )
    {
        std::vector< uint8_t > metadata = _memory->readMetadata( address, key );

        if ( metadata.size() < sizeof( T ) )
        {
            return false;
        }

        std::memcpy( &out, metadata.data(), sizeof( T ) );
        return true;
    }

    bool saveMetadata( int address, std::string key, uint8_t* metadata, int metadataSize )
    {
        return _memory->storeMetadata( address, key, metadata, metadataSize );
    }

    void onStorageMessage( Ip6Addr sender, uint8_t* data, unsigned int size )
    {
        _memory->onStorageMessage( sender, data, size );
    }

    void setLeader( Ip6Addr leader )
    {
        _memory->setLeader( leader );
    }
};