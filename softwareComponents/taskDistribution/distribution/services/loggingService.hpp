#pragma once

#include "../../utils/loggerBase.hpp"
#include <memory>
#include <concepts>

class LoggingService
{
    std::unique_ptr< LoggerBase > _logger;
public:
    template< std::derived_from< LoggerBase > Logger >
    void useLogger( const Logger& logger )
    {
        _logger = std::make_unique< Logger >( logger );
    }

    void logInfo( const std::string& message )
    {
        if ( _logger == nullptr )
        {
            return;
        }

        _logger->logInfo( message );
    }

    void logWarning( const std::string& message )
    {
        if ( _logger == nullptr )
        {
            return;
        }

        _logger->logWarning( message );
    }

    void logError( const std::string& message )
    {
        if ( _logger == nullptr )
        {
            return;
        }

        _logger->logError( message );
    }
};