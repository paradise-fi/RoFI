#include "distributor.hpp"

#include "internal_state.hpp"


using namespace rofi::networking;

using RofiId = Distributor::RofiId;
using SessionId = Distributor::SessionId;

using LockedModules = Distributor::LockedModules;


LockedModules::LockedModules( const Database & database ) : _database( database )
{
}

std::optional< RofiId > LockedModules::lockFreeRofi( SessionId sessionId )
{
    std::lock_guard< std::mutex > lock( _lockedModulesMutex );

    for ( auto & rofi : _database )
    {
        if ( _lockedModules.find( rofi.first ) != _lockedModules.end() )
        {
            continue;
        }
        if ( _lockedModules.insert( { rofi.first, std::move( sessionId ) } ).second )
        {
            return rofi.first;
        }
    }
    return false;
}

bool LockedModules::tryLockRofi( RofiId rofiId, SessionId sessionId )
{
    std::lock_guard< std::mutex > lock( _lockedModulesMutex );

    if ( !_database.hasRofi( rofiId ) )
    {
        return false;
    }
    if ( _lockedModules.find( rofiId ) != _lockedModules.end() )
    {
        return false;
    }

    return _lockedModules.insert( { rofiId, std::move( sessionId ) } ).second;
}

bool LockedModules::unlockRofi( RofiId rofiId, SessionId sessionId )
{
    std::lock_guard< std::mutex > lock( _lockedModulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it == _lockedModules.end() )
    {
        return true;
    }

    if ( it->second != sessionId )
    {
        return false;
    }

    _lockedModules.erase( it );
    return true;
}

std::optional< SessionId > LockedModules::getSessionId( RofiId rofiId ) const
{
    std::lock_guard< std::mutex > lock( _lockedModulesMutex );

    auto it = _lockedModules.find( rofiId );
    if ( it != _lockedModules.end() )
    {
        return it->second;
    }
    return {};
}

bool LockedModules::isLocked( RofiId rofiId ) const
{
    std::lock_guard< std::mutex > lock( _lockedModulesMutex );
    return _lockedModules.find( rofiId ) != _lockedModules.end();
}


Distributor::Distributor( const Database & database, const std::string & worldName )
        : _database( database )
        , _lockedModules( database )
{
    _node = boost::make_shared< gazebo::transport::Node >();
    if ( !_node )
    {
        throw std::runtime_error( "Could not create new Node" );
    }
    _node->Init( worldName + "/distributor" );
    assert( _node->IsInitialized() );

    _pub = _node->Advertise< rofi::messages::DistributorResp >( "~/response" );
    if ( !_pub )
    {
        throw std::runtime_error( "Publisher could not be created" );
    }
    _sub = _node->Subscribe( "~/request", &Distributor::onRequestCallback, this );
    if ( !_sub )
    {
        throw std::runtime_error( "Subcriber could not be created" );
    }
}

void Distributor::onRequest( const rofi::messages::DistributorReq & req )
{
    using rofi::messages::DistributorReq;

    switch ( req.reqtype() )
    {
        case DistributorReq::NO_REQ:
        {
            break;
        }
        case DistributorReq::GET_INFO:
        {
            if ( req.rofiid() != 0 )
            {
                std::cerr << "Got GET_INFO distributor request with non-zero id\n";
            }
            _pub->Publish( onGetInfoReq(), true );
            break;
        }
        case DistributorReq::LOCK_ONE:
        {
            if ( req.rofiid() != 0 )
            {
                std::cerr << "Got LOCK_ONE distributor request with non-zero id\n";
            }

            _pub->Publish( onLockOneReq( req.sessionid() ), true );
            break;
        }
        case DistributorReq::TRY_LOCK:
        {
            _pub->Publish( onTryLockReq( req.rofiid(), req.sessionid() ), true );
            break;
        }
        case DistributorReq::UNLOCK:
        {
            _pub->Publish( onUnlockReq( req.rofiid(), req.sessionid() ), true );
            break;
        }
        default:
        {
            std::cerr << "Unknown distributor request type: " << req.reqtype() << "\n";
            break;
        }
    }
}

rofi::messages::DistributorResp Distributor::onGetInfoReq()
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::GET_INFO );

    for ( auto & rofi : _database )
    {
        auto & info = *resp.add_rofiinfos();
        info.set_rofiid( rofi.first );
        info.set_topic( InternalState::getTopic( rofi.first ) );
        info.set_lock( _lockedModules.isLocked( rofi.first ) );
    }

    return resp;
}

rofi::messages::DistributorResp Distributor::onLockOneReq( SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::LOCK_ONE );
    resp.set_sessionid( sessionId );

    auto freeId = _lockedModules.lockFreeRofi( sessionId );
    if ( !freeId )
    {
        return resp;
    }

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( *freeId );
    info.set_topic( InternalState::getTopic( *freeId ) );
    info.set_lock( true );

    return resp;
}

rofi::messages::DistributorResp Distributor::onTryLockReq( RofiId rofiId, SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::TRY_LOCK );
    resp.set_sessionid( sessionId );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );
    info.set_topic( InternalState::getTopic( rofiId ) );
    info.set_lock( _lockedModules.tryLockRofi( rofiId, sessionId ) );

    return resp;
}

rofi::messages::DistributorResp Distributor::onUnlockReq( RofiId rofiId, SessionId sessionId )
{
    rofi::messages::DistributorResp resp;
    resp.set_resptype( rofi::messages::DistributorReq::UNLOCK );
    resp.set_sessionid( sessionId );

    auto & info = *resp.add_rofiinfos();
    info.set_rofiid( rofiId );

    if ( !_lockedModules.isLocked( rofiId ) )
    {
        std::cerr << "Got UNLOCK distributor request without id locked\n";
        return resp;
    }

    info.set_topic( InternalState::getTopic( rofiId ) );
    info.set_lock( !_lockedModules.unlockRofi( rofiId, sessionId ) );

    return resp;
}
