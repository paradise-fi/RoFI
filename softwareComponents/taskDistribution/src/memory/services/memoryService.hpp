#pragma once
#include "../../../include/distributedMemoryBase.hpp"
#include <queue>
#include "lwip++.hpp"
#include "atoms/util.hpp"
#include "../../messaging/messagingService.hpp"
#include "../memoryRequestType.hpp"
#include "../../logger/loggingService.hpp"
#include "../../callbacks/systemCallbackManager.hpp"
#include "../memoryRequestQueueItem.hpp"
#include "memoryWriter.hpp"
#include "memoryReader.hpp"
#include "memoryMessagingWrapper.hpp"

using namespace rofi::hal;
using namespace rofi::net;

class DistributedMemoryService
{
    const unsigned int METHOD_ID = 2;

    LoggingService& _loggingService;
    SystemCallbackManager& _callbackService;
    MemoryMessagingWrapper _memoryMessagingWrapper;
    MemoryWriter _memoryWriter;
    MemoryReader _memoryReader;
    std::unique_ptr< DistributedMemoryBase > _memory;

public:
    DistributedMemoryService( MessageDistributor& distributor, MessagingService& messaging,
        Ip6Addr& currentModuleAddress, LoggingService& loggingService, 
        SystemCallbackManager& callbackService, int blockingMessageTimeoutMs = 300 );

    bool isMemoryRegistered();

    /// @brief Removes / deregisters the shared memory implementation from DistributedMemoryService.
    /// @return True if the memory implementation was deregistered, otherwise false.
    bool deleteMemory();
    
    void onMemoryMessage( Ip6Addr sender, uint8_t* data, size_t size, MemoryRequestType requestType );

    /// @brief Checks whether the memory, in this instant, is going to process any more write operations that are queued.
    /// @return True if there are no more pending writes.
    bool isMemoryStable();

    void processQueues( unsigned int writeQueueBatchSize = 1, unsigned int readQueueBatchSize = 5 );
    
    /// @brief Save data in memory.
    /// @param data The data to be stored.
    /// @param size The size of the data.
    /// @param address The address to store the data at
    template < SerializableOrTrivial T >
    bool saveData( T&& data, int address )
    {
        return _memoryWriter.saveData< T >( std::forward< T >( data ), address );
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

    /// @brief Remove all entries in this module's storage queue.
    void clearLocalQueue();
    
    MemoryReadResult readMetadata( int address, const std::string& key );

    bool saveMetadata( int address, const std::string& key, uint8_t* metadata, std::size_t metadataSize );

    void removeMetadata( int address, const std::string& key );
    
    void setLeader( const Ip6Addr& leader );
    
    /// @brief Retrieves the internal implementation of memory for custom functionality.
    /// @return NULLOPT if no memory has been registered. Otherwise a reference for the memory.
    std::optional< std::reference_wrapper< DistributedMemoryBase > > memory();

    /// @brief Registers a shared memory implementation in the memory service. Only one implementation may be registered at a time.
    /// @tparam Memory
    /// @param memory The DistributedMemoryBase implementation to be registered.
    /// @return True if memory was succesfully registered, otherwise false.
    template< std::derived_from< DistributedMemoryBase > Memory >
    bool useMemory( std::unique_ptr< Memory > memory )
    {
        if (_memory != nullptr )
        {
            return false;
        }

        _memory = std::move( memory );
        _memoryWriter.registerMemory( _memory.get() );
        _memoryReader.registerMemory( _memory.get() );
        _memoryMessagingWrapper.registerMemory( _memory.get() );
        return true;
    }
};