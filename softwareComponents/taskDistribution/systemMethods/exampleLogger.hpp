#pragma once

#include "../utils/loggerBase.hpp"
#include <iostream>

class ExampleLogger : public LoggerBase
{
public:
    virtual void logInfo( const std::string& message ) override {
        std::cout << "Info: " << message << std::endl;
    }

    virtual void logWarning( const std::string& message ) override {
        std::cout << "Warning: " << message << std::endl;
    }

    virtual void logError( const std::string& message ) override {
        std::cout << "Error: " << message << std::endl;
    }
};