#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>

#include <google/protobuf/message.h>

#ifndef LOG_MESSAGES
#ifdef NDEBUG
#define LOG_MESSAGES 0 // Release mode
#else
#define LOG_MESSAGES 1 // Debug mode
#endif
#endif


namespace rofi::hal
{

inline void logMessage( [[maybe_unused]] const std::string & topic,
                        [[maybe_unused]] const google::protobuf::Message & msg,
                        [[maybe_unused]] bool sending )
{
#if LOG_MESSAGES
    auto now = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    if ( sending ) {
        std::cerr << std::put_time( std::localtime( &now ), "%T" ) << " Sending to '" << topic
                  << "':\n"
                  << msg.ShortDebugString() << std::endl;
    } else {
        std::cerr << std::put_time( std::localtime( &now ), "%T" ) << " Received from '" << topic
                  << "':\n"
                  << msg.ShortDebugString() << std::endl;
    }
#endif
}

inline void logSubscription( [[maybe_unused]] const std::string & topic,
                        [[maybe_unused]] bool starting )
{
#if LOG_MESSAGES
    auto now = std::chrono::system_clock::to_time_t( std::chrono::system_clock::now() );
    if ( starting ) {
        std::cerr << std::put_time( std::localtime( &now ), "%T" ) << " Subscribing to '" << topic
                  << std::endl;
    } else {
        std::cerr << std::put_time( std::localtime( &now ), "%T" ) << " Unsubscribing from '" << topic
                  << std::endl;
    }
#endif
}

} // namespace rofi::hal
