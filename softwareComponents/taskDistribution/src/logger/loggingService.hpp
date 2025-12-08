#pragma once

#include "../../include/loggerBase.hpp"
#include <memory>
#include <concepts>

class LoggingService
{
    std::unique_ptr< LoggerBase > _logger;
public:

    void logInfo( const std::string& message );

    void logWarning( const std::string& message );

    void logError( const std::string& message );
    
    template< std::derived_from< LoggerBase > Logger >
    void useLogger( const Logger& logger )
    {
        _logger = std::make_unique< Logger >( logger );
    }
};