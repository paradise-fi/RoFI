#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <set>

#include <boost/shared_ptr.hpp>
#include <gz/sim/System.hh>
#include <gz/sim/World.hh>

#include <rofi/gz_transport.hpp>

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
        _rofiTopics.emplace( rofiId, std::move( topic ) );
        _freeRofis.insert( rofiId );
    }

    std::optional< RofiId > lockFreeRofi( SessionId sessionId );
    bool tryLockRofi( RofiId rofiId, SessionId sessionId );
    bool unlockRofi( RofiId rofiId, SessionId sessionId );

    std::optional< SessionId > getSessionId( RofiId rofiId ) const;
    std::string getTopic( RofiId rofiId ) const;
    const auto & getTopics() const { return _rofiTopics; }
    bool isRegistered( RofiId rofiId ) const { return _rofiTopics.contains( rofiId ); }
    bool isLocked( RofiId rofiId ) const { return _freeRofis.find( rofiId ) == _freeRofis.end(); }

private:
    std::map< RofiId, std::string > _rofiTopics;
    std::map< RofiId, SessionId > _lockedRofis;
    std::set< RofiId > _freeRofis;
};

namespace gazebo
{
class RofiDistributorPlugin : public gz::sim::System,
                              public gz::sim::ISystemConfigure,
                              public gz::sim::ISystemPostUpdate
{
    using RequestPtr = boost::shared_ptr< const rofi::messages::DistributorReq >;
    using RofiId = RofiDatabase::RofiId;
    using SessionId = RofiDatabase::SessionId;

public:
    void Configure( const gz::sim::Entity & entity,
                    const std::shared_ptr< const sdf::Element > & sdf,
                    gz::sim::EntityComponentManager & ecm,
                    gz::sim::EventManager & eventMgr ) override;

    void PostUpdate( const gz::sim::UpdateInfo & info,
                     const gz::sim::EntityComponentManager & ecm ) override;

    void loadRofis( const gz::sim::EntityComponentManager & ecm );
    void refreshRofis( const gz::sim::EntityComponentManager & ecm );
    std::map< std::string, RofiId > getRofisFromSdf() const;

    void onRequest( const RequestPtr & req );
    static sdf::ElementPtr createRofiElem( RofiId id, const std::string & name );

private:
    rofi::messages::DistributorResp onGetInfoReq();
    rofi::messages::DistributorResp onLockOneReq( SessionId sessionId );
    rofi::messages::DistributorResp onTryLockReq( RofiId rofiId, SessionId sessionId );
    rofi::messages::DistributorResp onUnlockReq( RofiId rofiId, SessionId sessionId );

    gz::sim::Entity _worldEntity = gz::sim::kNullEntity;
    gz::sim::World _world;
    std::shared_ptr< const sdf::Element > _sdf;

    rofi::gz::NodePtr _node;
    rofi::gz::PublisherPtr _pub;
    rofi::gz::SubscriberPtr _sub;

    RofiDatabase _rofis;
    int _nextRofiId = 1;
    std::mutex _rofisMutex;
};

} // namespace gazebo
