#pragma once

#include <string>

class LoggerBase
{
public:
    virtual ~LoggerBase() = default;
    virtual void logInfo( const std::string& message ) = 0;
    virtual void logWarning( const std::string& message ) = 0;
    virtual void logError( const std::string& message ) = 0;
};
