
#include "master.hpp"

#include <gazebo/gazebo.hh>
#include <gazebo/gazebo_client.hh>


boost::mutex fini_mutex;
std::vector< gazebo::SystemPluginPtr > g_plugins;

gazebo::Master * g_master = NULL;


bool gazebo_shared_setup( const std::string & _prefix,
                          int _argc,
                          char ** _argv,
                          std::vector< gazebo::SystemPluginPtr > & _plugins )
{
    gazebo::common::load();

    // The SDF find file callback.
    sdf::setFindCallback( boost::bind( &gazebo::common::find_file, _1 ) );

    // Initialize the informational logger. This will log warnings, and
    // errors.
    gzLogInit( _prefix, "default.log" );

    // Load all the system plugins
    for ( auto & plugin : _plugins )
    {
        plugin->Load( _argc, _argv );
    }

    if ( !gazebo::transport::init() )
    {
        gzerr << "Unable to initialize transport.\n";
        return false;
    }

    // // Make sure the model database has started.
    // gazebo::common::ModelDatabase::Instance()->Start();

    // Run transport loop. Starts a thread
    gazebo::transport::run();

    // Init all system plugins
    for ( auto & plugin : _plugins )
    {
        plugin->Init();
    }

    return true;
}

bool startMaster( int argc, char ** argv )
{
    std::string host = "";
    unsigned int port = 0;

    gazebo::transport::get_master_uri( host, port );

    g_master = new gazebo::Master();
    g_master->Init( port );
    g_master->RunThread();

    if ( !gazebo_shared_setup( "server-", argc, argv, g_plugins ) )
    {
        gzerr << "Unable to setup Gazebo master\n";
        return false;
    }

    return true;
}
