#pragma once

#include "physical_netif.hpp"

#include <cassert>
#include <vector>
#include <sstream>

namespace rofinet {
	using namespace rofi::hal;

class RoIF : public Netif {
public:
	RoIF( RoFI rofi ) : RoIF( rofi, createAddress( rofi.getId() ), 128 ) {}
	RoIF( RoFI rofi, const Ip6Addr& ip, uint8_t mask ) {
		int id = rofi.getId();
		netif_add( &netif, nullptr, nullptr, nullptr, this, init, tcpip_input );
		addAddress( ip, mask );
		netif_set_default( &netif );

		int connectors = rofi.getDescriptor().connectorCount;
		PhysAddr pAddr( id, id, id, id, id, id );

		netifs.reserve( connectors );
		for ( int i = 0; i < connectors; i++ ) {
			netifs.emplace_back( pAddr, rofi.getConnector( i ), rtable, [ this ]( const Netif* n ) {
				broadcastRTableIfless( n );
			} );
		}
	}

	bool addAddress( const Ip6Addr& ip, uint8_t mask ) {
		s8_t index = -1;
		netif_add_ip6_address( &netif, &ip, &index );
		return index >= 0 && rtable.add( ip, mask, 0, &netif );
	}

	bool removeAddress( const Ip6Addr& ip, uint8_t mask ) {
		int index = 0;
		for ( ; index < LWIP_IPV6_NUM_ADDRESSES; index++ ) {
			if ( ip == Ip6Addr( *ip_2_ip6( &ip6_addr[ index ] ) ) ) {
				ip_addr_set_zero( &ip6_addr[ index ] );
				return rtable.remove( ip, mask, this );
			}
		}

		return false;
	}

	void setUp() {
        netif_set_up( &netif );
		for ( auto& n : netifs ) {
			n.setUp();
		}
	}

	void send( const Ip6Addr& ip, PBuf&& packet, int contentType = 0 ) {
		auto n = ip_find_route( &ip );
		if ( n )
			n->output_ip6( n, packet.release(), &ip );
	}

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
        auto self = reinterpret_cast< RoIF* >( roif );
        self->send( Ip6Addr( *ipaddr ), PBuf::reference( p ) );
        return ERR_OK;
    }

    static err_t linkOutput( struct netif*, struct pbuf* ) {
        assert( false && "Link output should not be directly used" );
        return ERR_OK;
    }

	void broadcastRTable( RTable::Command cmd = RTable::Command::Call ) {
		broadcastRTableIfless( nullptr, cmd );
	}

	const RTable& getRTable() const {
		return rtable;
	}

private:
	Ip6Addr createAddress ( int id ) {
		std::ostringstream s;
		s << "fc07::" << id;
		return Ip6Addr( s.str().c_str() );
	}

	void broadcastRTableIfless( const Netif* netif, RTable::Command cmd = RTable::Command::Call ) {
		for ( auto& n : netifs ) {
			if ( n.isConnected() && netif != n.getNetif() )
				n.sendRRP( cmd );
		}
	}

	Netif netif;
	RTable rtable;
	std::vector< PhysNetif > netifs;
};

} // namespace rofinet
