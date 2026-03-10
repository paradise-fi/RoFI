#pragma once

#include "../../include/loggerBase.hpp"
#include "verbosity.hpp"
#include <memory>
#include <concepts>

class LoggingService
{
    LogVerbosity _verbosity = LogVerbosity::Medium;
    std::unique_ptr< LoggerBase > _logger;
public:

    void logInfo( const std::string& message, LogVerbosity messageVerbosityLevel = LogVerbosity::Medium );

    void logWarning( const std::string& message );

    void logError( const std::string& message );

    template< std::derived_from< LoggerBase > Logger >
    void useLogger( const Logger& logger, LogVerbosity verbosity )
    {
        _verbosity = verbosity;
        _logger = std::make_unique< Logger >( logger );
    }
};