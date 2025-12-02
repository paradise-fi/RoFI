#pragma once
#include <functional>
#include "lwip++.hpp"
#include "../messaging/messagingResult.hpp"
#include "../services/electionService.hpp"
#include "callbackFacade.hpp"

class CallbackService : public CallbackFacade
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
    CallbackService( ElectionService& electionService, LoggingService& loggingService )
    : _electionService( electionService ), _loggingService( loggingService ) {}

    bool invokeOnTaskRequest( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester )
    {
        return !_onTaskRequest ? false : _onTaskRequest( manager, requester );
    }

    void invokeOnTaskFailure( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, int functionId )
    {
        if ( _onTaskFailure )
        {
            _onTaskFailure( manager, sender, functionId );
        }
    }

    void invokeOnCustomMessage( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
    {
        if ( !_onCustomMessage )
        {
            _loggingService.logError("Received custom message but no callback to handle it is registered.");
            return;
        }

        _onCustomMessage( manager, sender, data, size );
    }

    MessagingResult invokeOnCustomMessageBlocking( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
    {
        if ( !_onCustomMessageBlocking )
        {
            _loggingService.logError("Received blocking custom message but no callback to handle it is registered.");
            return MessagingResult( false );
        }
        
        return _onCustomMessageBlocking( manager, sender, data, size );
    }

    void invokeOnMemoryStored( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService )
    {
        if ( _onMemoryStored )
        {
            _onMemoryStored( memoryAddress, isLeaderMemory, memoryService );
        }
    }

    /// @brief Registers a callback that is called when leader election fails.
    /// @param callback The callback function.
    /// @return True if the registration succeeded.
    virtual bool registerLeaderFailureCallback( std::function< void() >&& callback ) override
    { 
        return _electionService.registerLeaderFailureCallback( std::forward< std::function< void() > >( callback ) ); 
    }

    /// @brief Unregisters a callback for leader election failure.
    /// @return True if the function was unregistered.
    virtual bool unregisterLeaderFailureCallback() override { return _electionService.unregisterLeaderFailureCallback(); }
    
    /// @brief Registers a callback that is called when a task request is received.
    /// @param callback Your custom callback. Returns true if the task request pipeline should not continue after this callback (e.g. to avoid double-scheduling of a task)
    virtual void registerTaskRequestCallback( OnTaskRequestCallback&& callback ) override
    { 
        _onTaskRequest = std::forward< OnTaskRequestCallback >( callback );
    }

    virtual void registerTaskFailedCallback( OnTaskFailureCallback&& callback ) override 
    { 
        _onTaskFailure = std::forward< OnTaskFailureCallback >( callback );
    }

    virtual void registerNonBlockingCustomMessageCallback( OnCustomMessageCallback&& callback ) override
    { 
        _onCustomMessage = std::forward< OnCustomMessageCallback >( callback );
    }

    virtual void registerBlockingCustomMessageCallback( OnCustomMessageBlockingCallback&& callback ) override
    { 
        _onCustomMessageBlocking = std::forward< OnCustomMessageBlockingCallback >( callback );
    }

    virtual void registerOnMemoryStoredCallback( OnMemoryStoredCallback&& callback ) override
    {
        _onMemoryStored = std::forward< OnMemoryStoredCallback >( callback );
    }
};