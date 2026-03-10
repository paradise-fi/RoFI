#pragma once
#include <string>
#include <vector>

struct MessagingResult
{
    bool success;
    std::string statusMessage;
    std::vector< uint8_t > rawData;

    MessagingResult( std::string statusMessage, bool success = false ) : success( success ), statusMessage( std::move( statusMessage ) ), rawData( 0 ) {}
    MessagingResult( std::vector< uint8_t >& buffer, bool success = true ) : success( success ), rawData( buffer ) {}
    MessagingResult( bool success, size_t dataSize ) : success( success ), rawData( dataSize ) {}
    MessagingResult( bool success ) : success( success ), rawData( 0 ) {}
};