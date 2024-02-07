#pragma once

#include <cassert>
#include <functional>
#include <stop_token>
#include <thread>
#include <type_traits>

#include <atoms/concurrent_queue.hpp>
#include <atoms/guarded.hpp>
#include <atoms/unreachable.hpp>

#include <rofi_hal.hpp>

#include <connectorResp.pb.h>


namespace rofi::hal
{
class ConnectorWorker {
    using Message = rofi::messages::ConnectorResp;
    using PacketCallback = std::function< void( Connector, uint16_t contentType, PBuf ) >;
    using EventCallback = std::function< void( Connector, ConnectorEvent ) >;

    struct ConnectorCallbacks {
        atoms::Guarded< EventCallback > eventCallback;
        atoms::Guarded< PacketCallback > packetCallback;
    };

public:
    ConnectorWorker() = default;

    ConnectorWorker( const ConnectorWorker & ) = delete;
    ConnectorWorker & operator=( const ConnectorWorker & ) = delete;

    ~ConnectorWorker()
    {
        // End thread before destructor ends
        if ( _workerThread.joinable() ) {
            _workerThread.request_stop();
            _workerThread.join();
        }
    }

    void init( std::weak_ptr< RoFI::Implementation > rofi, int connectorCount )
    {
        assert( connectorCount >= 0 );
        assert( !rofi.expired() );

        assert( _rofi.expired() );
        assert( _callbacks.empty() );
        assert( !_workerThread.joinable() );

        _rofi = std::move( rofi );
        // resize doesn't work because `std::mutex` isn't noexcept move constructible
        _callbacks = std::vector< ConnectorCallbacks >( connectorCount );

        _workerThread = std::jthread(
                [ this ]( std::stop_token stoken ) { this->run( std::move( stoken ) ); } );
    }


    void processMessage( const Message & msg )
    {
        _queue.push( msg );
    }

    void registerEventCallback( int connectorIndex, EventCallback && callback )
    {
        assert( connectorIndex >= 0 );
        assert( static_cast< size_t >( connectorIndex ) < _callbacks.size() );

        _callbacks[ connectorIndex ].eventCallback.replace( std::move( callback ) );
    }

    void registerPacketCallback( int connectorIndex, PacketCallback && callback )
    {
        assert( connectorIndex >= 0 );
        assert( static_cast< size_t >( connectorIndex ) < _callbacks.size() );

        _callbacks[ connectorIndex ].packetCallback.replace( std::move( callback ) );
    }

private:
    std::shared_ptr< RoFI::Implementation > getRoFI() const
    {
        auto rofi = _rofi.lock();
        assert( rofi && "RoFI invalid access from connector worker" );
        return rofi;
    }

    Connector getConnector( int index ) const
    {
        auto rofi = getRoFI();

        assert( static_cast< size_t >( rofi->getDescriptor().connectorCount )
                == _callbacks.size() );
        assert( index >= 0 );
        assert( index < rofi->getDescriptor().connectorCount );

        return rofi->getConnector( index );
    }

    static ConnectorEvent readEvent( const rofi::messages::ConnectorCmd::Type & eventRespType )
    {
        using rofi::messages::ConnectorCmd;

        switch ( eventRespType ) {
            case ConnectorCmd::CONNECT:
                return ConnectorEvent::Connected;
            case ConnectorCmd::DISCONNECT:
                return ConnectorEvent::Disconnected;
            case ConnectorCmd::POWER_CHANGED:
                return ConnectorEvent::PowerChanged;
            default:
                break;
        }
        ROFI_UNREACHABLE( "Unknown connector event type" );
    }

    static std::pair< uint16_t, PBuf > getPacket( const rofi::messages::Packet & packet )
    {
        assert( packet.message().size() < INT_MAX );
        PBuf pbufPacket = PBuf::allocate( static_cast< int >( packet.message().size() ) );
        size_t pos = 0;
        for ( auto it = pbufPacket.chunksBegin(); it != pbufPacket.chunksEnd(); ++it ) {
            std::copy_n( packet.message().begin() + pos, it->size(), it->mem() );
            pos += it->size();
        }

        assert( pos == packet.message().size() );
        return { packet.contenttype(), pbufPacket };
    }

    void callCallback( const Message & message )
    {
        auto connectorIndex = message.connector();
        assert( connectorIndex >= 0 );
        assert( static_cast< size_t >( connectorIndex ) < _callbacks.size() );

        auto connector = getConnector( connectorIndex );
        switch ( message.resptype() ) {
            case rofi::messages::ConnectorCmd::PACKET:
            {
                auto [ contentType, packet ] = getPacket( message.packet() );
                _callbacks[ connectorIndex ].packetCallback.visit( [ & ]( auto & callback ) {
                    if ( callback ) {
                        std::invoke( callback,
                                     std::move( connector ),
                                     contentType,
                                     std::move( packet ) );
                    }
                } );
                return;
            }
            case rofi::messages::ConnectorCmd::CONNECT:
            case rofi::messages::ConnectorCmd::DISCONNECT:
            case rofi::messages::ConnectorCmd::POWER_CHANGED:
            {
                auto event = readEvent( message.resptype() );
                _callbacks[ connectorIndex ].eventCallback.visit( [ & ]( auto & callback ) {
                    if ( callback ) {
                        std::invoke( callback, std::move( connector ), event );
                    }
                } );
                return;
            }
            default:
                break;
        }
        ROFI_UNREACHABLE( "Unknown connector response type" );
    }

    void run( std::stop_token stoken )
    {
        while ( true ) {
            auto message = _queue.pop( stoken );
            if ( !message ) {
                return;
            }
            callCallback( *message );
        }
    }


    std::weak_ptr< RoFI::Implementation > _rofi;

    std::vector< ConnectorCallbacks > _callbacks;
    atoms::ConcurrentQueue< Message > _queue;

    std::jthread _workerThread;
};

} // namespace rofi::hal
