#pragma once

#include <string>
#include <thread>

#include <gazebo/transport/transport.hh>

#include <message_logger.hpp>

#include "command_handler.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
class LockedModuleCommunication {
public:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;

    LockedModuleCommunication( CommandHandler & commandHandler,
                               gazebo::transport::Node & node,
                               std::string moduleTopicName,
                               ModuleId moduleId,
                               bool verbose );

    LockedModuleCommunication( const LockedModuleCommunication & ) = delete;
    LockedModuleCommunication( LockedModuleCommunication && ) = delete;
    LockedModuleCommunication & operator=( const LockedModuleCommunication & ) = delete;
    LockedModuleCommunication & operator=( LockedModuleCommunication && ) = delete;

    ~LockedModuleCommunication()
    {
        assert( _sub );
        _logger.logUnsubscribe( _sub->GetTopic() );
        _sub->Unsubscribe();
    }


    const std::string & topic() const
    {
        return _topic;
    }

    void sendResponse( rofi::messages::RofiResp resp )
    {
        assert( _pub );

        // Workaround for gazebo losing messages
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        _logger.logSending( _pub->GetTopic(), resp );
        _pub->Publish( std::move( resp ), true );
    }

private:
    void onRofiCmd( const RofiCmdPtr & msg );

private:
    CommandHandler & _commandHandler;

    ModuleId _moduleId = {};
    std::string _topic;

    msgs::MessageLogger _logger;
    gazebo::transport::PublisherPtr _pub;
    gazebo::transport::SubscriberPtr _sub;
};

} // namespace rofi::simplesim
