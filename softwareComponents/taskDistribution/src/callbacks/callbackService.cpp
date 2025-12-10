#include "callbackService.hpp"

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

bool CallbackService::registerLeaderFailureCallback( std::function< void() >&& callback )
{ 
    return _electionService.registerLeaderFailureCallback( std::forward< std::function< void() > >( callback ) ); 
}

bool CallbackService::unregisterLeaderFailureCallback()
{
    return _electionService.unregisterLeaderFailureCallback();
}

void CallbackService::registerTaskRequestCallback( 
        std::function< bool( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& requester ) >&& callback )
{ 
    _onTaskRequest = std::forward< 
        std::function< bool( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& requester ) > >( callback );
}

void CallbackService::registerTaskFailedCallback(  
        std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             const int functionId ) >&& callback )
{ 
    _onTaskFailure = std::forward<
        std::function< void( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, 
                             const int functionId ) > >( callback );
}

void CallbackService::registerNonBlockingCustomMessageCallback( 
        std::function< void( DistributedTaskManager& manager,
                             const rofi::hal::Ip6Addr& sender,
                             uint8_t* data,
                             const size_t size ) >&& callback )
{ 
    _onCustomMessage = std::forward<
        std::function< void( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender, uint8_t* data, const size_t size ) > >( callback );
}

void CallbackService::registerBlockingCustomMessageCallback( 
        std::function< MessagingResult( DistributedTaskManager& manager,
                                        const rofi::hal::Ip6Addr& sender,
                                        uint8_t* data,
                                        const size_t size ) >&& callback )
{ 
    _onCustomMessageBlocking = std::forward< 
        std::function< MessagingResult( DistributedTaskManager& manager, const rofi::hal::Ip6Addr& sender,
                                        uint8_t* data, const size_t size ) > >( callback );
}