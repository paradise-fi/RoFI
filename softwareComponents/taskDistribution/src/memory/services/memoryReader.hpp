#pragma once
#include "../../../include/distributedMemoryBase.hpp"
#include <queue>
#include "../memoryReadResult.hpp"
#include "../memoryRequestQueueItem.hpp"
#include <atoms/util.hpp>
#include "../../messaging/messagingService.hpp"
#include "../../logger/loggingService.hpp"
#include "memoryMessagingWrapper.hpp"

class DistributedMemoryBase;

class MemoryReader
{
    DistributedMemoryBase* _memory = nullptr;
    MemoryMessagingWrapper& _memoryMessaging;
    rofi::hal::Ip6Addr _currentModuleAddress;
    MessagingService& _messaging;
    LoggingService& _loggingService;
    int _blockingMessageTimeoutMs = 300;
    

    // For external read requests that need to be sent over network.
    std::queue<MemoryRequestQueueItem> _memoryReadQueue;

    MemoryReadResult readDataBlocking( const Ip6Addr& target, uint8_t* data, size_t dataSize );

    MemoryReadResult forwardReadRequest( const Ip6Addr& target, uint8_t* data, size_t dataSize );

    MemoryReadResult readDataInternal( int address, const Ip6Addr& origin, bool isUserCall );

    MemoryReadResult readMetadataInternal( int address, const Ip6Addr& origin, const std::string& key, bool isUserCall );

    public:
    MemoryReader( MemoryMessagingWrapper& memoryMessaging, rofi::hal::Ip6Addr currentModuleAddress, MessagingService& messaging, LoggingService& logging, int blockingMessageTimeoutMs );
    
    /// @brief Reads data from specified address in memory.
    /// @param address The memory address to be read
    /// @return MemoryReadResult struct containing information about read success and a method for extracting the data read.
    MemoryReadResult readData( int address );
    
    MemoryReadResult readMetadata( int address, const std::string& key );
    
    void emplaceIntoQueue( Ip6Addr& sender, uint8_t* data, size_t dataSize, int address, bool isMetadataOnly, MemoryRequestType requestType );
    bool processQueue();

    /// @brief Remove all entries in this module's storage queue.
    void clearLocalQueue();

    void unregisterMemory();

    void registerMemory( DistributedMemoryBase* memory );
};