#pragma once

#include <cassert>
#include <functional>

#include <gazebo/transport/transport.hh>

#include "gazebo_client_holder.hpp"


namespace rofi::hal
{
template < typename Message >
class SubscriberWrapper
{
public:
    SubscriberWrapper( gazebo::transport::NodePtr node,
                       const std::string & topic,
                       std::function< void( const Message & ) > callback )
            : _clientHolder( GazeboClientHolder::get() )
            , _callback( std::move( callback ) )
            , _node( std::move( node ) )
    {
        if ( !_callback )
        {
            throw std::runtime_error( "empty callback" );
        }
        if ( !_node || !_node->IsInitialized() )
        {
            throw std::runtime_error( "node not initialized" );
        }

        _sub = _node->Subscribe( topic, &SubscriberWrapper::onMsg, this );
    }

    ~SubscriberWrapper()
    {
        if ( _sub )
        {
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
        _callback( *localMsg );
    }

    std::shared_ptr< GazeboClientHolder > _clientHolder;

    std::function< void( const Message & ) > _callback;
    gazebo::transport::NodePtr _node;
    gazebo::transport::SubscriberPtr _sub;
};

template < typename Message >
using SubscriberWrapperPtr = std::unique_ptr< SubscriberWrapper< Message > >;

} // namespace rofi::hal
