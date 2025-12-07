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

    bool isInLogLevel( LogLevel messageLevel );
    
public:
    ExampleLogger();
    ExampleLogger( LogLevel logLevel );

    virtual void logInfo( const std::string& message ) override;

    virtual void logWarning( const std::string& message ) override;

    virtual void logError( const std::string& message ) override;
};