#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <gazebo/Master.hh>
#include <gazebo/gazebo_client.hh>


namespace rofi::msgs
{
inline std::vector< char * > getCStyleArgs( char * progName, std::span< std::string > args )
{
    auto cArgs = std::vector< char * >();
    cArgs.reserve( args.size() + 1 );
    cArgs.push_back( progName );
    std::transform( args.begin(), args.end(), std::back_inserter( cArgs ), []( std::string & str ) {
        return str.data();
    } );

    return cArgs;
}

class [[nodiscard]] Server {
public:
    static Server createAndLoopInThread( std::string_view logName = "default" );

private:
    std::unique_ptr< gazebo::Master > _impl;
};


class [[nodiscard]] Client {
public:
    Client()
    {
        if ( !gazebo::client::setup() ) {
            throw std::runtime_error( "Could not setup client" );
        }
    }
    Client( int argc, char ** argv )
    {
        if ( !gazebo::client::setup( argc, argv ) ) {
            throw std::runtime_error( "Could not setup client" );
        }
    }
    /**
     * \brief Simulate the `argc, argv` arguments from span of strings.
     *
     * Not using `vector<string>` overload of `client::setup`,
     * because it does more work than it needs to.
     */
    Client( std::string progName, std::span< std::string > args )
    {
        auto cArgs = getCStyleArgs( progName.data(), args );
        if ( !gazebo::client::setup( static_cast< int >( cArgs.size() ), cArgs.data() ) ) {
            throw std::runtime_error( "Could not setup client" );
        }
    }
    Client( const Client & ) = delete;
    Client & operator=( const Client & ) = delete;

    ~Client()
    {
        gazebo::client::shutdown();
    }
};

} // namespace rofi::msgs
