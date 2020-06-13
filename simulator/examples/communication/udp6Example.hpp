#pragma once

#include <iostream>

#include <lwip/udp.h>
#include "roif6.hpp"

namespace udpEx6 {
	using namespace _rofi;

inline void onMasterPacket(void *, struct udp_pcb *pcb, struct pbuf *p,
	const ip_addr_t *addr, u16_t port)
{
	if ( !p ) return;

	auto packet = rofi::hal::PBuf::own( p );
	std::cout << Ip6Addr( addr->u_addr.ip6 )
		<< "; port: " << port << " sent: "	<< packet.asString() << std::endl;
	
	udp_sendto( pcb, packet.get(), addr, port );
}

inline void onSlavePacket(void *, struct udp_pcb *, struct pbuf *p,
	const ip_addr_t *addr, u16_t port)
{
	if ( !p ) return;

	auto packet = rofi::hal::PBuf::own( p );
	std::cout << Ip6Addr( addr->u_addr.ip6 )
		<< "; port: " << port << " responded: " << packet.asString() << std::endl;
}

inline void runMaster() {
	std::cout << "Starting UDP echo server (udp6ex)\n";

	udp_pcb* pcb = udp_new();
	if (!pcb) 
		std::cout << "pcb is null" << std::endl;
	int res = udp_bind( pcb, IP6_ADDR_ANY, 7777 );
	std::cout << "UDP binded (" << res << ")" << std::endl;
	udp_recv( pcb, onMasterPacket, nullptr );

	while ( true )
		sleep( 2 );
}

inline void runSlave( const char* masterAddr ) {
	std::cout << "Starting UDP client\n";

	udp_pcb* pcb = udp_new();
	if (!pcb)
		std::cout << "pcb is null" << std::endl;
	int res = udp_bind( pcb, IP6_ADDR_ANY, 7777 );
	std::cout << "UDP binded (" << res << ")" << std::endl;
	udp_recv( pcb, onSlavePacket, nullptr );

	ip_addr_t addr;
	if ( !ipaddr_aton( masterAddr, &addr ))
		std::cout << "Cannot create IP address" << std::endl;
	int counter = 0;
	while ( true ) {
		const char* message = "Hello world!  ";
		const int len = strlen( message );
		auto buffer = rofi::hal::PBuf::allocate( len );
		for ( int i = 0; i != len + 1; i++ ) {
			buffer[ i ] = message[ i ];
		}
		buffer[ len - 1 ] = 'a' + ( counter++ ) % 26;
		std::cout << "Sending message: " << buffer.asString() << " to " << Ip6Addr( addr.u_addr.ip6 ) << std::endl;
		res = udp_sendto( pcb, buffer.get(), &addr, 7777 );
		sleep( 2 );
	}
}

} // namespace udpEx6
