#pragma once
#include "../../messaging/messagingService.hpp"
#include "../memoryPropagationType.hpp"
#include "../../logger/loggingService.hpp"
#include "../../../include/distributedMemoryBase.hpp"

class MemoryMessagingWrapper
{
    MessagingService& _messaging;
    LoggingService& _loggingService;
    unsigned int _methodId;
    DistributedMemoryBase* _memory;
    rofi::hal::Ip6Addr _currentAddress;
    rofi::hal::Ip6Addr _leader;
    

public:
    MemoryMessagingWrapper( MessagingService& messagingService, LoggingService& loggingService, unsigned int methodId, rofi::hal::Ip6Addr& currentAddress );

    void unregisterMemory();
    void registerMemory(  DistributedMemoryBase* memory );

    void setLeader( const rofi::hal::Ip6Addr& leaderAddr );
    const rofi::hal::Ip6Addr& getLeader();
    bool isLeaderMemory();

    size_t genericMemoryMessageHeaderSize();
    size_t genericMessageHeaderSize();
    
    void prepareMemoryMessageData( uint8_t* buffer, int address, const uint8_t* data, size_t dataSize, bool isMetadataOnly );

    void sendDataBroadcast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, DistributionMessageType messageType );
    void sendDataUnicast( int address, const uint8_t* data, size_t size, bool isMetadataOnly, const Ip6Addr& target, DistributionMessageType messageType );

    void propagateMemoryChange( const MemoryPropagationType type, int address, uint8_t* data, size_t size,
        bool isMetadataOnly, std::optional< Ip6Addr > target, DistributionMessageType messageType );

    void propagateMetadataChange( MemoryPropagationType type, int address, const std::string& key, uint8_t* metadata, 
        size_t metadataSize, std::optional< Ip6Addr > target, DistributionMessageType messageType);
};