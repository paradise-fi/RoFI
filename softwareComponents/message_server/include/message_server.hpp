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
        int argc = static_cast< int >( args.size() ) + 1;

        auto argv = std::vector< char * >();
        argv.reserve( argc );
        argv.push_back( progName.data() );
        std::transform( args.begin(),
                        args.end(),
                        std::back_inserter( argv ),
                        []( std::string & str ) { return str.data(); } );

        assert( args.size() == static_cast< size_t >( argc ) );
        if ( !gazebo::client::setup( argc, argv.data() ) ) {
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
