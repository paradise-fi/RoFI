#pragma once

#include <gazebo/transport/transport.hh>

#include "command_handler.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>
#include <rofiCmd.pb.h>


namespace rofi::simplesim
{
class ModulesCommunication;

class Distributor
{
    template < typename T >
    using remove_cvref_t = std::remove_cv_t< std::remove_reference_t< T > >;

public:
    using SessionId = remove_cvref_t< decltype( rofi::messages::DistributorReq().sessionid() ) >;

    // Initializes the Distributor
    // Make sure, that Gazebo communication is running before calling this
    Distributor( gazebo::transport::Node & node, ModulesCommunication & modulesCommunication );

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
    rofi::messages::DistributorResp onTryLockReq( ModuleId moduleId, SessionId sessionId );
    rofi::messages::DistributorResp onUnlockReq( ModuleId moduleId, SessionId sessionId );


    ModulesCommunication & _modulesCommunication;

    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};
} // namespace rofi::simplesim
