#pragma once

#include "rofi_hal.hpp"
#include "routing_table.hpp"

#include <lwip/mld6.h>
#include <lwip/raw.h>
#include <lwip/ip6_addr.h>
#include <lwip/tcpip.h>

#include <algorithm>
#include <string>
#include <cassert>
#include <sstream>
#include <iostream>

namespace rofinet {
    using namespace rofi::hal;

static uint8_t seqNum() {
    static uint8_t num = 0;
    return num++;
}

class PhysNetif {
public:
    PhysNetif( PhysAddr pAddr, Connector connector, RTable& rt, const std::function< void( RTable::Command ) >& f )
    : pAddr( pAddr ), connector( connector ), rtable( rt ), _sendToOthers( f ) {
        connector.onPacket( [ this ]( Connector c, uint16_t contentType, PBuf&& pb ) {
            onPacket( c, contentType, std::move( pb ) );
        } );

        connector.onConnectorEvent( [ this ]( Connector, ConnectorEvent e) {
            if ( e == ConnectorEvent::Connected ) {
                netif.setActive( true );
                sendRRP( RTable::Command::Hello );
            } else { // DISCONNECT
                netif.setActive( false );
                netif.setStub( false );
                rtable.removeForIf( &netif );
                if ( rtable.isStub() ) {
                    if ( &netif != rtable.getStubOut() )
                        syncStubbyOut();             // propagate to output interface only
                    else {
                        rtable.destroyStub();        // the output interface was disconnected, try to find a new one
                        sendToOthers( RTable::Command::Hello );  // this makes sense only in cyclic networks
                    }
                    return;
                }
                sendToOthers(); // no stub -> propagate to all neighbours
            }
        } );

        LOCK_TCPIP_CORE();
        netif_add_noaddr( &netif, this, init, tcpip_input );
        UNLOCK_TCPIP_CORE();
        setRRP();
    }

    bool isConnected() {
        return connector.getState().connected;
    }

    void setUp() {
        LOCK_TCPIP_CORE();
        netif_create_ip6_linklocal_address( &netif, 0 );
        netif_set_up( &netif );
        UNLOCK_TCPIP_CORE();
        if ( isConnected() )
            sendRRP( RTable::Command::Hello );
    }

    bool sendRRP( RTable::Command cmd = RTable::Command::Call ) {
        ip_addr_t ip;
        ipaddr_aton( "ff02::1f", &ip );
        auto rrp = rtable.isHello( cmd )        ? rtable.createRRPhello( &netif, cmd )
                 : cmd == RTable::Command::Sync ? rtable.createRRPhello( &netif, cmd )
                                                : rtable.createRRPmsgIfless( &netif, cmd );

        LOCK_TCPIP_CORE();
        // use link-local address as source
        err_t res = raw_sendto_if_src( pcb, rrp.release(), &ip, &netif, &netif.ip6_addr[ 0 ] );
        UNLOCK_TCPIP_CORE();
        if ( res != ERR_OK )
            std::cerr << "Error - failed to send RRP msg: " << lwip_strerr( res ) << std::endl;
        return res == ERR_OK;
    }

    const Netif* getNetif() const {
        return &netif;
    }

    void setStubOut( const std::function< void() >& f ) {
        toStubOut = f;
    }

    void syncStubbyOut() {
        if ( toStubOut )
            toStubOut();
    }

private:
    void send( PBuf&& packet, uint16_t contentType = 0 ) {
        connector.send( contentType, std::move( packet ) );
    }

    static err_t init( struct netif* n ) {
        auto* self = reinterpret_cast< PhysNetif* >( n->state );
        n->hwaddr_len = 6;
        std::copy_n( self->pAddr.addr, 6, n->hwaddr );
        n->mtu = 120;
        n->name[ 0 ] = 'r'; n->name[ 1 ] = 'd';
        n->num = seqNum();
        n->output_ip6 = output;
        n->input = netif_input;
        n->linkoutput = linkOutput;
        n->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_UP | NETIF_FLAG_MLD6;
        return ERR_OK;
    }

    static err_t output( struct netif* n, struct pbuf *p, const ip6_addr_t* ip ) {
        auto self = reinterpret_cast< PhysNetif* >( n->state );
        self->send( PBuf::reference( p ) );

        return ERR_OK;
    }

    static err_t linkOutput( struct netif*, struct pbuf* ) {
        ROFI_UNREACHABLE( "Link output should not be directly used" );
    }

    void onPacket( Connector, uint16_t contentType, PBuf&& packet ) {
        if ( contentType == 0 ) {
            LOCK_TCPIP_CORE();
            netif.input( packet.release(), &netif );
            UNLOCK_TCPIP_CORE();
        }
    }

    void handleUpdate( RTable::Action act ) {
        switch( act ) {
            case RTable::Action::RespondToAll:
                sendToOthers();
                sendRRP( rtable.isStub() ? RTable::Command::Stubby : RTable::Command::Response );
                break;
            case RTable::Action::Respond:
                sendRRP( rtable.isStub() ? RTable::Command::Stubby : RTable::Command::Response );
                break;
            case RTable::Action::OnHello:
                if ( !rtable.isStub() )
                    sendToOthers();
                else
                    syncStubbyOut();
                sendRRP( RTable::Command::HelloResponse );
                break;
            case RTable::Action::CallToAll:
                sendToOthers();
                sendRRP();
                break;
            case RTable::Action::HelloToAll:
                sendToOthers( RTable::Command::Hello );
                sendRRP( RTable::Command::Hello );
                break;
            default: // Nothing (or Hello, which is not in use here)
                if ( rtable.isStub() )
                    syncStubbyOut();
                return;
        };
    }

    u8_t static onRRP( void *arg, struct raw_pcb*, struct pbuf *p, const ip_addr_t* ) {
        auto self = reinterpret_cast< PhysNetif* >( arg );
        if ( self ) {
            p = pbuf_free_header( p, IP6_HLEN );
            self->handleUpdate( self->rtable.update( PBuf::reference( p ), &self->netif ) );
            return 1;
        }
        return 0;
    }

    void setRRP() {
        create_rrp_listener();
        ip6_addr_t ip;
        ip6addr_aton( "ff02::1f", &ip );
        LOCK_TCPIP_CORE();
        auto res = mld6_joingroup_netif( &netif, &ip );
        UNLOCK_TCPIP_CORE();
        if ( res != ERR_OK )
            std::cerr << "Error - failed to join mld6 group: " << lwip_strerr( res ) << std::endl;
    }

    void create_rrp_listener() {
        LOCK_TCPIP_CORE();
        pcb = raw_new_ip6( IP6_NEXTH_ICMP6 );
        assert( pcb && "raw_new_ip6 failed, pcb is null!" );
        raw_bind_netif( pcb, &netif );
        raw_recv( pcb, onRRP, this );
        UNLOCK_TCPIP_CORE();
    }

    void sendToOthers( RTable::Command cmd = RTable::Command::Call ) {
        if ( _sendToOthers )
            _sendToOthers( cmd );
    }

    Netif netif;
    struct raw_pcb* pcb = nullptr;
    PhysAddr pAddr;
    Connector connector;
    RTable& rtable;
    std::function< void( RTable::Command ) > _sendToOthers;
    std::function< void() > toStubOut;

    friend class RoIF;
};


} // namespace rofinet
