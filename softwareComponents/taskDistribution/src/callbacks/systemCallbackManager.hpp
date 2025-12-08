#pragma once
#include "../messaging/messagingResult.hpp"

class DistributedTaskManager;
class DistributedMemoryService;

class SystemCallbackManager
{
public:
    virtual bool invokeOnTaskRequest( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester ) = 0;

    virtual void invokeOnTaskFailure( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, int functionId ) = 0;

    virtual void invokeOnCustomMessage( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) = 0;

    virtual MessagingResult invokeOnCustomMessageBlocking( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) = 0;

    virtual void invokeOnMemoryStored( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService ) = 0;
};