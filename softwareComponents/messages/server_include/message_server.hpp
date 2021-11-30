#pragma once

#include <memory>
#include <string_view>


namespace rofi::msgs
{
namespace details
{
    class MessageServerImpl;
}


class [[nodiscard]] MessageServer
{
public:
    MessageServer( std::string_view logName = "default" );
    ~MessageServer(); // = default;

    void poll();
    void loop();
    void loopInThread();

private:
    std::unique_ptr< details::MessageServerImpl > _impl;
};

} // namespace rofi::msgs
