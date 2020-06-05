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

    std::lock_guard< std::mutex > lock( _rofisMutex );

    auto getNextId = [ & ]( physics::ModelPtr model ) {
        auto it = config.find( model->GetName() );
        if ( it != config.end() )
        {
            auto result = it->second;
            config.erase( it );
            return result;
        }

        while ( usedIds.find( _nextRofiId ) != usedIds.end() )
        {
            _nextRofiId++;
        }
        return _nextRofiId++;
    };

    for ( auto model : _world->Models() )
    {
        if ( !isRoFIModule( model ) )
        {
            continue;
        }

        RofiId id = getNextId( model );
        auto topic = getElemPath( model );

        _rofis.registerNewRofiId( id, topic );

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
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::GET_INFO );

    std::lock_guard< std::mutex > lock( _rofisMutex );

    for ( auto & [ id, topic ] : _rofis.getTopics() )
    {
        auto & info = *resp.add_rofiinfos();
        info.set_rofiid( id );
        info.set_topic( topic );
        info.set_lock( _rofis.isLocked( id ) );
    }

    return resp;
}

rofi::messages::DistributorResp RDP::onLockOneReq( RDP::SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::LOCK_ONE );
    resp.set_sessionid( sessionId );

    std::lock_guard< std::mutex > lock( _rofisMutex );

    auto freeId = _rofis.lockFreeRofi( sessionId );
    if ( !freeId )
    {
        return resp;
    }

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( *freeId );
    info.set_topic( _rofis.getTopic( *freeId ) );
    info.set_lock( true );

    return resp;
}

rofi::messages::DistributorResp RDP::onTryLockReq( RDP::RofiId rofiId, RDP::SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::TRY_LOCK );
    resp.set_sessionid( sessionId );

    std::lock_guard< std::mutex > lock( _rofisMutex );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );
    info.set_topic( _rofis.getTopic( rofiId ) );
    info.set_lock( _rofis.tryLockRofi( rofiId, sessionId ) );

    return resp;
}

rofi::messages::DistributorResp RDP::onUnlockReq( RDP::RofiId rofiId, RDP::SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::UNLOCK );
    resp.set_sessionid( sessionId );

    std::lock_guard< std::mutex > lock( _rofisMutex );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );

    if ( !_rofis.isRegistered( rofiId ) )
    {
        gzwarn << "Got UNLOCK distributor request without id belonging to rofi\n";
        return resp;
    }

    if ( !_rofis.isLocked( rofiId ) )
    {
        gzwarn << "Got UNLOCK distributor request without id locked\n";
        return resp;
    }

    info.set_topic( _rofis.getTopic( rofiId ) );
    info.set_lock( !_rofis.unlockRofi( rofiId, sessionId ) );

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
            _pub->Publish( onGetInfoReq(), true );
            break;
        }
        case DistributorReq::LOCK_ONE:
        {
            if ( req->rofiid() != 0 )
            {
                gzwarn << "Got LOCK_ONE distributor request with non-zero id\n";
            }

            _pub->Publish( onLockOneReq( req->sessionid() ), true );
            break;
        }
        case DistributorReq::TRY_LOCK:
        {
            _pub->Publish( onTryLockReq( req->rofiid(), req->sessionid() ), true );
            break;
        }
        case DistributorReq::UNLOCK:
        {
            _pub->Publish( onUnlockReq( req->rofiid(), req->sessionid() ), true );
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

    std::lock_guard< std::mutex > lock( _rofisMutex );

    while ( _rofis.isRegistered( _nextRofiId ) )
    {
        _nextRofiId++;
    }
    RofiId id = _nextRofiId++;

    auto topic = getElemPath( model );

    _rofis.registerNewRofiId( id, topic );

    gzmsg << "Added new RoFI '" << model->GetName() << "' with ID: " << id << "\n";
}

GZ_REGISTER_WORLD_PLUGIN( RofiDistributorPlugin )

} // namespace gazebo
