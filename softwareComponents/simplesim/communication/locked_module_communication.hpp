#pragma once

#include <string>

#include <gazebo/transport/transport.hh>

#include <rofiCmd.pb.h>


namespace rofi::simplesim
{
class ModulesCommunication;

class LockedModuleCommunication
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    LockedModuleCommunication( ModulesCommunication & modulesCommunication, RofiId rofiId );

    std::string topic( const gazebo::transport::Node & node ) const
    {
        return "/gazebo/" + node.GetTopicNamespace() + "/" + _topicName;
    }
    gazebo::transport::Publisher & pub()
    {
        assert( _pub );
        return *_pub;
    }
    gazebo::transport::Subscriber & sub()
    {
        assert( _sub );
        return *_sub;
    }

private:
    void onRofiCmd( const RofiCmdPtr & msg );

private:
    ModulesCommunication & _modulesCommunication;
    RofiId _rofiId = {};
    std::string _topicName;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};

} // namespace rofi::simplesim
