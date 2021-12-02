#pragma once

#include <memory>
#include <string>

#include <gazebo/Master.hh>


namespace rofi::msgs
{
class [[nodiscard]] MessageServer
{
public:
    // MessageServer( std::string_view logName = "default" );

    // void poll();
    // void loop();
    // void loopInThread();

    static MessageServer createAndLoopInThread( std::string_view logName = "default" );

private:
    std::unique_ptr< gazebo::Master > _impl;
};

} // namespace rofi::msgs
