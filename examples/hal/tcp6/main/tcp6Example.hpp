#pragma once

#include <lwip/tcp.h>
#include <iostream>

#include "dock.hpp"
#include "roif6.hpp"

namespace tcp6Ex {

static int connCounter = 0;

void onError( void* arg, err_t err ) {
    std::cout << "Error: " << lwip_strerr( err ) << "\n";
}

err_t masterReceive( void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err ) {
	std::cout << "Master receive!" << std::endl;

    int conn = reinterpret_cast< int >( arg );
    if ( err == ERR_OK && p == nullptr ) {
        // close the connection
        tcp_arg( pcb, nullptr );
        tcp_sent( pcb, nullptr );
        tcp_recv( pcb, nullptr );
        tcp_close( pcb );
        std::cout << "Connection " << conn << " closed\n";
        return ERR_OK;
    }

    auto data = _rofi::PBuf::own( p );
    if ( err == ERR_OK ) {
        std::cout << "Connection " << conn << ": " << data.asString() << "\n";
        tcp_write( pcb, data.payload(), data.size(), 0 );
        tcp_sent( pcb, nullptr );
    } else {
        std::cout << "Connection " << conn << " error: " << lwip_strerr( err ) << "\n";
    }

    return ERR_OK;
}

err_t accept( void *arg, tcp_pcb *pcb, err_t err ) {
    tcp_arg( pcb, reinterpret_cast< void* >( ++connCounter ) );
    std::cout << "New connection (" << connCounter << ") established\n";

    tcp_setprio( pcb, TCP_PRIO_MIN );
    tcp_recv( pcb, masterReceive );
    tcp_err( pcb, onError );
    tcp_poll( pcb, NULL, 4 );
    return ERR_OK;
}

void runMaster() {
    std::cout << "Starting TCP server\n";
    tcp_pcb* pcb = tcp_new();
    tcp_bind( pcb, IP6_ADDR_ANY, 7777 );

    pcb = tcp_listen( pcb );

    tcp_accept( pcb, accept );
    std::cout << "Waiting for new clients\n";
    while ( true )
        vTaskDelay( 2000 / portTICK_PERIOD_MS );
}

err_t slaveReceive( void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err ) {
    if ( err == ERR_OK && p == nullptr ) {
        // close the connection
        tcp_close( pcb );
        std::cout << "Connection " << "closed\n";
        return ERR_OK;
    }

    auto data = _rofi::PBuf::own( p );
    if ( err == ERR_OK ) {
        std::cout << "Received: " << data.asString() << "\n";
	} else {
        std::cout << "Connection error: " << lwip_strerr( err ) << "\n";
	}

    return ERR_OK;
}

err_t send(void *arg, struct tcp_pcb *pcb, u16_t len ) {
    vTaskDelay( 500 / portTICK_PERIOD_MS );
    const char* message = "Hello world!";
    std::cout << "Sending: " << message << "\n";
    tcp_write( pcb, message, strlen( message ), 0 );
    return ERR_OK;
}

err_t connect( void *arg, struct tcp_pcb* pcb, err_t err ) {
    std::cout << "Connected to a server\n";
    send( arg, pcb, 0 );
    return ERR_OK;
}


void runSlave( const char* masterAddr ) {
    std::cout << "Starting TCP client\n";
    vTaskDelay( 7000 / portTICK_PERIOD_MS );
    tcp_pcb* pcb = tcp_new();
    tcp_recv( pcb, slaveReceive );
    tcp_sent( pcb, send );

    tcp_err( pcb, onError );

    ip_addr_t addr;
    if ( !ipaddr_aton( masterAddr, &addr ) )
        std::cout << "Cannot create IP address\n";

    err_t err;
    if ( ( err = tcp_connect( pcb, &addr, 7777, connect ) ) != ERR_OK ) {
        std::cout << "Cannot connect: " << lwip_strerr( err ) << "\n";
    }
    std::cout << "Waiting for connection\n";
}

} // namespace tcp6Ex
