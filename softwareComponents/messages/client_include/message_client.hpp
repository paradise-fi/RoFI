#pragma once

#include <memory>
#include <string_view>


namespace rofi::msgs
{
namespace details
{
    class PublisherImpl;
    class SubscriberImpl;
    class MessageClientImpl;

} // namespace details


class Publisher
{
public:
    ~Publisher(); // = default;

private:
    friend class MessageClient;
    Publisher( std::unique_ptr< details::PublisherImpl > impl ) : _impl( std::move( impl ) ) {}

    std::unique_ptr< details::PublisherImpl > _impl;
};

class [[nodiscard]] Subscriber
{
public:
    ~Subscriber(); // = default;

private:
    friend class MessageClient;
    Subscriber( std::unique_ptr< details::SubscriberImpl > impl ) : _impl( std::move( impl ) ) {}

    std::unique_ptr< details::SubscriberImpl > _impl;
};

// Has to outlive its subscriber and publisher
class [[nodiscard]] MessageClient
{
public:
    MessageClient( std::string_view topicName = {} );

    Subscriber sub();
    Publisher pub();

private:
    std::shared_ptr< details::MessageClientImpl > _impl;
};

} // namespace rofi::msgs
