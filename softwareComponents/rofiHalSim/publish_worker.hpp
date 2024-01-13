#pragma once

#include <atomic>
#include <cassert>
#include <mutex>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <variant>

#include <atoms/concurrent_queue.hpp>
#include <gazebo/transport/transport.hh>

#include <rofi_hal.hpp>

#include "gazebo_node_handler.hpp"
#include "message_logger.hpp"
#include "session_id.hpp"
#include "subscriber_wrapper.hpp"

#include <distributorReq.pb.h>
#include <distributorResp.pb.h>
#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>


namespace rofi::hal
{
class PublishWorker {
    using MessageVariant = std::variant< rofi::messages::RofiCmd, rofi::messages::DistributorReq >;

    PublishWorker()
    {
        _rofiTopicsSub = subscribe( [ this ]( auto resp ) { updateRofiTopics( resp ); } );

        _workerThread = std::jthread(
                [ this ]( std::stop_token stoken ) { this->run( std::move( stoken ) ); } );
    }

public:
    PublishWorker( const PublishWorker & ) = delete;
    PublishWorker & operator=( const PublishWorker & ) = delete;

    ~PublishWorker()
    {
        // End thread before destructor ends
        if ( _workerThread.joinable() ) {
            _workerThread.request_stop();
            _workerThread.join();
        }
    }

    template < typename Message >
    void publish( Message && msg )
    {
        auto topic = getTopic( msg );
        publish( std::move( topic ), std::forward< Message >( msg ) );
    }

    template < typename Message >
    void publish( std::string topic, Message && msg )
    {
        _queue.emplace( std::move( topic ), MessageVariant( std::forward< Message >( msg ) ) );
    }

    SubscriberWrapperPtr< rofi::messages::DistributorResp > subscribe(
            std::function< void( const rofi::messages::DistributorResp & ) > callback )
    {
        assert( callback );
        using SubWrapper = SubscriberWrapper< rofi::messages::DistributorResp >;
        return std::make_unique< SubWrapper >( _node, getDistRespTopic(), std::move( callback ) );
    }

    SubscriberWrapperPtr< rofi::messages::RofiResp > subscribe(
            rofi::hal::RoFI::Id rofiId,
            std::function< void( const rofi::messages::RofiResp & ) > callback )
    {
        assert( callback );
        using SubWrapper = SubscriberWrapper< rofi::messages::RofiResp >;
        return std::make_unique< SubWrapper >( _node,
                                               getRofiRespTopic( rofiId ),
                                               std::move( callback ) );
    }

    static PublishWorker & get()
    {
        static PublishWorker instance;
        return instance;
    }

private:
    void updateRofiTopics( const rofi::messages::DistributorResp & resp )
    {
        {
            std::lock_guard< std::mutex > lock( _rofiTopicsMutex );
            for ( auto & info : resp.rofiinfos() ) {
                _rofiTopics.emplace( info.rofiid(), info.topic() );
            }
        }

        _onRofiTopicsUpdate.notify_all();
    }

    std::string getRofiTopic( rofi::hal::RoFI::Id rofiId )
    {
        std::unique_lock< std::mutex > lock( _rofiTopicsMutex );
        auto it = _rofiTopics.find( rofiId );
        if ( it != _rofiTopics.end() ) {
            return it->second;
        }

        // Subscribe to ~/distributor/response before sending to ~/distributor/request
        auto sub = PublishWorker::get().subscribe(
                [ & ]( auto resp ) { std::cout << "Subscribed to response\n"; } );

        // Lock the module if not locked already
        rofi::messages::DistributorReq req;
        req.set_reqtype( rofi::messages::DistributorReq::TRY_LOCK );
        req.set_sessionid( SessionId::get().bytes() );
        req.set_rofiid( rofiId );
        publish( std::move( req ) );

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
        auto & pub = _pubs[ topic ];
        if ( !pub ) {
            pub = _node->Advertise< std::decay_t< Message > >( topic );
            assert( pub );
            pub->WaitForConnection();
        }
        assert( pub );

        // Workaround for gazebo losing messages
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

        logMessage( topic, msg, true );

        pub->Publish( std::forward< Message >( msg ), true );
    }

    void run( std::stop_token stoken )
    {
        while ( true ) {
            auto newMessage = _queue.pop( stoken );
            if ( !newMessage ) {
                return;
            }
            auto & [ topic, msgVariant ] = *newMessage;
            std::visit( [ this, &topic ]( auto msg ) { sendMessage( topic, std::move( msg ) ); },
                        std::move( msgVariant ) );
        }
    }

    atoms::ConcurrentQueue< std::pair< std::string, MessageVariant > > _queue;

    GazeboNodeHandler _node;

    std::mutex _rofiTopicsMutex;
    std::map< RoFI::Id, std::string > _rofiTopics;
    std::condition_variable _onRofiTopicsUpdate;
    SubscriberWrapperPtr< rofi::messages::DistributorResp > _rofiTopicsSub;

    std::map< std::string, gazebo::transport::PublisherPtr > _pubs;

    std::jthread _workerThread;
};
} // namespace rofi::hal
