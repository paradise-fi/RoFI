#pragma once

#include <memory>

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>


namespace rofi::hal
{
class GazeboClientHolder
{
    GazeboClientHolder()
    {
        std::cerr << "Starting gazebo client" << std::endl;
        gazebo::client::setup();
    }

public:
    GazeboClientHolder( const GazeboClientHolder & ) = delete;
    GazeboClientHolder & operator=( const GazeboClientHolder & ) = delete;

    ~GazeboClientHolder()
    {
        gazebo::client::shutdown();
        std::cerr << "Ending gazebo client" << std::endl;
    }

    static std::shared_ptr< GazeboClientHolder > get()
    {
        static std::mutex instanceMutex;
        std::lock_guard< std::mutex > lock( instanceMutex );

        static std::weak_ptr< GazeboClientHolder > weakInstance;
        auto instance = weakInstance.lock();

        if ( !instance )
        {
            instance = std::shared_ptr< GazeboClientHolder >( new GazeboClientHolder() );
            weakInstance = instance;
        }

        assert( weakInstance.lock() == instance );
        assert( instance );
        return instance;
    }
};

} // namespace rofi::hal
