#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <gz/transport/Node.hh>

namespace rofi::gz
{
class Publisher {
public:
    Publisher() = default;

    Publisher( ::gz::transport::Node::Publisher publisher, std::string topic )
            : _publisher( std::move( publisher ) ), _topic( std::move( topic ) )
    {}

    explicit operator bool() const
    {
        return _publisher.Valid();
    }

    bool HasConnections() const
    {
        return _publisher.HasConnections();
    }

    template < typename Message >
    bool Publish( const Message & message, bool /* block */ = false )
    {
        return _publisher.Publish( message );
    }

    bool WaitForConnection() const
    {
        while ( !_publisher.HasConnections() ) {
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        return true;
    }

    template < typename Rep, typename Period >
    bool WaitForConnection( const std::chrono::duration< Rep, Period > & timeout ) const
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while ( !_publisher.HasConnections() ) {
            if ( std::chrono::steady_clock::now() >= deadline ) {
                return _publisher.HasConnections();
            }
            std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
        }
        return true;
    }

    const std::string & GetTopic() const
    {
        return _topic;
    }

    void Fini()
    {}

private:
    ::gz::transport::Node::Publisher _publisher;
    std::string _topic;
};

class Subscriber {
public:
    Subscriber() = default;

    Subscriber( ::gz::transport::Node::Subscriber subscriber, std::string topic )
            : _subscriber( std::move( subscriber ) ), _topic( std::move( topic ) )
    {}

    explicit operator bool() const
    {
        return _subscriber.Valid();
    }

    bool Unsubscribe()
    {
        return _subscriber.Unsubscribe();
    }

    const std::string & GetTopic() const
    {
        return _topic;
    }

private:
    ::gz::transport::Node::Subscriber _subscriber;
    std::string _topic;
};

using PublisherPtr = boost::shared_ptr< Publisher >;
using SubscriberPtr = boost::shared_ptr< Subscriber >;

namespace detail
{
template < typename... >
inline constexpr bool alwaysFalse = false;

inline std::string trimSlashes( std::string value )
{
    while ( !value.empty() && value.front() == '/' ) {
        value.erase( value.begin() );
    }
    while ( !value.empty() && value.back() == '/' ) {
        value.pop_back();
    }
    return value;
}

template < typename Message, typename Callback >
auto wrapCallback( Callback && callback )
{
    return [ callback = std::forward< Callback >( callback ) ]( const Message & message ) mutable {
        if constexpr ( std::is_invocable_v< Callback, const Message & > ) {
            callback( message );
        }
        else if constexpr ( std::is_invocable_v< Callback,
                                                 const boost::shared_ptr< const Message > & > ) {
            callback( boost::make_shared< Message >( message ) );
        }
        else {
            static_assert( alwaysFalse< Callback >,
                           "Unsupported transport callback signature" );
        }
    };
}

template < typename Message, typename T >
auto wrapCallback( void ( T::*callback )( const Message & ), T * object )
{
    return [ object, callback ]( const Message & message ) { ( object->*callback )( message ); };
}

template < typename Message, typename T >
auto wrapCallback( void ( T::*callback )( const boost::shared_ptr< const Message > & ),
                   T * object )
{
    return [ object, callback ]( const Message & message ) {
        ( object->*callback )( boost::make_shared< Message >( message ) );
    };
}
} // namespace detail

class Node {
public:
    void Init( std::string topicNamespace = {} )
    {
        _topicNamespace = detail::trimSlashes( std::move( topicNamespace ) );
        if ( _topicNamespace.empty() ) {
            _topicNamespace = "default";
        }
        _initialized = true;
    }

    bool IsInitialized() const
    {
        return _initialized;
    }

    void Fini()
    {}

    const std::string & GetTopicNamespace() const
    {
        return _topicNamespace;
    }

    template < typename Message >
    PublisherPtr Advertise( const std::string & topic )
    {
        const auto resolvedTopic = resolveTopic( topic );
        auto publisher = _node.Advertise< Message >( resolvedTopic );
        if ( !publisher ) {
            return {};
        }
        return boost::make_shared< Publisher >( std::move( publisher ), resolvedTopic );
    }

    template < typename Message, typename Callback >
    SubscriberPtr Subscribe( const std::string & topic,
                             Callback && callback,
                             bool /* latching */ = false )
    {
        const auto resolvedTopic = resolveTopic( topic );
        auto handler = std::function< void( const Message & ) >(
                detail::wrapCallback< Message >( std::forward< Callback >( callback ) ) );
        auto subscriber = _node.CreateSubscriber( resolvedTopic, std::move( handler ) );
        if ( !subscriber ) {
            return {};
        }
        return boost::make_shared< Subscriber >( std::move( subscriber ), resolvedTopic );
    }

    template < typename Message, typename T >
    SubscriberPtr Subscribe( const std::string & topic,
                             void ( T::*callback )( const Message & ),
                             T * object,
                             bool /* latching */ = false )
    {
        const auto resolvedTopic = resolveTopic( topic );
        auto handler = std::function< void( const Message & ) >(
                detail::wrapCallback( callback, object ) );
        auto subscriber = _node.CreateSubscriber( resolvedTopic, std::move( handler ) );
        if ( !subscriber ) {
            return {};
        }
        return boost::make_shared< Subscriber >( std::move( subscriber ), resolvedTopic );
    }

    template < typename Message, typename T >
    SubscriberPtr Subscribe(
            const std::string & topic,
            void ( T::*callback )( const boost::shared_ptr< const Message > & ),
            T * object,
            bool /* latching */ = false )
    {
        const auto resolvedTopic = resolveTopic( topic );
        auto handler = std::function< void( const Message & ) >(
                detail::wrapCallback( callback, object ) );
        auto subscriber = _node.CreateSubscriber( resolvedTopic, std::move( handler ) );
        if ( !subscriber ) {
            return {};
        }
        return boost::make_shared< Subscriber >( std::move( subscriber ), resolvedTopic );
    }

private:
    std::string resolveTopic( const std::string & topic ) const
    {
        if ( topic.empty() ) {
            return {};
        }

        if ( topic.starts_with( "/gazebo/" ) || topic.starts_with( "/world/" )
             || topic.front() == '/' ) {
            return topic;
        }

        const auto base = "/gazebo/" + _topicNamespace;
        if ( topic == "~" ) {
            return base;
        }
        if ( topic.starts_with( "~/" ) ) {
            return base + "/" + topic.substr( 2 );
        }
        if ( topic.starts_with( "~" ) ) {
            return base + "/" + topic.substr( 1 );
        }
        return base + "/" + topic;
    }

    ::gz::transport::Node _node;
    std::string _topicNamespace = "default";
    bool _initialized = false;
};

using NodePtr = boost::shared_ptr< Node >;
} // namespace rofi::gz
