#pragma once

#include <string>

#include <gazebo/transport/transport.hh>

#include "command_handler.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class LockedModuleCommunication
{
public:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    LockedModuleCommunication( CommandHandler & commandHandler,
                               gazebo::transport::Node & node,
                               std::string moduleTopicName,
                               ModuleId moduleId );

    ~LockedModuleCommunication()
    {
        assert( _sub );
        _sub->Unsubscribe();
    }

    LockedModuleCommunication( const LockedModuleCommunication & ) = delete;
    LockedModuleCommunication( LockedModuleCommunication && ) = delete;
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
    CommandHandler & _commandHandler;

    ModuleId _moduleId = {};
    std::string _topic;

    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};

} // namespace rofi::simplesim
