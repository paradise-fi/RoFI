#include "distributorPlugin.hpp"

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

    _pub = _node->Advertise< rofi::messages::DistributorResp >( "~/response" );
    if ( !_pub )
    {
        gzerr << "Publisher could not be created\n";
        throw std::runtime_error( "Publisher could not be created" );
    }

    _sub = _node->Subscribe( "~/request", &RDP::onRequest, this );
    if ( !_sub )
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

    std::lock_guard< std::mutex > lock( _rofiInfoMutex );

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
        _freeRofiIds.insert( id );

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


rofi::messages::DistributorResp RDP::onGetInfoReq()
{
    std::lock_guard< std::mutex > lock( _rofiInfoMutex );

    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::GET_INFO );

    for ( const auto & [ id, topic ] : _rofiTopics )
    {
        auto & info = *resp.add_rofiinfos();
        info.set_rofiid( id );
        info.set_topic( topic );
        info.set_lock( _freeRofiIds.find( id ) == _freeRofiIds.end() );
    }

    return resp;
}

rofi::messages::DistributorResp RDP::onLockOneReq()
{
    std::lock_guard< std::mutex > lock( _rofiInfoMutex );

    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::LOCK_ONE );

    if ( _freeRofiIds.empty() )
    {
        return resp;
    }
    RofiId id = *_freeRofiIds.begin();
    _freeRofiIds.erase( _freeRofiIds.begin() );

    assert( _rofiTopics.find( id ) != _rofiTopics.end() );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( id );
    info.set_topic( _rofiTopics[ id ] );
    info.set_lock( true );

    return resp;
}

rofi::messages::DistributorResp RDP::onTryLockReq( RDP::RofiId rofiId )
{
    std::lock_guard< std::mutex > lock( _rofiInfoMutex );

    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::TRY_LOCK );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );
    auto topicIt = _rofiTopics.find( rofiId );
    if ( topicIt != _rofiTopics.end() )
    {
        info.set_topic( topicIt->second );
    }
    auto freeRofiIt = _freeRofiIds.find( rofiId );
    if ( freeRofiIt != _freeRofiIds.end() )
    {
        _freeRofiIds.erase( freeRofiIt );
        info.set_lock( true );
    }
    else
    {
        info.set_lock( false );
    }

    return resp;
}

rofi::messages::DistributorResp RDP::onUnlockReq( RDP::RofiId rofiId )
{
    std::lock_guard< std::mutex > lock( _rofiInfoMutex );

    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::UNLOCK );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );

    if ( _rofiTopics.find( rofiId ) == _rofiTopics.end() )
    {
        gzwarn << "Got UNLOCK distributor request without id belonging to rofi\n";
        return resp;
    }

    if ( _freeRofiIds.find( rofiId ) != _freeRofiIds.end() )
    {
        gzwarn << "Got UNLOCK distributor request with already free id\n";
        return resp;
    }

    info.set_topic( _rofiTopics[ rofiId ] );
    info.set_lock( false );
    _freeRofiIds.insert( rofiId );

    return resp;
}

void RDP::onRequest( const RequestPtr & req )
{
    using rofi::messages::DistributorReq;

    switch ( req->reqtype() )
    {
        case DistributorReq::GET_INFO:
        {
            if ( req->rofiid() != 0 )
            {
                gzwarn << "Got GET_INFO distributor request with non-zero id\n";
            }
            auto resp = onGetInfoReq();
            resp.set_sessionid( req->sessionid() );
            _pub->Publish( resp, true );
            break;
        }
        case DistributorReq::LOCK_ONE:
        {
            if ( req->rofiid() != 0 )
            {
                gzwarn << "Got LOCK_ONE distributor request with non-zero id\n";
            }

            auto resp = onLockOneReq();
            resp.set_sessionid( req->sessionid() );
            _pub->Publish( resp, true );
            break;
        }
        case DistributorReq::TRY_LOCK:
        {
            auto resp = onTryLockReq( req->rofiid() );
            resp.set_sessionid( req->sessionid() );
            _pub->Publish( resp, true );
            break;
        }
        case DistributorReq::UNLOCK:
        {
            auto resp = onUnlockReq( req->rofiid() );
            resp.set_sessionid( req->sessionid() );
            _pub->Publish( resp, true );
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

    std::lock_guard< std::mutex > lock( _rofiInfoMutex );

    while ( _rofiTopics.find( _nextRofiId ) != _rofiTopics.end() )
    {
        _nextRofiId++;
    }
    RofiId id = _nextRofiId++;

    auto topic = getElemPath( model );

    [[maybe_unused]] bool inserted = _rofiTopics.insert( { id, topic } ).second;
    assert( inserted );
    _freeRofiIds.insert( id );

    gzmsg << "Added new RoFI '" << model->GetName() << "' with ID: " << id << "\n";
}

GZ_REGISTER_WORLD_PLUGIN( RofiDistributorPlugin )

} // namespace gazebo
