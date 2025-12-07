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
    OnTaskRequestCallback _onTaskRequest;
    OnTaskFailureCallback _onTaskFailure;
    OnCustomMessageCallback _onCustomMessage;
    OnCustomMessageBlockingCallback _onCustomMessageBlocking;
    OnMemoryStoredCallback _onMemoryStored;

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
    virtual void registerTaskRequestCallback( OnTaskRequestCallback&& callback ) override;

    virtual void registerTaskFailedCallback( OnTaskFailureCallback&& callback ) override;

    virtual void registerNonBlockingCustomMessageCallback( OnCustomMessageCallback&& callback ) override;

    virtual void registerBlockingCustomMessageCallback( OnCustomMessageBlockingCallback&& callback ) override;

    virtual void registerOnMemoryStoredCallback( OnMemoryStoredCallback&& callback ) override;
};