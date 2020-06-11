#pragma once

#include <atomic>
#include <cassert>
#include <thread>
#include <type_traits>
#include <variant>

#include <gazebo/gazebo_client.hh>
#include <gazebo/transport/transport.hh>

#include "concurrentQueue.hpp"
#include "gazeboClientHolder.hpp"
#include "rofi_hal.hpp"
#include "subscriberWrapper.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::hal
{
class GazeboWorker
{
    using DistributorRespPtr = boost::shared_ptr< const rofi::messages::DistributorResp >;
    using RofiRespPtr = boost::shared_ptr< const rofi::messages::RofiResp >;
    using MessageVariant = std::variant< rofi::messages::RofiCmd, rofi::messages::DistributorReq >;

    GazeboWorker()
            : _clientHolder( GazeboClientHolder::get() )
            , _node( initializedNode() )
            , _rofiTopicsSub( _node, getDistRespTopic(), [ this ]( auto resp ) {
                updateRofiTopics( resp );
            } )
    {
        assert( _node );
        assert( _node->IsInitialized() );
    }

public:
    GazeboWorker( const GazeboWorker & ) = delete;
    GazeboWorker & operator=( const GazeboWorker & ) = delete;

    template < typename Message >
    void publish( Message && msg )
    {
        publish( getTopic( msg ), std::forward< Message >( msg ) );
    }

    template < typename Message >
    void publish( std::string topic, Message && msg )
    {
        _queue.emplace( std::move( topic ), MessageVariant( std::forward< Message >( msg ) ) );
    }

    SubscriberWrapperPtr< rofi::messages::DistributorResp > subscribe(
            std::function< void( DistributorRespPtr ) > callback )
    {
        using SubWrapper = SubscriberWrapper< rofi::messages::DistributorResp >;
        return std::make_unique< SubWrapper >( _node, getDistRespTopic(), std::move( callback ) );
    }

    SubscriberWrapperPtr< rofi::messages::RofiResp > subscribe(
            rofi::hal::RoFI::Id rofiId,
            std::function< void( RofiRespPtr ) > callback )
    {
        using SubWrapper = SubscriberWrapper< rofi::messages::RofiResp >;
        return std::make_unique< SubWrapper >( _node,
                                               getRofiRespTopic( rofiId ),
                                               std::move( callback ) );
    }

    void updateRofiTopics( const DistributorRespPtr & resp )
    {
        assert( resp );

        {
            std::lock_guard< std::mutex > lock( _rofiTopicsMutex );
            for ( auto & info : resp->rofiinfos() )
            {
                _rofiTopics.emplace( info.rofiid(), info.topic() );
            }
        }

        _onRofiTopicsUpdate.notify_all();
    }

    static GazeboWorker & get()
    {
        if ( !instance()._initialized )
        {
            instance().runWorker();
            while ( !instance()._initialized )
            {
                std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            }
        }
        assert( instance()._initialized );
        return instance();
    }

private:
    static gazebo::transport::NodePtr initializedNode()
    {
        auto node = boost::make_shared< gazebo::transport::Node >();
        assert( node );
        node->Init();
        assert( node->IsInitialized() );
        return node;
    }

    static GazeboWorker & instance()
    {
        static GazeboWorker instance;
        return instance;
    }

    void runWorker()
    {
        std::call_once( instance()._runOnce, [] {
            std::thread( &GazeboWorker::run, &GazeboWorker::instance() ).detach();
        } );
    }

    std::string getRofiTopic( rofi::hal::RoFI::Id rofiId )
    {
        std::unique_lock< std::mutex > lock( _rofiTopicsMutex );
        auto it = _rofiTopics.find( rofiId );
        if ( it != _rofiTopics.end() )
        {
            return it->second;
        }

        rofi::messages::DistributorReq req;
        req.set_reqtype( rofi::messages::DistributorReq::GET_INFO );
        publish( std::move( req ) );

        _onRofiTopicsUpdate.wait( lock, [ this, rofiId ] {
            return _rofiTopics.find( rofiId ) != _rofiTopics.end();
        } );
        return _rofiTopics.at( rofiId );
    }

    std::string getTopic( const rofi::messages::RofiCmd & msg )
    {
        return getRofiTopic( msg.rofiid() ) + "/control";
    }

    std::string getTopic( const rofi::messages::DistributorReq & )
    {
        return "~/distributor/request";
    }

    std::string getRofiRespTopic( RoFI::Id rofiId )
    {
        return getRofiTopic( rofiId ) + "/response";
    }

    static std::string getDistRespTopic()
    {
        return "~/distributor/response";
    }

    template < typename Message >
    void sendMessage( const std::string & topic, Message && msg )
    {
        assert( _node );

        auto & pub = _pubs[ topic ];
        if ( !pub )
        {
            pub = _node->Advertise< std::decay_t< Message > >( topic );
            assert( pub );
            pub->WaitForConnection();
        }
        assert( pub );

        pub->Publish( std::move( msg ), true );
    }

    void run()
    {
        _node = boost::make_shared< gazebo::transport::Node >();
        assert( _node );
        _node->Init();
        assert( _node->IsInitialized() );

        _initialized = true;

        while ( true )
        {
            auto [ topic, msgVariant ] = _queue.pop();
            std::visit( [ this, &topic ]( auto msg ) { sendMessage( topic, msg ); }, msgVariant );
        }
    }


    const std::shared_ptr< GazeboClientHolder > _clientHolder;

    std::once_flag _runOnce;
    std::atomic< bool > _initialized = false;

    gazebo::transport::NodePtr _node;
    std::map< std::string, gazebo::transport::PublisherPtr > _pubs;

    std::mutex _rofiTopicsMutex;
    std::map< RoFI::Id, std::string > _rofiTopics;
    std::condition_variable _onRofiTopicsUpdate;
    SubscriberWrapper< rofi::messages::DistributorResp > _rofiTopicsSub;

    ConcurrentQueue< std::pair< std::string, MessageVariant > > _queue;
};
} // namespace rofi::hal
