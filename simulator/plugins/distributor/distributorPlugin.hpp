#pragma once

#include <map>
#include <set>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "utils.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>


class RofiDatabase
{
public:
    using RofiId = int;
    using SessionId = std::string;

    void registerNewRofiId( RofiId rofiId, std::string topic )
    {
        if ( _rofiTopics.find( rofiId ) != _rofiTopics.end() )
        {
            gzerr << "RoFI with this ID already exists (ID: " << rofiId << ", topic: " << topic
                  << ")\n";
            return;
        }
        assert( _lockedRofis.find( rofiId ) == _lockedRofis.end() );
        assert( _freeRofis.find( rofiId ) == _freeRofis.end() );

        _rofiTopics[ rofiId ] = std::move( topic );
        _freeRofis.insert( rofiId );
    }

    std::optional< RofiId > lockFreeRofi( SessionId sessionId )
    {
        auto it = _freeRofis.begin();
        if ( it == _freeRofis.end() )
        {
            return {};
        }
        RofiId rofiId = *it;
        _freeRofis.erase( it );

        _lockedRofis[ rofiId ] = sessionId;
        return rofiId;
    }

    bool tryLockRofi( RofiId rofiId, SessionId sessionId )
    {
        auto it = _freeRofis.find( rofiId );
        if ( it == _freeRofis.end() )
        {
            return false;
        }

        assert( _lockedRofis.find( rofiId ) == _lockedRofis.end() );
        _freeRofis.erase( it );
        _lockedRofis[ rofiId ] = sessionId;
        return true;
    }

    bool unlockRofi( RofiId rofiId, SessionId sessionId )
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

    std::optional< SessionId > getSessionId( RofiId rofiId ) const
    {
        auto it = _lockedRofis.find( rofiId );
        if ( it != _lockedRofis.end() )
        {
            return it->second;
        }
        return {};
    }

    std::string getTopic( RofiId rofiId ) const
    {
        auto it = _rofiTopics.find( rofiId );
        if ( it != _rofiTopics.end() )
        {
            return it->second;
        }
        return {};
    }

    auto & getTopics() const
    {
        return _rofiTopics;
    }

    bool isRegistered( RofiId rofiId ) const
    {
        return _rofiTopics.find( rofiId ) != _rofiTopics.end();
    }

    bool isLocked( RofiId rofiId ) const
    {
        assert( _rofiTopics.find( rofiId ) != _rofiTopics.end() );
        bool result = _freeRofis.find( rofiId ) == _freeRofis.end();
        assert( result == ( _lockedRofis.find( rofiId ) != _lockedRofis.end() ) );
        return result;
    }

private:
    std::map< RofiId, std::string > _rofiTopics;
    std::map< RofiId, SessionId > _lockedRofis;
    std::set< RofiId > _freeRofis;
};


namespace gazebo
{
class RofiDistributorPlugin : public WorldPlugin
{
    using RequestPtr = boost::shared_ptr< const rofi::messages::DistributorReq >;
    using RofiId = RofiDatabase::RofiId;
    using SessionId = RofiDatabase::SessionId;

public:
    RofiDistributorPlugin() = default;

    RofiDistributorPlugin( const RofiDistributorPlugin & ) = delete;
    RofiDistributorPlugin & operator=( const RofiDistributorPlugin & ) = delete;

    void Load( physics::WorldPtr world, sdf::ElementPtr sdf ) override;

    void loadRofis( sdf::ElementPtr pluginSdf );
    std::map< std::string, RofiId > loadRofisFromSdf( sdf::ElementPtr pluginSdf );

    void onRequest( const RequestPtr & req );
    void onAddEntity( std::string added );

private:
    rofi::messages::DistributorResp onGetInfoReq();
    rofi::messages::DistributorResp onLockOneReq( SessionId sessionId );
    rofi::messages::DistributorResp onTryLockReq( RofiId rofiId, SessionId sessionId );
    rofi::messages::DistributorResp onUnlockReq( RofiId rofiId, SessionId sessionId );

    physics::WorldPtr _world;

    transport::NodePtr _node;
    transport::PublisherPtr _pub;
    transport::SubscriberPtr _sub;

    event::ConnectionPtr _onAddEntityConnection;

    RofiDatabase _rofis;
    int _nextRofiId = 1;
    std::mutex _rofisMutex;
};

} // namespace gazebo
