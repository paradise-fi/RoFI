#pragma once

#include <cassert>
#include <functional>
#include <thread>
#include <type_traits>

#include "callbacks.hpp"
#include "atoms/concurrent_queue.hpp"
#include "atoms/jthread.hpp"
#include "rofi_hal.hpp"

#include <connectorResp.pb.h>


namespace rofi::hal
{
class ConnectorWorker
{
    using Message = rofi::messages::ConnectorResp;
    using PacketCallback = std::function< void( Connector, uint16_t contentType, PBuf ) >;
    using EventCallback = std::function< void( Connector, ConnectorEvent ) >;

    struct ConnectorCallbacks
    {
        Callbacks< EventCallback > eventCallbacks;
        Callbacks< PacketCallback > packetCallbacks;
    };

public:
    ConnectorWorker() = default;

    ConnectorWorker( const ConnectorWorker & ) = delete;
    ConnectorWorker & operator=( const ConnectorWorker & ) = delete;

    ~ConnectorWorker()
    {
        // End thread before destructor ends
        if ( _workerThread.joinable() )
        {
            _workerThread.request_stop();
            _workerThread.join();
        }
    }

    void init( std::weak_ptr< RoFI::Implementation > rofi, int connectorCount )
    {
        assert( connectorCount >= 0 );
        assert( rofi.lock() );

        _rofi = std::move( rofi );
        _callbacks.resize( connectorCount );

        _workerThread = atoms::jthread(
                [ this ]( atoms::stop_token stoken ) { this->run( std::move( stoken ) ); } );
    }


    void processMessage( const Message & msg )
    {
        _queue.push( msg );
    }

    void registerEventCallback( int connectorIndex, EventCallback && callback )
    {
        assert( connectorIndex >= 0 );
        assert( static_cast< size_t >( connectorIndex ) < _callbacks.size() );
        assert( callback );

        _callbacks[ connectorIndex ].eventCallbacks.registerCallback( std::move( callback ) );
    }

    void registerPacketCallback( int connectorIndex, PacketCallback && callback )
    {
        assert( connectorIndex >= 0 );
        assert( static_cast< size_t >( connectorIndex ) < _callbacks.size() );
        assert( callback );

        _callbacks[ connectorIndex ].packetCallbacks.registerCallback( std::move( callback ) );
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

        switch ( eventRespType )
        {
            case ConnectorCmd::CONNECT:
                return ConnectorEvent::Connected;
            case ConnectorCmd::DISCONNECT:
                return ConnectorEvent::Disconnected;
            case ConnectorCmd::CONNECT_POWER:
                return ConnectorEvent::ConnectedPower;
            case ConnectorCmd::DISCONNECT_POWER:
                return ConnectorEvent::DisconnectedPower;
            default:
                assert( false );
                throw std::invalid_argument( "not an event response" );
        }
    }

    static std::pair< uint16_t, PBuf > getPacket( const rofi::messages::Packet & packet )
    {
        PBuf pbufPacket = PBuf::allocate( packet.message().size() );
        size_t pos = 0;
        for ( auto it = pbufPacket.chunksBegin(); it != pbufPacket.chunksEnd(); ++it )
        {
            std::copy_n( packet.message().begin() + pos, it->size(), it->mem() );
            pos += it->size();
        }

        assert( pos == packet.message().size() );
        return { packet.contenttype(), pbufPacket };
    }

    void callCallbacks( const Message & message )
    {
        auto connectorIndex = message.connector();
        assert( connectorIndex >= 0 );
        assert( static_cast< size_t >( connectorIndex ) < _callbacks.size() );

        auto connector = getConnector( connectorIndex );
        switch ( message.resptype() )
        {
            case rofi::messages::ConnectorCmd::PACKET:
            {
                auto [ contentType, packet ] = getPacket( message.packet() );
                _callbacks[ connectorIndex ].packetCallbacks.callCallbacks( std::move( connector ),
                                                                            contentType,
                                                                            std::move( packet ) );
                break;
            }
            case rofi::messages::ConnectorCmd::CONNECT:
            case rofi::messages::ConnectorCmd::DISCONNECT:
            case rofi::messages::ConnectorCmd::CONNECT_POWER:
            case rofi::messages::ConnectorCmd::DISCONNECT_POWER:
            {
                auto event = readEvent( message.resptype() );
                _callbacks[ connectorIndex ].eventCallbacks.callCallbacks( std::move( connector ),
                                                                           event );
                break;
            }
            default:
                assert( false );
        }
    }

    void run( atoms::stop_token stoken )
    {
        while ( true )
        {
            auto message = _queue.pop( stoken );
            if ( !message )
            {
                return;
            }
            callCallbacks( *message );
        }
    }


    std::weak_ptr< RoFI::Implementation > _rofi;

    std::vector< ConnectorCallbacks > _callbacks;
    atoms::ConcurrentQueue< Message > _queue;

    atoms::jthread _workerThread;
};

} // namespace rofi::hal
