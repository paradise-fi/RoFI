#include "distributorPlugin.hpp"

#include <distributorResp.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace gazebo
{
using RDP = RofiDistributorPlugin;

void RDP::Load( physics::WorldPtr world, sdf::ElementPtr sdf )
{
    _world = std::move( world );
    assert( _world );

    _node = boost::make_shared< transport::Node >();
    if ( !_node )
    {
        gzerr << "Could not create new Node\n";
        throw std::runtime_error( "Could not create new Node" );
    }
    _node->Init( _world->Name() + "/distributor" );
    assert( _node->IsInitialized() );

    loadRofis( sdf );

    _pubOut = _node->Advertise< rofi::messages::DistributorResp >( "~/response" );
    if ( !_pubOut )
    {
        gzerr << "Publisher could not be created\n";
        throw std::runtime_error( "Publisher could not be created" );
    }

    _subOut = _node->Subscribe( "~/request", &RDP::onRequest, this );
    if ( !_subOut )
    {
        gzerr << "Subcriber could not be created\n";
        throw std::runtime_error( "Subcriber could not be created" );
    }

    _onAddEntityConnection =
            event::Events::ConnectAddEntity( [ this ]( auto added ) { onAddEntity( added ); } );

    gzmsg << "RoFI distributor running\n";
}

void RDP::loadRofis( sdf::ElementPtr pluginSdf )
{
    auto config = loadRofisFromSdf( pluginSdf );

    std::set< RofiId > usedIds;
    for ( auto & elem : config )
    {
        auto inserted = usedIds.insert( elem.second ).second;
        if ( !inserted )
        {
            gzerr << "Two rofis with same ID: " << elem.second << "\n";
            std::runtime_error( "Two rofis with same ID: " + std::to_string( elem.second ) );
        }
    }

    std::lock_guard< std::mutex > lock( _rofiTopicsMutex );

    for ( auto model : _world->Models() )
    {
        if ( !isRoFIModule( model ) )
        {
            continue;
        }

        RofiId id = {};
        auto it = config.find( model->GetName() );
        if ( it != config.end() )
        {
            id = it->second;
            config.erase( it );
        }
        else
        {
            while ( usedIds.find( _nextRofiId ) != usedIds.end() )
            {
                _nextRofiId++;
            }
            id = _nextRofiId++;
        }

        auto topic = getElemPath( model );

        [[maybe_unused]] bool inserted = _rofiTopics.insert( { id, topic } ).second;
        assert( inserted );

        gzmsg << "Loaded RoFI '" << model->GetName() << "' with ID: " << id << "\n";
    }

    for ( auto elem : config )
    {
        gzerr << "Could not find RoFI module '" << elem.first << "'\n";
    }
}

std::map< std::string, RDP::RofiId > RDP::loadRofisFromSdf( sdf::ElementPtr pluginSdf )
{
    assert( pluginSdf );

    std::map< std::string, RofiId > config;

    checkChildrenNames( pluginSdf, { "rofi" } );
    for ( auto rofiSdf : getChildren( pluginSdf, "rofi" ) )
    {
        checkChildrenNames( rofiSdf, { "id", "name" } );
        RofiId id = getOnlyChild< true >( rofiSdf, "id" )->Get< int >();
        std::string name = getOnlyChild< true >( rofiSdf, "name" )->Get< std::string >();

        auto inserted = config.insert( { name, id } ).second;
        if ( !inserted )
        {
            gzerr << "Two rofis with same name: '" << name << "'\n";
            std::runtime_error( "Two rofis with same name: '" + name + "'" );
        }
    }

    return config;
}

void RDP::onRequest( const RequestPtr & req )
{
    using rofi::messages::DistributorReq;

    switch ( req->reqtype() )
    {
        case DistributorReq::GET_INFO:
        {
            gzmsg << "GET_INFO\n";
            break;
        }
        case DistributorReq::TRY_LOCK:
        {
            gzmsg << "TRY_LOCK\n";
            break;
        }
        case DistributorReq::UNLOCK:
        {
            gzmsg << "UNLOCK\n";
            break;
        }
        default:
        {
            gzwarn << "Unknown distributor request type: " << req->reqtype() << "\n";
            break;
        }
    }
}

void RDP::onAddEntity( std::string added )
{
    assert( _world );

    // World::ModelByName freezes the simulation
    auto model = boost::dynamic_pointer_cast< physics::Model >( _world->EntityByName( added ) );
    if ( !model || !isRoFIModule( model ) )
    {
        return;
    }

    std::lock_guard< std::mutex > lock( _rofiTopicsMutex );

    while ( _rofiTopics.find( _nextRofiId ) != _rofiTopics.end() )
    {
        _nextRofiId++;
    }
    RofiId id = _nextRofiId++;

    auto topic = getElemPath( model );

    [[maybe_unused]] bool inserted = _rofiTopics.insert( { id, topic } ).second;
    assert( inserted );

    gzmsg << "Added new RoFI '" << model->GetName() << "' with ID: " << id << "\n";
}

GZ_REGISTER_WORLD_PLUGIN( RofiDistributorPlugin )

} // namespace gazebo
