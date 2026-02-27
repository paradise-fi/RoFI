#pragma once
#include "../src/memory/services/memoryService.hpp"

class MemoryFacade
{
private:
    DistributedMemoryService& _memoryService;

public:
    MemoryFacade( DistributedMemoryService& memoryService );

    bool isMemoryRegistered();

    /// @brief Removes / deregisters the shared memory implementation from DistributedMemoryService.
    /// @return True if the memory implementation was deregistered, otherwise false.
    bool deleteMemory();

    /// @brief Save data in memory.
    /// @param data The data to be stored.
    /// @param size The size of the data.
    /// @param address The address to store the data at
    template < SerializableOrTrivial T >
    bool saveData( T&& data, int address )
    {
        return _memoryService.saveData< T >( std::move( data ), address );
    }

    /// @brief Reads data from specified address in memory.
    /// @param address The memory address to be read
    /// @return MemoryReadResult struct containing information about read success and a method for extracting the data read.
    MemoryReadResult readData( int address );
    
    /// @brief Remove data at specified address
    /// @param address The address of the data in memory
    void removeData( int address );
    
    /// @brief Remove all data from this module's memory.
    void clearLocalMemory();

    MemoryReadResult readMetadata( int address, const std::string& key );

    bool saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize );

    void removeMetadata( int address, const std::string& key );
    
    /// @brief Retrieves the internal implementation of memory for custom functionality that may require this access.
    /// @return NULLOPT if no memory has been registered. Otherwise a reference for the memory.
    std::optional< std::reference_wrapper< DistributedMemoryBase > > memory();

    /// @brief Registers a shared memory implementation in the memory service. Only one implementation may be registered at a time.
    /// @tparam Memory
    /// @param memory The DistributedMemoryBase implementation to be registered.
    /// @return True if memory was succesfully registered, otherwise false.
    template< std::derived_from< DistributedMemoryBase > Memory >
    bool useMemory( std::unique_ptr< Memory > memory )
    {
        return _memoryService.useMemory( std::move( memory ) );
    }
};