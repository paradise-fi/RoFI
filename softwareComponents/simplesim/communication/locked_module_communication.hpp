#pragma once

#include <string>

#include <gazebo/transport/transport.hh>

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class ModulesCommunication;

class LockedModuleCommunication
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    LockedModuleCommunication( ModulesCommunication & modulesCommunication,
                               gazebo::transport::Node & node,
                               std::string moduleTopicName,
                               RofiId rofiId );

    ~LockedModuleCommunication()
    {
        _sub->Unsubscribe();
    }

    LockedModuleCommunication( const LockedModuleCommunication & ) = delete;
    LockedModuleCommunication( LockedModuleCommunication && ) = default;
    LockedModuleCommunication & operator=( const LockedModuleCommunication & ) = delete;
    LockedModuleCommunication & operator=( LockedModuleCommunication && ) = delete;


    const std::string & topic( const gazebo::transport::Node & node ) const
    {
        return _topic;
    }

    void sendResponse( rofi::messages::RofiResp resp )
    {
        assert( _pub );
        _pub->Publish( std::move( resp ) );
    }

private:
    void onRofiCmd( const RofiCmdPtr & msg );

private:
    ModulesCommunication & _modulesCommunication;

    RofiId _rofiId = {};
    std::string _topic;

    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};

} // namespace rofi::simplesim
