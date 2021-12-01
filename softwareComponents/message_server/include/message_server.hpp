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
    MessageServer( MessageServer && ) noexcept; // = default;
    ~MessageServer(); // = default;

    void poll();
    void loop();
    void loopInThread();

    static MessageServer createAndLoopInThread( std::string_view logName = "default" )
    {
        auto result = MessageServer( logName );
        result.loopInThread();
        return std::move( result );
    }

private:
    std::unique_ptr< details::MessageServerImpl > _impl;
};

} // namespace rofi::msgs
