#pragma once

#include "physical_netif.hpp"

#include <lwip/netif.h>
#include <lwip/ip6.h>

#include <cassert>
#include <vector>
#include <sstream>
#include <iostream>

namespace rofinet {
    using namespace rofi::hal;

class RoIF : public Netif {
public:
    RoIF( RoFI rofi ) : RoIF( rofi, createAddress( rofi.getId() ), 80 ) {}

    RoIF( RoFI rofi, const Ip6Addr& ip, uint8_t mask ) {
        int id = rofi.getId();
        LOCK_TCPIP_CORE();
        netif_add_noaddr( &netif, this, init, tcpip_input );
        UNLOCK_TCPIP_CORE();
        addAddress( ip, mask );
        LOCK_TCPIP_CORE();
        netif_set_default( &netif );
        UNLOCK_TCPIP_CORE();

        int connectors = rofi.getDescriptor().connectorCount;
        uint8_t im = static_cast< uint8_t >( id );
        PhysAddr pAddr( im, im, im, im, im, im );

        netifs.reserve( connectors );
        for ( int i = 0; i < connectors; i++ ) {
            netifs.emplace_back( pAddr, rofi.getConnector( i ), rtable, [ index = i, this ]( RTable::Command cmd ) {
                broadcastRTableIfless( netifs[ index ].getNetif(), cmd );
            } );
            netifs.back().setStubOut( [ this ]() { syncStub(); } );
        }
    }

    bool addAddress( const Ip6Addr& ip, uint8_t mask ) {
        s8_t index = -1;
        LOCK_TCPIP_CORE();
        netif_add_ip6_address( &netif, &ip, &index );
        netif_ip6_addr_set_state( &netif, index, IP6_ADDR_VALID );
        UNLOCK_TCPIP_CORE();
        if ( index >= 0 && rtable.add( ip, mask, 0, &netif ) ) {
            if ( rtable.isStub() )
                syncStub();
            else
                broadcastRTable();
            return true;
        }
        return false;
    }

    bool removeAddress( const Ip6Addr& ip, uint8_t mask ) {
        int index = 0;
        for ( ; index < LWIP_IPV6_NUM_ADDRESSES; index++ ) {
            if ( ip == Ip6Addr( ip6_addr[ index ] ) ) {
                ip_addr_set_zero( &ip6_addr[ index ] );
                if ( rtable.remove( ip, mask, this ) ) {
                    if ( rtable.isStub() )
                        syncStub();
                    else
                        broadcastRTable();
                    return true;
                }
                return false;
            }
        }

        return false;
    }

    void setUp() {
        LOCK_TCPIP_CORE();
        netif_set_up( &netif );
        UNLOCK_TCPIP_CORE();
        for ( auto& n : netifs ) {
            n.setUp();
        }
    }

    void broadcastRTable( RTable::Command cmd = RTable::Command::Call ) {
        broadcastRTableIfless( nullptr, cmd );
    }

    const RTable& getRTable() const {
        return rtable;
    }

private:
    static err_t init( struct netif* roif ) {
        roif->mtu = 120;
        roif->name[ 0 ] = 'r'; roif->name[ 1 ] = 'l';
        roif->num = seqNum();
        roif->output_ip6 = output;
        roif->linkoutput = linkOutput;
        roif->flags |= NETIF_FLAG_LINK_UP | NETIF_FLAG_IGMP | NETIF_FLAG_UP | NETIF_FLAG_MLD6;
        return ERR_OK;
    }

    static err_t output( struct netif* roif, struct pbuf *p, const ip6_addr_t *ipaddr ) {
        auto* n = ip_find_route( ipaddr, &roif->ip6_addr[ 1 ] );
        if ( roif && n ) {
            return ip6_output_if_src(p, &roif->ip6_addr[ 1 ], LWIP_IP_HDRINCL, 0, 0, 0, n);
        }
        return ERR_OK;
    }

    static err_t linkOutput( struct netif*, struct pbuf* ) {
        ROFI_UNREACHABLE( "Link output should not be directly used" );
    }

    void syncStub() {
        Netif* net = rtable.getStubOut();
        if ( !net )
            return;

        for ( auto& n : netifs ) {
            if ( n.getNetif() == net ) {
                n.sendRRP( RTable::Command::Sync );
                rtable.clearChanges();
                break;
            }
        }
    }

    Ip6Addr createAddress ( int id ) {
        std::ostringstream s;
        s << "fc07::" << id << ":0:0:1";
        return Ip6Addr( s.str().c_str() );
    }

    void broadcastRTableIfless( const Netif* netif, RTable::Command cmd = RTable::Command::Call ) {
        for ( auto& n : netifs ) {
            const Netif* out = n.getNetif();
            if ( n.isConnected() && netif != out && ( out && out->isActive() && !out->isStub() ) )
                n.sendRRP( cmd );
        }
        rtable.clearChanges();
    }

    Netif netif;
    RTable rtable;
    std::vector< PhysNetif > netifs;
};

} // namespace rofinet
