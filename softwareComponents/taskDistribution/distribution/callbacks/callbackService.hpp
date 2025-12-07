#pragma once
#include <functional>
#include "lwip++.hpp"
#include "../messaging/messagingResult.hpp"
#include "../services/electionService.hpp"
#include "callbackFacade.hpp"
#include "systemCallbackManager.hpp"

class CallbackService : public CallbackFacade, public SystemCallbackManager
{
private:
    std::function< bool( DistributedTaskManager& manager,
                         const rofi::hal::Ip6Addr& requester ) > _onTaskRequest;

    std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             const int functionId ) > _onTaskFailure;

    std::function< void( DistributedTaskManager& manager,
                         const rofi::hal::Ip6Addr& sender,
                         uint8_t* data,
                         const size_t size ) > _onCustomMessage;

    std::function< MessagingResult( DistributedTaskManager& manager,
                                    const rofi::hal::Ip6Addr& sender,
                                    uint8_t* data,
                                    const size_t size ) > _onCustomMessageBlocking;

    std::function< void( int memoryAddress,
                         bool isLeaderMemory,
                         MemoryFacade memory ) > _onMemoryStored;

    ElectionService& _electionService;
    LoggingService& _loggingService;

public:
    CallbackService( ElectionService& electionService, LoggingService& loggingService );

    virtual bool invokeOnTaskRequest( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester ) override;

    virtual void invokeOnTaskFailure( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, int functionId ) override;

    virtual void invokeOnCustomMessage( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) override;

    virtual MessagingResult invokeOnCustomMessageBlocking( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size ) override;

    virtual void invokeOnMemoryStored( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService ) override;

    /// @brief Registers a callback that is called when leader election fails.
    /// @param callback The callback function.
    /// @return True if the registration succeeded.
    virtual bool registerLeaderFailureCallback( std::function< void() >&& callback ) override;

    /// @brief Unregisters a callback for leader election failure.
    /// @return True if the function was unregistered.
    virtual bool unregisterLeaderFailureCallback() override;
    
    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    virtual void registerTaskRequestCallback(
        std::function< bool( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& requester ) >&& callback ) override;

    virtual void registerTaskFailedCallback( 
        std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             const int functionId ) >&& callback ) override;

    virtual void registerNonBlockingCustomMessageCallback( 
        std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             uint8_t* data,
                             const size_t size ) >&& callback ) override;

    virtual void registerBlockingCustomMessageCallback(
        std::function< MessagingResult( DistributedTaskManager& manager,
                                        const rofi::hal::Ip6Addr& sender,
                                        uint8_t* data,
                                        const size_t size ) >&& callback ) override;

    virtual void registerOnMemoryStoredCallback(
        std::function< void( int memoryAddress,
                             bool isLeaderMemory,
                             MemoryFacade memory ) >&& callback ) override;
};