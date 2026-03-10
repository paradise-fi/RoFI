#include "blockingMessageDataService.hpp"
#include <iostream>

void BlockingMessageDataService::completeBlockingMessage( uint8_t* message, size_t messageSize )
{
    {
        std::lock_guard lk( _lock );
        _blockingDataBuffer.resize( messageSize );
        std::memcpy( _blockingDataBuffer.data(), message, messageSize );
        _dataReady = true;
    }
    _blockingMessageCV.notify_all();
}

void BlockingMessageDataService::clearFlag()
{
    std::lock_guard lk( _lock );
    _dataReady = false;
}

MessagingResult BlockingMessageDataService::awaitBlockingMessage( int timeout )
{
    std::unique_lock lk ( _lock );
    _dataReady = false;
    auto signalled = _blockingMessageCV.wait_for( lk, std::chrono::milliseconds( timeout ), [&](){ return _dataReady; } );

    if ( !signalled )
    {
        return MessagingResult( std::string("Message sending timed out."), false );
    }

    return MessagingResult( _blockingDataBuffer, true );
}