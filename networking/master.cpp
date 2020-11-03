
#include "master.hpp"

#include <gazebo/gazebo.hh>


namespace rofi::networking
{
std::unique_ptr< gazebo::Master > startMaster()
{
    std::string host; // Host is ignored for master
    unsigned port = {};
    gazebo::transport::get_master_uri( host, port );

    auto master = std::make_unique< gazebo::Master >();
    master->Init( port );
    master->RunThread();


    gazebo::common::load();

    // Initialize the informational logger. This will log warnings, and errors.
    gzLogInit( "networking-sim-", "default.log" );

    if ( !gazebo::transport::init() )
    {
        throw std::runtime_error( "Unable to initialize transport." );
    }

    // Run transport loop. Starts a thread.
    gazebo::transport::run();


    return master;
}

} // namespace rofi::networking
