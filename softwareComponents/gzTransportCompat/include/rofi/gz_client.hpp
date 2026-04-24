#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rofi::gz
{
inline std::vector< char * > getCStyleArgs( char * progName, std::span< std::string > args )
{
    auto cArgs = std::vector< char * >();
    cArgs.reserve( args.size() + 1 );
    cArgs.push_back( progName );
    for ( std::string & arg : args ) {
        cArgs.push_back( arg.data() );
    }
    return cArgs;
}

struct [[nodiscard]] Server {
    static Server createAndLoopInThread( std::string_view /* logName */ = "default" )
    {
        return {};
    }
};

class [[nodiscard]] Client {
public:
    Client() = default;
    Client( int /* argc */, char ** /* argv */ )
    {}
    Client( std::string /* progName */, std::span< std::string > /* args */ )
    {}
};
} // namespace rofi::gz
