#pragma once

#include <iostream>
#include <experimental/source_location>

namespace std {
    using std::experimental::source_location;
}


namespace rofi::log {

void info( const std::string_view& message,
    const std::source_location& location = std::source_location::current() )
{
    std::cout << "info:"
              << location.file_name() << ":"
              << location.line() << " "
              << message << '\n';
}

void warning( const std::string_view& message,
    const std::source_location& location = std::source_location::current() )
{
    std::cout << "warning:"
              << location.file_name() << ":"
              << location.line() << " "
              << message << '\n';
}

void error(const std::string_view& message,
    const std::source_location& location = std::source_location::current() )
{
    std::cout << "error:"
              << location.file_name() << ":"
              << location.line() << " "
              << message << '\n';
}

} // namespace rofi::log

