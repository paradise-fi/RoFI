#include "loggingService.hpp"

void LoggingService::logInfo( const std::string& message, LogVerbosity messageVerbosityLevel )
{
    if ( _logger == nullptr )
    {
        return;
    }

    if ( messageVerbosityLevel > _verbosity )
    {
        return;
    }

    _logger->logInfo( message );
}

void LoggingService::logWarning( const std::string& message )
{
    if ( _logger == nullptr )
    {
        return;
    }

    _logger->logWarning( message );
}

void LoggingService::logError( const std::string& message )
{
    if ( _logger == nullptr )
    {
        return;
    }

    _logger->logError( message );
}
