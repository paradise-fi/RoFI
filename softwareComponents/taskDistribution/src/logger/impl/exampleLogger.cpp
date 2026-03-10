#include "../../../include/systemMethods/exampleLogger.hpp"

ExampleLogger::ExampleLogger() : _logLevel( LogLevel::Info ) {}
ExampleLogger::ExampleLogger( LogLevel logLevel ) : _logLevel( logLevel ) {}

void ExampleLogger::logInfo( const std::string& message )
{
    if ( !isInLogLevel( LogLevel::Info ) )
    {
        return;
    }

    std::cout << "Info: " << message << std::endl;
}

void ExampleLogger::logWarning( const std::string& message )
{
    if ( !isInLogLevel( LogLevel::Warning ) )
    {
        return;
    }

    std::cout << "Warning: " << message << std::endl;
}

void ExampleLogger::logError( const std::string& message )
{
    if ( !isInLogLevel( LogLevel::Error ) )
    {
        return;
    }

    std::cout << "Error: " << message << std::endl;
}

// =============== PRIVATE
bool ExampleLogger::isInLogLevel( LogLevel messageLevel )
{
    if ( _logLevel == LogLevel::Info )
    {
        return true;
    }

    if ( _logLevel == LogLevel::Warning )
    {
        return messageLevel == LogLevel::Warning || messageLevel == LogLevel::Error;
    }

    return messageLevel == LogLevel::Error;
}