#include "distributorPlugin.hpp"

#include <algorithm>

#include <gz/plugin/Register.hh>
#include <gz/sim/Model.hh>

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

std::optional< RofiDatabase::RofiId > RofiDatabase::lockFreeRofi( SessionId sessionId )
{
    auto it = _freeRofis.begin();
    if ( it == _freeRofis.end() )
    {
        return {};
    }
    auto rofiId = *it;
    _freeRofis.erase( it );
    _lockedRofis[ rofiId ] = std::move( sessionId );
    return rofiId;
}

bool RofiDatabase::tryLockRofi( RofiId rofiId, SessionId sessionId )
{
    auto it = _freeRofis.find( rofiId );
    if ( it == _freeRofis.end() )
    {
        return false;
    }
    _freeRofis.erase( it );
    _lockedRofis[ rofiId ] = std::move( sessionId );
    return true;
}

bool RofiDatabase::unlockRofi( RofiId rofiId, SessionId sessionId )
{
    auto it = _lockedRofis.find( rofiId );
    if ( it == _lockedRofis.end() )
    {
        return true;
    }
    if ( it->second != sessionId )
    {
        return false;
    }
    _lockedRofis.erase( it );
    _freeRofis.insert( rofiId );
    return true;
}

std::optional< RofiDatabase::SessionId > RofiDatabase::getSessionId( RofiId rofiId ) const
{
    auto it = _lockedRofis.find( rofiId );
    return it == _lockedRofis.end() ? std::optional< SessionId >() : std::optional< SessionId >( it->second );
}

std::string RofiDatabase::getTopic( RofiId rofiId ) const
{
    auto it = _rofiTopics.find( rofiId );
    return it == _rofiTopics.end() ? std::string() : it->second;
}

