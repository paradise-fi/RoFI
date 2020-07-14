#pragma once

#include "rofi_hal.hpp"
#include "routing_table.hpp"

#include <lwip/mld6.h>
#include <lwip/raw.h>
#include <lwip/ip6_addr.h>

#include <algorithm>
#include <string>
#include <cassert>
#include <sstream>

namespace rofinet {
	using namespace rofi::hal;

static int seqNum() {
    static int num = 0;
    return num++;
}

struct PhysAddr {
	PhysAddr( uint8_t *a ) {
		std::copy_n( a, 6, addr );
	}
	PhysAddr( uint8_t a, uint8_t b, uint8_t c, uint8_t d, uint8_t e, uint8_t f ) {
        addr[ 0 ] = a; addr[ 1 ] = b; addr[ 2 ] = c;
        addr[ 3 ] = d; addr[ 4 ] = e; addr[ 5 ] = f;
    }

	PhysAddr( const PhysAddr& ) = default;
    PhysAddr& operator=( const PhysAddr& ) = default;

	static int size() { return 6; }

    uint8_t addr[ 6 ];
};


class PhysNetif {
public:
	PhysNetif( PhysAddr pAddr, Connector connector, RTable& rt, const std::function< void( const Netif*, RTable::Command ) >& f )
	: pAddr( pAddr ), connector( connector ), rtable( rt ), _broadcast( f ) {
		connector.onPacket( [ this ]( Connector c, uint16_t contentType, PBuf&& pb ) {
			onPacket( c, contentType, std::move( pb ) );
		} );

		connector.onConnectorEvent( [ this ]( Connector, ConnectorEvent e) {
			if ( e == ConnectorEvent::Connected ) {
				netif.setActive( true );
				sendRRP( RTable::Command::Hello );
			} else {
				netif.setActive( false );
				rtable.removeForIf( &netif );
				if ( rtable.isStub() )
					syncStubbyOut(); // propagate to output interface only 
				else
					broadcast();     // propagate to all neighbours
			}
		} );

        netif_add( &netif, nullptr, nullptr, nullptr, this, init, tcpip_input );
        setRRP();
	}

	bool isConnected() {
		return connector.getState().connected;
	}

	void setUp() {
		netif_create_ip6_linklocal_address( &netif, 0 );
		netif_set_up( &netif );
		if ( isConnected() )
			sendRRP( RTable::Command::Hello );
	}

	bool sendRRP( RTable::Command cmd = RTable::Command::Call ) {
		ip_addr_t ip;
	    ipaddr_aton( "ff02::1f", &ip );
		auto rrp = rtable.isHello( cmd )        ? rtable.createRRPhello( &netif, cmd )
		         : cmd == RTable::Command::Sync ? rtable.createRRPhello( &netif, cmd )
		                                        : rtable.createRRPmsgIfless( &netif, cmd );
		err_t res = raw_sendto_if_src( pcb, rrp.release(), &ip, &netif, &netif.ip6_addr[ 0 ] ); // use link-local address as source
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
	void send( const Ip6Addr&, PBuf&& packet, int contentType = 0 ) {
		connector.send( contentType, std::move( packet) );
	}

	static err_t init( struct netif* n ) {
		auto self = reinterpret_cast< PhysNetif* >( n->state );
		n->hwaddr_len = 6;
		std::copy_n( self->pAddr.addr, 6, n->hwaddr );
		n->mtu = 120;
		n->name[ 0 ] = 'r'; n->name[ 1 ] = 'd';
		n->num = seqNum();
		n->output_ip6 = output;
		n->input = netif_input;
		n->linkoutput = linkOutput;
		n->flags |= NETIF_FLAG_IGMP | NETIF_FLAG_MLD6;
		return ERR_OK;
	}

	static err_t output( struct netif* n, struct pbuf *p, const ip6_addr_t* ip ) {
		auto self = reinterpret_cast< PhysNetif* >( n->state );
		self->send( Ip6Addr( *ip ), PBuf::reference( p ) );

		return ERR_OK;
	}

	static err_t linkOutput( struct netif*, struct pbuf* ) {
		assert( false && "Link output should not be directly used" );
		return ERR_OK;
	}

	void onPacket( Connector, uint16_t contentType, PBuf&& packet ) {
		if ( contentType == 0 ) {
			netif.input( packet.release(), &netif );
		}
	}

	void handleUpdate( RTable::Action act ) {
		switch( act ) {
			case RTable::Action::BroadcastRespond:
				sendRRP( rtable.isStub() ? RTable::Command::Stubby : RTable::Command::Response );
				broadcast( &netif );
				break;
			case RTable::Action::Respond:
				sendRRP( rtable.isStub() ? RTable::Command::Stubby : RTable::Command::Response );
				break;
			case RTable::Action::OnHello:
				sendRRP( RTable::Command::HelloResponse );
				break;
			case RTable::Action::BroadcastCall:
				broadcast( &netif );
				break;
			case RTable::Action::BroadcastHello:
				broadcast( &netif, RTable::Command::Hello );
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
		auto res = mld6_joingroup_netif( &netif, &ip );
		if ( res != ERR_OK )
			std::cout << "Error - failed to join mld6 group: " << lwip_strerr( res ) << std::endl;
	}

	void create_rrp_listener() {
		pcb = raw_new_ip6( IP6_NEXTH_ICMP6 );
		if ( pcb == nullptr ) 
			assert( false && "raw_new_ip6 failed, pcb is null!");
		raw_bind_netif( pcb, &netif );
		raw_recv( pcb, onRRP, this );
	}

	void broadcast( const Netif* without = nullptr, RTable::Command cmd = RTable::Command::Call ) {
		if ( _broadcast )
			_broadcast( without, cmd );
	}

	Netif netif;
	struct raw_pcb* pcb = nullptr;
	PhysAddr pAddr;
	Connector connector;
	RTable& rtable;
	std::function< void( const Netif*, RTable::Command ) > _broadcast;
	std::function< void() > toStubOut;
};
	

} // namespace rofinet
