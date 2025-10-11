#pragma once

#include "../utils/loggerBase.hpp"
#include <iostream>

enum LogLevel {
    Info,
    Warning,
    Error,
};

class ExampleLogger : public LoggerBase
{
    LogLevel _logLevel;

    bool isInLogLevel( LogLevel messageLevel )
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
public:
    ExampleLogger() : _logLevel( LogLevel::Info ) {}
    ExampleLogger( LogLevel logLevel ) : _logLevel( logLevel ) {}

    virtual void logInfo( const std::string& message ) override {
        if ( !isInLogLevel( LogLevel::Info ) )
        {
            return;
        }

        std::cout << "Info: " << message << std::endl;
    }

    virtual void logWarning( const std::string& message ) override {
        if ( !isInLogLevel( LogLevel::Warning ) )
        {
            return;
        }

        std::cout << "Warning: " << message << std::endl;
    }

    virtual void logError( const std::string& message ) override {
        if ( !isInLogLevel( LogLevel::Error ) )
        {
            return;
        }

        std::cout << "Error: " << message << std::endl;
    }
};