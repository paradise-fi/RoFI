#pragma once

#include <cassert>
#include <functional>
#include <memory>

#include <gazebo/transport/transport.hh>

#include "gazebo_node_handler.hpp"
#include "message_logger.hpp"


namespace rofi::hal
{
template < typename Message >
class SubscriberWrapper {
public:
    SubscriberWrapper( const GazeboNodeHandler & node,
                       std::string topic,
                       std::function< void( const Message & ) > callback )
            : _callback( std::move( callback ) ), _node( node ), _topic( std::move( topic ) )
    {
        if ( !_callback ) {
            throw std::runtime_error( "empty callback" );
        }

        logSubscription( _topic, true );

        _sub = _node->Subscribe( _topic, &SubscriberWrapper::onMsg, this );
    }

    SubscriberWrapper( const SubscriberWrapper & ) = delete;
    SubscriberWrapper & operator=( const SubscriberWrapper & ) = delete;

    ~SubscriberWrapper()
    {
        if ( _sub ) {
            logSubscription( _sub->GetTopic(), false );
            _sub->Unsubscribe();
        }
    }

private:
    void onMsg( const boost::shared_ptr< const Message > & msg )
    {
        assert( _callback );
        assert( msg );

        auto localMsg = msg;
        assert( localMsg );

        logMessage( _topic, *localMsg, false );

        _callback( *localMsg );
    }

    const std::function< void( const Message & ) > _callback;
    GazeboNodeHandler _node;
    std::string _topic;
    gazebo::transport::SubscriberPtr _sub;
};

template < typename Message >
using SubscriberWrapperPtr = std::unique_ptr< SubscriberWrapper< Message > >;

} // namespace rofi::hal
