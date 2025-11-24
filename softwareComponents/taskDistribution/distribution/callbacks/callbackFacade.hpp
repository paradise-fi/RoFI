#pragma once
#include <functional>
#include "lwip++.hpp"
#include "../messaging/messagingResult.hpp"
#include "../services/electionService.hpp"

class DistributedTaskManager;
class DistributedMemoryService;

class CallbackFacade
{
public:
    using OnTaskRequestCallback = std::function< bool( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester ) >;
    using OnTaskFailureCallback = std::function< void( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, const int functionId ) >;
    using OnCustomMessageCallback = std::function< void( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, const size_t size ) >;
    using OnCustomMessageBlockingCallback = std::function< MessagingResult( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, const size_t size ) >;
    using OnMemoryStoredCallback = std::function< void( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService ) >;

    /// @brief Registers a callback that is called when leader election fails.
    /// @param callback The callback function.
    /// @return True if the registration succeeded.
    virtual bool registerLeaderFailureCallback( std::function< void() >&& callback ) = 0;

    /// @brief Unregisters a callback for leader election failure.
    /// @return True if the function was unregistered.
    virtual bool unregisterLeaderFailureCallback() = 0;
    
    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    virtual void registerTaskRequestCallback( OnTaskRequestCallback&& callback ) = 0;

    virtual void registerTaskFailedCallback( OnTaskFailureCallback&& callback ) = 0;

    virtual void registerNonBlockingCustomMessageCallback( OnCustomMessageCallback&& callback ) = 0;

    virtual void registerBlockingCustomMessageCallback( OnCustomMessageBlockingCallback&& callback ) = 0;

    virtual void registerOnMemoryStoredCallback( OnMemoryStoredCallback&& callback ) = 0;
};