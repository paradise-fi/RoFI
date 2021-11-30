#include "message_client.hpp"

#include <gazebo/gazebo.hh>
#include <gazebo/gazebo_client.hh>


namespace rofi::msgs
{
namespace details
{
    class GazeboClientHolder
    {
        GazeboClientHolder()
        {
            gazebo::client::setup();
        }

    public:
        GazeboClientHolder( const GazeboClientHolder & ) = delete;
        GazeboClientHolder & operator=( const GazeboClientHolder & ) = delete;

        ~GazeboClientHolder()
        {
            gazebo::client::shutdown();
        }

        // Runs Gazebo client
        static void run()
        {
            static GazeboClientHolder instance;
        }
    };


    class PublisherImpl : public gazebo::transport::Publisher
    {};
    class SubscriberImpl : public gazebo::transport::Subscriber
    {};

    class MessageClientImpl : public gazebo::transport::Node
    {
    public:
        MessageClientImpl( std::string_view topicName )
        {
            using Node = gazebo::transport::Node;

            Node::Init( std::string( topicName ) );
            assert( Node::IsInitialized() );
        }
    };

} // namespace details


Publisher::~Publisher() = default;
Subscriber::~Subscriber() = default;

MessageClient::MessageClient( std::string_view topicName )
        : _impl( [ &topicName ] {
            details::GazeboClientHolder::run();
            return std::make_shared< details::MessageClientImpl >( topicName );
        }() )
{
    assert( _impl );
}

} // namespace rofi::msgs
