#pragma once

#include <gazebo/transport/transport.hh>

#include "modules_info.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>


namespace rofi::networking
{
class Distributor
{
public:
    using RofiId = ModulesInfo::RofiId;
    using SessionId = ModulesInfo::SessionId;

    // Initializes the Distributor
    // Make sure, that Gazebo communication is running before calling this
    Distributor( gazebo::transport::Node & node, ModulesInfo & modulesInfo );

    Distributor( const Distributor & ) = delete;
    Distributor & operator=( const Distributor & ) = delete;

private:
    void onRequest( const rofi::messages::DistributorReq & req );
    void onRequestCallback( const boost::shared_ptr< const rofi::messages::DistributorReq > & req )
    {
        auto reqCopy = req;
        onRequest( *reqCopy );
    }

    rofi::messages::DistributorResp onGetInfoReq();
    rofi::messages::DistributorResp onLockOneReq( SessionId sessionId );
    rofi::messages::DistributorResp onTryLockReq( RofiId rofiId, SessionId sessionId );
    rofi::messages::DistributorResp onUnlockReq( RofiId rofiId, SessionId sessionId );


    ModulesInfo & _modulesInfo;

    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};
} // namespace rofi::networking
