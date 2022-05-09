#pragma once

#include <cassert>
#include <thread>

#include <gazebo/transport/transport.hh>

#include <message_logger.hpp>

#include "command_handler.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>
#include <rofiCmd.pb.h>


namespace rofi::simplesim
{
class ModulesCommunication;

class Distributor {
    template < typename T >
    using remove_cvref_t = std::remove_cv_t< std::remove_reference_t< T > >;

public:
    using SessionId = remove_cvref_t< decltype( rofi::messages::DistributorReq().sessionid() ) >;

    // Initializes the Distributor
    // Make sure, that Gazebo communication is running before calling this
    Distributor( gazebo::transport::Node & node,
                 ModulesCommunication & modulesCommunication,
                 bool verbose );

    Distributor( const Distributor & ) = delete;
    Distributor & operator=( const Distributor & ) = delete;
    Distributor( Distributor && ) = delete;
    Distributor & operator=( Distributor && ) = delete;

    ~Distributor()
    {
        assert( _sub );
        _sub->Unsubscribe();
    }

private:
    void sendResponse( rofi::messages::DistributorResp resp )
    {
        assert( _pub );

        // Workaround for gazebo losing messages
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        _logger.logSending( _pub->GetTopic(), resp );
        _pub->Publish( std::move( resp ), true );
    }

    void onRequest( const rofi::messages::DistributorReq & req );
    void onRequestCallback( const boost::shared_ptr< const rofi::messages::DistributorReq > & req )
    {
        assert( req );
        auto reqCopy = req;
        assert( reqCopy );

        _logger.logReceived( _sub->GetTopic(), *reqCopy );

        onRequest( *reqCopy );
    }

    rofi::messages::DistributorResp onGetInfoReq();
    rofi::messages::DistributorResp onLockOneReq( SessionId sessionId );
    rofi::messages::DistributorResp onTryLockReq( ModuleId moduleId, SessionId sessionId );
    rofi::messages::DistributorResp onUnlockReq( ModuleId moduleId, SessionId sessionId );


    ModulesCommunication & _modulesCommunication;

    msgs::MessageLogger _logger;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};
} // namespace rofi::simplesim
