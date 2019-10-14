#pragma once

#include <iostream>

#include <lwip/udp.h>
#include "dock.hpp"
#include "roif6.hpp"

std::ostream& printA( std::ostream& o, ip6_addr_t a ) {
	o << IP6_ADDR_BLOCK1(&a) << ":" << IP6_ADDR_BLOCK2(&a) << ":" << IP6_ADDR_BLOCK3(&a)
		<< ":" << IP6_ADDR_BLOCK4(&a) << ":" << IP6_ADDR_BLOCK5(&a) << ":" << IP6_ADDR_BLOCK6(&a)
		<< ":" << IP6_ADDR_BLOCK7(&a) << ":" << IP6_ADDR_BLOCK8(&a);
	return o;
}

namespace udpEx6 {

inline void onMasterPacket(void *arg, struct udp_pcb *pcb, struct pbuf *p,
	const ip_addr_t *addr, u16_t port)
{
	if ( !p ) return;

	auto packet = _rofi::PBuf::own( p );
	std::cout << std::hex << _rofi::Ip6Addr( addr->u_addr.ip6 ) << ":" << port << " sent: "
				<< packet.asString() << " (on master packet)" << std::dec << std::endl;
	
	udp_sendto( pcb, packet.get(), addr, port );
	// free(p);
}

inline void onSlavePacket(void *arg, struct udp_pcb *pcb, struct pbuf *p,
	const ip_addr_t *addr, u16_t port)
{
	if ( !p ) return;

	auto packet = _rofi::PBuf::own( p );
	std::cout << std::hex << _rofi::Ip6Addr( addr->u_addr.ip6 ) << ":" << port << " responded: "
				<< packet.asString() << " (on slave packet)" << std::dec << std::endl;
	// free(p);
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
		vTaskDelay( 2000 / portTICK_PERIOD_MS );
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
		auto buffer = _rofi::PBuf::allocate( len );
		for ( int i = 0; i != len + 1; i++ ) {
			buffer[ i ] = message[ i ];
		}
		buffer[ len - 1 ] = 'a' + ( counter++ ) % 26;
		std::cout << "Sending message: " << buffer.asString() << " to ";
		std::cout << std::hex; printA(std::cout, *(ip6_addr_t*) &addr) << std::dec << std::endl;
		res = udp_sendto( pcb, buffer.get(), &addr, 7777 );
		vTaskDelay( 2000 / portTICK_PERIOD_MS );
	}
}

} // namespace udpEx6
