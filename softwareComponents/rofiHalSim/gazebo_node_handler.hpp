#pragma once

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>


namespace rofi::hal
{
class GazeboNodeHandler
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

public:
    GazeboNodeHandler()
            : _node( [] {
                GazeboClientHolder::run();
                auto node = boost::make_shared< gazebo::transport::Node >();
                node->Init();
                return node;
            }() )
    {
        assert( _node );
        assert( _node->IsInitialized() );
    }

    gazebo::transport::Node & operator*()
    {
        return *_node;
    }

    const gazebo::transport::Node & operator*() const
    {
        return *_node;
    }

    gazebo::transport::Node * operator->()
    {
        return _node.get();
    }

    const gazebo::transport::Node * operator->() const
    {
        return _node.get();
    }

private:
    const gazebo::transport::NodePtr _node;
};

} // namespace rofi::hal
