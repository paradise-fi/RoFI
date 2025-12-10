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

class SystemCallbackManager; 

using namespace rofi::hal;
using namespace rofi::net;

class DistributedMemoryService
{
    const unsigned int METHOD_ID = 2;
    const unsigned int READ_BATCH_SIZE = 5;

    MessagingService& _messaging;
    Ip6Addr _leader;
    const Ip6Addr& _currentModuleAddress;
    std::unique_ptr< DistributedMemoryBase > _memory;
    
    // For external read requests that need to be sent over network.
    std::queue<MemoryRequestQueueItem> _memoryReadQueue;
    
    // For external write requests.
    std::queue<MemoryRequestQueueItem> _memoryStorageQueue;
    LoggingService& _loggingService;
    SystemCallbackManager& _callbackService;
    int _blockingMessageTimeoutMs = 300;

    bool isLeaderMemory();

    size_t genericMemoryMessageHeaderSize();
    
    void prepareMemoryMessageData( uint8_t* buffer, int address, const uint8_t* data, size_t dataSize, bool isMetadataOnly );

    void sendDataBroadcast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, DistributionMessageType messageType );
    void sendDataUnicast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, const Ip6Addr& target, DistributionMessageType messageType );

    void propagateMemoryChange( const MemoryPropagationType type, int address, uint8_t* data, size_t size,
        bool isMetadataOnly, std::optional< Ip6Addr > target, DistributionMessageType messageType );

    void propagateMetadataChange( MemoryPropagationType type, int address, const std::string& key, uint8_t* metadata, 
        size_t metadataSize, std::optional< Ip6Addr > target, DistributionMessageType messageType);

    MemoryWriteResult handleMetadataUpdate( MemoryRequestQueueItem& memoryItem );

    MemoryWriteResult handleDataUpdate( MemoryRequestQueueItem& memoryItem );

    MemoryReadResult readDataBlocking( Ip6Addr& target, uint8_t* data, size_t dataSize );

    MemoryReadResult forwardReadRequest( Ip6Addr& target, uint8_t* data, size_t dataSize );

    MemoryReadResult readDataInternal( int address, const Ip6Addr& origin, bool isUserCall );

    MemoryReadResult readMetadataInternal( int address, const Ip6Addr& origin, const std::string& key, bool isUserCall );

    void handleReadRequest( MemoryRequestQueueItem& memoryItem );

    bool processMemoryQueue( std::queue< MemoryRequestQueueItem >& queue );

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

    void processQueue();
    
    /// @brief Save data in memory.
    /// @param data The data to be stored.
    /// @param size The size of the data.
    /// @param address The address to store the data at
    template < SerializableOrTrivial T >
    bool saveData( T&& data, int address )
    {
        if ( !isMemoryRegistered() )
        {
            return false;
        }
        // Convert the data to the memory format required by the implementation
        std::vector< uint8_t > dataBuffer;

        if constexpr ( std::is_base_of_v< Serializable, T > )
        {
            std::vector< uint8_t > buffer;
            buffer.resize( data.size() );
            data.serialize( buffer );
            dataBuffer = _memory->serializeDataToMemoryFormat( buffer.data(), buffer.size(), address, false );
        }
        else
        {
            dataBuffer = _memory->serializeDataToMemoryFormat( reinterpret_cast< uint8_t* >( &data ), sizeof( T ), address );
        }

        if ( _memory->storageBehavior() == MemoryStorageBehavior::LeaderFirstStorage )
        {
            if ( isLeaderMemory() )
            {
                auto result = _memory->writeData( dataBuffer.data(), dataBuffer.size(), address, true );
                
                if ( result.success )
                {
                    propagateMemoryChange( result.propagationType, address, dataBuffer.data(), dataBuffer.size(), 
                        false, result.propagationTarget, DistributionMessageType::DataStorageRequest );
                }

                return result.success;
            }

            propagateMemoryChange( MemoryPropagationType::ONE_TARGET, address, 
                dataBuffer.data(), dataBuffer.size(), false, _leader, 
                DistributionMessageType::DataStorageRequest );

            return true;
        }

        auto result = _memory->writeData( dataBuffer.data(), dataBuffer.size(), address, false );
        if ( result.success )
        {
            propagateMemoryChange( result.propagationType, address, dataBuffer.data(), dataBuffer.size(), 
                false, result.propagationTarget, DistributionMessageType::DataStorageRequest );
        }
        return result.success;
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
        return true;
    }
};