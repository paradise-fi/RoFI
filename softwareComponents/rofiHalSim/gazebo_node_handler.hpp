#pragma once

#include <rofi/gz_transport.hpp>


namespace rofi::hal
{
class GazeboNodeHandler {
    class GazeboClientHolder {
        GazeboClientHolder()
        {}

    public:
        GazeboClientHolder( const GazeboClientHolder & ) = delete;
        GazeboClientHolder & operator=( const GazeboClientHolder & ) = delete;

        ~GazeboClientHolder() = default;

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
                auto node = std::make_shared< rofi::gz::Node >();
                node->Init();
                return node;
            }() )
    {
        assert( _node );
        assert( _node->IsInitialized() );
    }

    rofi::gz::Node & operator*()
    {
        return *_node;
    }

    const rofi::gz::Node & operator*() const
    {
        return *_node;
    }

    rofi::gz::Node * operator->()
    {
        return _node.get();
    }

    const rofi::gz::Node * operator->() const
    {
        return _node.get();
    }

private:
    const rofi::gz::NodePtr _node;
};

} // namespace rofi::hal
