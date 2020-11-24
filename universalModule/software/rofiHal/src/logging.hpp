#pragma once

#include <iostream>
#include <experimental/source_location>

namespace std {
    using std::experimental::source_location;
}

namespace rofi::log::detail {
    std::string limitPathLen( const std::string& path, int limit = 2 ) {
        std::size_t pos = path.size() - 1;
        for ( int i = 0; i != limit; i++ ) {
            std::size_t newPos = path.rfind( '/', pos );
            if ( newPos == std::string::npos )
                return path;
            if ( newPos == 0 )
                return path;
            pos = newPos - 1;
        }
        return path.substr( pos + 2 );
    }
};

namespace rofi::log {

void info( const std::string_view& message,
    const std::source_location& location = std::source_location::current() )
{
    std::cout << "info: "
              << detail::limitPathLen( location.file_name() ) << ":"
              << location.line() << " "
              << message << '\n';
}

void warning( const std::string_view& message,
    const std::source_location& location = std::source_location::current() )
{
    std::cout << "warning: "
              << detail::limitPathLen( location.file_name() ) << ":"
              << location.line() << " "
              << message << '\n';
}

void error(const std::string_view& message,
    const std::source_location& location = std::source_location::current() )
{
    std::cout << "error: "
              << detail::limitPathLen( location.file_name() ) << ":"
              << location.line() << " "
              << message << '\n';
}

} // namespace rofi::log

