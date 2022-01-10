#pragma once

#include <memory>
#include <string>
#include <string_view>

#include <gazebo/Master.hh>
#include <gazebo/gazebo_client.hh>


namespace rofi::msgs
{
class [[nodiscard]] Server
{
public:
    static Server createAndLoopInThread( std::string_view logName = "default" );

private:
    std::unique_ptr< gazebo::Master > _impl;
};


class [[nodiscard]] Client
{
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
    Client( const Client & ) = delete;
    Client & operator=( const Client & ) = delete;

    ~Client()
    {
        gazebo::client::shutdown();
    }
};

} // namespace rofi::msgs
