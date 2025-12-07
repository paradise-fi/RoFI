#include "../distribution/callbacks/callbackService.hpp"

CallbackService::CallbackService( ElectionService& electionService, LoggingService& loggingService )
    : _electionService( electionService ), _loggingService( loggingService ) {}


bool CallbackService::invokeOnTaskRequest( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester )
{
    return !_onTaskRequest ? false : _onTaskRequest( manager, requester );
}

void CallbackService::invokeOnTaskFailure( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, int functionId )
{
    if ( _onTaskFailure )
    {
        _onTaskFailure( manager, sender, functionId );
    }
}

void CallbackService::invokeOnCustomMessage( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
{
    if ( !_onCustomMessage )
    {
        _loggingService.logError("Received custom message but no callback to handle it is registered.");
        return;
    }

    _onCustomMessage( manager, sender, data, size );
}

MessagingResult CallbackService::invokeOnCustomMessageBlocking( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, size_t size )
{
    if ( !_onCustomMessageBlocking )
    {
        _loggingService.logError("Received blocking custom message but no callback to handle it is registered.");
        return MessagingResult( false );
    }
    
    return _onCustomMessageBlocking( manager, sender, data, size );
}

void CallbackService::invokeOnMemoryStored( int memoryAddress, bool isLeaderMemory, DistributedMemoryService& memoryService )
{
    if ( _onMemoryStored )
    {
        _onMemoryStored( memoryAddress, isLeaderMemory, MemoryFacade( memoryService ) );
    }
}

bool CallbackService::registerLeaderFailureCallback( std::function< void() >&& callback )
{ 
    return _electionService.registerLeaderFailureCallback( std::forward< std::function< void() > >( callback ) ); 
}

bool CallbackService::unregisterLeaderFailureCallback()
{
    return _electionService.unregisterLeaderFailureCallback();
}

void CallbackService::registerTaskRequestCallback( OnTaskRequestCallback&& callback )
{ 
    _onTaskRequest = std::forward< OnTaskRequestCallback >( callback );
}

void CallbackService::registerTaskFailedCallback( OnTaskFailureCallback&& callback )
{ 
    _onTaskFailure = std::forward< OnTaskFailureCallback >( callback );
}

void CallbackService::registerNonBlockingCustomMessageCallback( OnCustomMessageCallback&& callback )
{ 
    _onCustomMessage = std::forward< OnCustomMessageCallback >( callback );
}

void CallbackService::registerBlockingCustomMessageCallback( OnCustomMessageBlockingCallback&& callback )
{ 
    _onCustomMessageBlocking = std::forward< OnCustomMessageBlockingCallback >( callback );
}

void CallbackService::registerOnMemoryStoredCallback( OnMemoryStoredCallback&& callback )
{
    _onMemoryStored = std::forward< OnMemoryStoredCallback >( callback );
}