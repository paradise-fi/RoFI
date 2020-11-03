#pragma once

#include <map>
#include <optional>
#include <string>

#include <gazebo/transport/transport.hh>

#include "database.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>


namespace rofi::networking
{
class Distributor
{
public:
    using RofiId = Database::RofiId;
    using SessionId = std::string;
    using RequestPtr = boost::shared_ptr< const rofi::messages::DistributorReq >;

    class LockedModules
    {
    public:
        LockedModules( const Database & database );

        std::optional< RofiId > lockFreeRofi( SessionId sessionId );
        bool tryLockRofi( RofiId rofiId, SessionId sessionId );
        bool unlockRofi( RofiId rofiId, SessionId sessionId );
        std::optional< SessionId > getSessionId( RofiId rofiId ) const;

        bool isLocked( RofiId rofiId ) const;

    private:
        const Database & _database;

        mutable std::mutex _lockedModulesMutex;
        std::map< RofiId, SessionId > _lockedModules;
    };

    // Initializes the Distributor
    // Make sure, that Gazebo communication is running
    Distributor( const Database & database, const std::string & worldName = "default" );

    Distributor( const Distributor & ) = delete;
    Distributor & operator=( const Distributor & ) = delete;

    void onRequest( const rofi::messages::DistributorReq & req );
    void onRequestCallback( const RequestPtr & req )
    {
        auto reqPtrCopy = req;
        onRequest( *reqPtrCopy );
    }

    rofi::messages::DistributorResp onGetInfoReq();
    rofi::messages::DistributorResp onLockOneReq( SessionId sessionId );
    rofi::messages::DistributorResp onTryLockReq( RofiId rofiId, SessionId sessionId );
    rofi::messages::DistributorResp onUnlockReq( RofiId rofiId, SessionId sessionId );

private:
    gazebo::transport::NodePtr _node;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;

    const Database & _database;

    LockedModules _lockedModules;
};

} // namespace rofi::networking
