#include <cassert>
#include <functional>

#include <gazebo/transport/transport.hh>


template < typename Message >
class SubscriberWrapper
{
public:
    using MessagePtr = boost::shared_ptr< const Message >;

    SubscriberWrapper( gazebo::transport::NodePtr node,
                       const std::string & topic,
                       std::function< void( MessagePtr ) > callback )
            : _callback( std::move( callback ) )
    {
        if ( !node )
        {
            throw std::runtime_error( "empty node" );
        }
        if ( !callback )
        {
            throw std::runtime_error( "empty callback" );
        }

        _sub = node->Subscribe( topic, &SubscriberWrapper::onMsg, this );
    }

    ~SubscriberWrapper()
    {
        if ( _sub )
        {
            _sub->Unsubscribe();
        }
    }

private:
    void onMsg( const MessagePtr & msg )
    {
        assert( _callback );
        assert( msg );
        _callback( msg );
    }

    std::function< void( MessagePtr ) > _callback;
    gazebo::transport::SubscriberPtr _sub;
};
