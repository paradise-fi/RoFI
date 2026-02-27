#pragma once

#include <iostream>

#include <lwip/udp.h>

namespace udpEx6
{
using namespace rofi::net;

inline void onMasterPacket( void *,
                            struct udp_pcb * pcb,
                            struct pbuf * p,
                            const ip6_addr_t* addr,
                            u16_t port )
{
    if ( !p )
    {
        return;
    }

    auto packet = rofi::hal::PBuf::own( p );
    std::cout << Ip6Addr( *addr ) << "; port: " << port << " sent: " << packet.asString()
              << std::endl;

    auto res = udp_sendto( pcb, packet.release(), addr, port );
    if ( res != ERR_OK )
        std::cout << "udp_sendto returned " << lwip_strerr( res ) << "\n";
}

inline void onSlavePacket( void *,
                           struct udp_pcb *,
                           struct pbuf * p,
                           const ip6_addr_t * addr,
                           u16_t port )
{
    if ( !p )
    {
        return;
    }

    auto packet = rofi::hal::PBuf::own( p );
    std::cout << Ip6Addr( *addr ) << "; port: " << port
              << " responded: " << packet.asString() << std::endl;
}

inline void runMaster()
{
    std::cout << "Starting UDP echo server (udp6ex)\n";

    LOCK_TCPIP_CORE();
    udp_pcb * pcb = udp_new();
    UNLOCK_TCPIP_CORE();
    if ( !pcb )
    {
        std::cout << "pcb is null" << std::endl;
    }

    LOCK_TCPIP_CORE();
    int res = udp_bind( pcb, IP6_ADDR_ANY, 7777 );
    std::cout << "UDP binded (" << res << ")" << std::endl;
    udp_recv( pcb, onMasterPacket, nullptr );
    UNLOCK_TCPIP_CORE();

    while ( true )
    {
        sleep( 4 );
    }
}

inline void runSlave( const char * masterAddr )
{
    std::cout << "Starting UDP client\n";
    std::cout << "Master address given: " << masterAddr << "\n";

    LOCK_TCPIP_CORE();
    udp_pcb * pcb = udp_new();
    UNLOCK_TCPIP_CORE();
    if ( !pcb )
    {
        std::cout << "pcb is null" << std::endl;
    }

    Ip6Addr addr( masterAddr );

    LOCK_TCPIP_CORE();
    err_t res = udp_bind( pcb, IP6_ADDR_ANY, 7777 );
    std::cout << "UDP binded (" << static_cast< int >( res ) << ")" << std::endl;
    udp_recv( pcb, onSlavePacket, nullptr );
    UNLOCK_TCPIP_CORE();

    int counter = 0;
    while ( true )
    {
        const char * message = "Hello world!  ";
        const int len = static_cast< int >( strlen( message ) );
        auto buffer = rofi::hal::PBuf::allocate( len );
        for ( int i = 0; i != len + 1; i++ )
        {
            buffer[ i ] = message[ i ];
        }
        buffer[ len - 1 ] = static_cast< uint8_t >( 'a' + ( counter++ ) % 26 );
        std::cout << "Sending message: " << buffer.asString() << " to "
                  << addr << std::endl;

        LOCK_TCPIP_CORE();
        res = udp_sendto( pcb, buffer.release(), &addr, 7777 );
        UNLOCK_TCPIP_CORE();

        if ( res != ERR_OK )
        {
            std::cout << "udp_sendto returned " << lwip_strerr( res ) << "\n";
        }
        sleep( 4 );
    }
}

} // namespace udpEx6
