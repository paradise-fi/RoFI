#pragma once
#include <functional>
#include "lwip++.hpp"
#include "../messaging/messagingResult.hpp"
#include "../election/electionService.hpp"
#include "../../include/memoryFacade.hpp"

class DistributedTaskManager;
class DistributedMemoryService;

class CallbackFacade
{
public:
    /// @brief Registers a callback that is called when leader election fails.
    /// @param callback The callback function.
    /// @return True if the registration succeeded.
    virtual bool registerLeaderFailureCallback( std::function< void() >&& callback ) = 0;

    /// @brief Unregisters a callback for leader election failure.
    /// @return True if the function was unregistered.
    virtual bool unregisterLeaderFailureCallback() = 0;
    
    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    virtual void registerTaskRequestCallback( 
        std::function< bool( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& requester ) >&& callback ) = 0;

    /// @brief Registers a callback that is called when a task failure is received. This failure arises from a system configuration issue, such as a function not being registered at a follower module.
    /// @param callback Your custom callback.
    virtual void registerTaskFailedCallback( 
        std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             const int functionId ) >&& callback ) = 0;
    
    /// @brief Registers a callback for the event of a non-blocking custom message reception.
    /// @param callback Your custom callback.
    virtual void registerNonBlockingCustomMessageCallback( 
        std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             uint8_t* data,
                             const size_t size ) >&& callback ) = 0;

    /// @brief Registers a callback for the event of a blocking custom message reception.
    /// @param callback Your custom callback.
    virtual void registerBlockingCustomMessageCallback( 
        std::function< MessagingResult( DistributedTaskManager& manager,
                                        const rofi::hal::Ip6Addr& sender,
                                        uint8_t* data,
                                        const size_t size ) >&& callback ) = 0;

    /// @brief Registers a callback for the event of memory being succesfully written into on this module.
    /// @param callback Your custom callback.
    virtual void registerOnMemoryStoredCallback( 
        std::function< void( int memoryAddress,
                             bool isLeaderMemory,
                             MemoryFacade memory ) >&& callback ) = 0;
};