namespace gazebo
{
void RofiDistributorPlugin::Configure( const gz::sim::Entity & entity,
                                       const std::shared_ptr< const sdf::Element > & sdf,
                                       gz::sim::EntityComponentManager & ecm,
                                       gz::sim::EventManager & )
{
    _worldEntity = entity;
    _world = gz::sim::World( entity );
    _sdf = sdf;

    _node = std::make_shared< rofi::gz::Node >();
    _node->Init( _world.Name( ecm ).value_or( "default" ) + "/distributor" );
    _pub = _node->Advertise< rofi::messages::DistributorResp >( "~/response" );
    _sub = _node->Subscribe( "~/request", &RofiDistributorPlugin::onRequest, this );

    loadRofis( ecm );
}

void RofiDistributorPlugin::PostUpdate( const gz::sim::UpdateInfo &,
                                        const gz::sim::EntityComponentManager & ecm )
{
    refreshRofis( ecm );
}

void RofiDistributorPlugin::loadRofis( const gz::sim::EntityComponentManager & ecm )
{
    auto config = getRofisFromSdf();
    std::set< RofiId > usedIds;
    for ( const auto & [ _, id ] : config )
    {
        usedIds.insert( id );
    }

    std::lock_guard lock( _rofisMutex );
    for ( auto model : _world.Models( ecm ) )
    {
        if ( !isRoFIModule( model, ecm ) )
        {
            continue;
        }

        auto modelName = gz::sim::Model( model ).Name( ecm );
        auto it = config.find( modelName );
        RofiId id = 0;
        if ( it != config.end() )
        {
            id = it->second;
            config.erase( it );
        }
        else
        {
            while ( usedIds.contains( _nextRofiId ) || _rofis.isRegistered( _nextRofiId ) )
            {
                ++_nextRofiId;
            }
            id = _nextRofiId++;
        }

        if ( _rofis.isRegistered( id ) )
        {
            continue;
        }

        _rofis.registerNewRofiId( id, "/gazebo/" + getElemPath( model, ecm ) );
    }
}

void RofiDistributorPlugin::refreshRofis( const gz::sim::EntityComponentManager & ecm )
{
    std::lock_guard lock( _rofisMutex );
    for ( auto model : _world.Models( ecm ) )
    {
        if ( !isRoFIModule( model, ecm ) )
        {
            continue;
        }

        const auto topic = "/gazebo/" + getElemPath( model, ecm );
        const bool alreadyKnown = std::any_of(
                _rofis.getTopics().begin(),
                _rofis.getTopics().end(),
                [ & ]( const auto & entry ) { return entry.second == topic; } );
        if ( alreadyKnown )
        {
            continue;
        }

        while ( _rofis.isRegistered( _nextRofiId ) )
        {
            ++_nextRofiId;
        }
        _rofis.registerNewRofiId( _nextRofiId++, topic );
    }
}

std::map< std::string, RofiDistributorPlugin::RofiId > RofiDistributorPlugin::getRofisFromSdf() const
{
    std::map< std::string, RofiId > config;
    auto sdf = std::const_pointer_cast< sdf::Element >( _sdf );
    checkChildrenNames( sdf, { "rofi" } );
    for ( auto rofiSdf : getChildren( sdf, "rofi" ) )
    {
        RofiId id = getOnlyChild< true >( rofiSdf, "id" )->Get< int >();
        std::string name = getOnlyChild< true >( rofiSdf, "name" )->Get< std::string >();
        config.emplace( std::move( name ), id );
    }
    return config;
}

void RofiDistributorPlugin::onRequest( const RequestPtr & req )
{
    if ( !req )
    {
        return;
    }

    using rofi::messages::DistributorReq;
    switch ( req->reqtype() )
    {
        case DistributorReq::GET_INFO:
            _pub->Publish( onGetInfoReq(), true );
            break;
        case DistributorReq::LOCK_ONE:
            _pub->Publish( onLockOneReq( req->sessionid() ), true );
            break;
        case DistributorReq::TRY_LOCK:
            _pub->Publish( onTryLockReq( req->rofiid(), req->sessionid() ), true );
            break;
        case DistributorReq::UNLOCK:
            _pub->Publish( onUnlockReq( req->rofiid(), req->sessionid() ), true );
            break;
        default:
            break;
    }
}

sdf::ElementPtr RofiDistributorPlugin::createRofiElem( RofiId id, const std::string & name )
{
    auto rofiElem = newElement( "rofi" );
    insertElement( rofiElem, newElemWithValue( "name", name ) );
    insertElement( rofiElem, newElemWithValue( "id", id ) );
    return rofiElem;
}

rofi::messages::DistributorResp RofiDistributorPlugin::onGetInfoReq()
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::GET_INFO );

    std::lock_guard lock( _rofisMutex );
    for ( const auto & [ id, topic ] : _rofis.getTopics() )
    {
        auto & info = *resp.add_rofiinfos();
        info.set_rofiid( id );
        info.set_topic( topic );
        info.set_lock( _rofis.isLocked( id ) );
    }
    return resp;
}

rofi::messages::DistributorResp RofiDistributorPlugin::onLockOneReq( SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::LOCK_ONE );
    resp.set_sessionid( sessionId );

    std::lock_guard lock( _rofisMutex );
    if ( auto freeId = _rofis.lockFreeRofi( sessionId ) )
    {
        auto & info = *resp.add_rofiinfos();
        info.set_rofiid( *freeId );
        info.set_topic( _rofis.getTopic( *freeId ) );
        info.set_lock( true );
    }
    return resp;
}

rofi::messages::DistributorResp RofiDistributorPlugin::onTryLockReq( RofiId rofiId,
                                                                     SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::TRY_LOCK );
    resp.set_sessionid( sessionId );

    std::lock_guard lock( _rofisMutex );
    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );
    info.set_topic( _rofis.getTopic( rofiId ) );
    info.set_lock( _rofis.tryLockRofi( rofiId, sessionId ) );
    return resp;
}

rofi::messages::DistributorResp RofiDistributorPlugin::onUnlockReq( RofiId rofiId,
                                                                    SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::UNLOCK );
    resp.set_sessionid( sessionId );

    std::lock_guard lock( _rofisMutex );
    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );
    info.set_topic( _rofis.getTopic( rofiId ) );
    info.set_lock( !_rofis.unlockRofi( rofiId, sessionId ) );
    return resp;
}

} // namespace gazebo

GZ_ADD_PLUGIN( gazebo::RofiDistributorPlugin,
               gz::sim::System,
               gz::sim::ISystemConfigure,
               gz::sim::ISystemPostUpdate )
GZ_ADD_PLUGIN_ALIAS( gazebo::RofiDistributorPlugin, "rofi::sim::systems::RofiDistributorPlugin" )
