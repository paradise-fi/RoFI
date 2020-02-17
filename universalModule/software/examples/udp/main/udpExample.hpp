#pragma once

#include <iostream>

#include <lwip/udp.h>
#include "dock.hpp"
#include "roif.hpp"

namespace udpEx {

inline void onMasterPacket(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    std::cout << "Master receive\n";
    if ( !p ) return;

    auto packet = _rofi::PBuf::own( p );
    std::cout << _rofi::Ip4Addr( addr->u_addr.ip4 ) << ":" << port << " sent: "
              << packet.asString() << "\n";

	udp_sendto( pcb, packet.get(), addr, port );
}

inline void onSlavePacket(void *arg, struct udp_pcb *pcb, struct pbuf *p,
    const ip_addr_t *addr, u16_t port)
{
    if ( !p ) return;

    auto packet = _rofi::PBuf::own( p );
    std::cout << _rofi::Ip4Addr( addr->u_addr.ip4 ) << ":" << port << " responded: "
              << packet.asString() << "\n";
}

inline void runMaster() {
    std::cout << "Starting UDP echo server\n";

    udp_pcb* pcb = udp_new();
    udp_bind( pcb, IP_ADDR_ANY, 7777 );
    std::cout << "UDP binded\n";
    udp_recv( pcb , onMasterPacket, nullptr );

    while ( true )
        vTaskDelay( 2000 / portTICK_PERIOD_MS );
}

inline void runSlave( const char* masterAddr ) {
    std::cout << "Starting UDP client\n";

    udp_pcb* pcb = udp_new();
    udp_bind( pcb, IP_ADDR_ANY, 7777 );
    udp_recv( pcb , onSlavePacket, nullptr );

    ip_addr_t addr;
    if ( !ipaddr_aton( masterAddr, &addr ) )
        std::cout << "Cannot create IP address\n";
    int counter = 0;
    while ( true ) {
        const char* message = "Hello world!  ";
        const int len = strlen( message );
        auto buffer = _rofi::PBuf::allocate( len );
        for ( int i = 0; i != len + 1; i++ ) {
            buffer[ i ] = message[ i ];
        }
        buffer[ len - 1 ] = 'a' + ( counter++ ) % 26;
        std::cout << "Sending message: " << buffer.asString() << "\n";
        udp_sendto( pcb, buffer.get(), &addr, 7777 );
        vTaskDelay( 1000 / portTICK_PERIOD_MS );
    }
}

} // namespace udpEx