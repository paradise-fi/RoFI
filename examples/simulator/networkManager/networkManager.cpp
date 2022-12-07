#include <networking/networkManagerCli.hpp>
#include "lwip++.hpp"

#include <networking/protocols/simpleReactive.hpp>
#include <networking/protocols/simplePeriodic.hpp>
#include <networking/protocols/rrp.hpp>

#include <networking/protocols/leaderElect.hpp>

#include <lwip/udp.h>
#include <iostream>

using namespace rofi::hal;
using namespace rofi::net;

/*
 * Simple demo of the Network Manager and its CLI interface. For routing to start,
 * you have to enable chosen protocol on connected interfaces. For example, you can
 * populate the routing table on world set0 with enabling the protocol on interfaces
 * rl0, rd3, and rd6. You can do it with those commands (we use simple-reactive
 * protocol, but you can use other) run on every module:
 * 
 * > netmg proto simple-reactive manage rl0
 * > netmg proto simple-reactive manage rd3
 * > netmg proto simple-reactive manage rd6
 * 
 * Print out the routing table state to see available routes
 * 
 * > netmg table show
*/

void printHelp() {
    const char* msg =
        "There are three commands available.\n"
        "\t netmg <command> .... control the networking (NetworkManager)\n"
        "\t msg ................ send a message (starts an interactive session)\n"
        "\t message ............ send a message (same as msg above)\n"
        "\t connect ............ connect a connector of the local RoFI (starts an interactive session\n"
        "\t disconnect ......... complementary to the previous action\n"
        "\t help | ?............ display this message\n";
    std::cout << msg;
}

void onMessage(  void*, struct udp_pcb*, struct pbuf* p, const ip6_addr_t* addr, u16_t ) {
    if ( !p || !addr )
        return;

    auto packet = PBuf::own( p );
    std::cout << "Got message from " << Ip6Addr( *addr ) << "; msg: " << packet.asString() << "\n";
    std::cout << "> " << std::flush;
}

udp_pcb* setUpListener() {
    udp_pcb* pcb = udp_new();
    if ( !pcb ) {
        throw std::runtime_error( "pcb is null" );
    }

    udp_bind( pcb, IP6_ADDR_ANY, 7777 );
    udp_recv( pcb, onMessage, nullptr );

    return pcb;
}

std::pair< Ip6Addr, std::string > getMsgToSend() {
    std::string ip, msg;
    std::cout << "send message to: ";
    std::getline( std::cin, ip );
    std::cout << "message text: ";
    std::getline( std::cin, msg );
    return { Ip6Addr( ip ), msg };
}

void sendMessage( const Ip6Addr& ip, const std::string& msg, udp_pcb* pcb ) {
    if ( !pcb ) {
        std::cout << "pcb is null\n";
        return;
    }

    int size = static_cast< int >( msg.length() );
    auto packet = PBuf::allocate( size );
    for ( int i = 0; i != size; i++ ) {
        packet[ i ] = msg[ i ];
    }

    err_t res = udp_sendto( pcb, packet.release(), &ip, 7777 );
    if ( res != ERR_OK ) {
        std::cout << "send failed with " << lwip_strerr( res ) << std::endl;
    }
}

void handleConnector( bool connect ) {
    int connectorIdx;
    auto rofi = RoFI::getLocalRoFI();
    std::cout << "which connector do you want to disconnect? (0 - "
              << rofi.getDescriptor().connectorCount << "): ";
    std::cin >> connectorIdx;
    if ( connectorIdx >= 0 && connectorIdx < rofi.getDescriptor().connectorCount ) {
        if ( connect )
            rofi.getConnector( connectorIdx ).connect();
        else {
            rofi.getConnector( connectorIdx ).disconnect();
        }
    } else {
        std::cout << "index out of range! ignoring..." << std::endl;
    }
}

int main() {
    std::cout << "Starting NetworkManager example\n";

    tcpip_init( nullptr, nullptr );

    int id = RoFI::getLocalRoFI().getId();
    std::cout << "ID: " + std::to_string( id ) + "\n";

    NetworkManager net( RoFI::getLocalRoFI() );
    NetworkManagerCli netcli( net );

    if ( id == 1 ) {
        net.addAddress( "fc07:0:0:1::1"_ip, 80, net.interface( "rl0" ) );
    } else if ( id == 2 ) {
        net.addAddress( "fc07:0:0:2::1"_ip, 80, net.interface( "rl0" ) );
    } else if ( id == 3 ) {
        net.addAddress( "fc07:0:0:3::1"_ip, 80, net.interface( "rl0" ) );
    } else if ( id == 4 ) {
        net.addAddress( "fc07:0:0:4::1"_ip, 80, net.interface( "rl0" ) );
    } else {
        throw std::runtime_error( "more than 4 bots!" );
    }

    // ToDo: Maybe addAddress might return an optional< index > instead of bool?
    //       Then you could write "just" net.interface( "rl0" ).get().getAddress( index ). 
    std::cout << "address: " << net.interface( "rl0" ).getAddress().front().first << std::endl;
    net.setUp();

    auto* pcb = setUpListener();

    net.addProtocol( RRP() );
    net.addProtocol( SimplePeriodic() );
    net.addProtocol( SimpleReactive() );
    net.addProtocol( LeaderElect( id, "fc07:a::a", 96 ) );

    /*
     * If you want to set-up some protocol on all interfaces, you can do it by
     * uncommenting following for with an according line for the chosen protocol.  
    for ( const auto& i : net.interfaces() ) {
        net.setProtocol( *net.getProtocol( "leader-elect" ), net.interface( i.name() ) );
        net.setProtocol( *net.getProtocol( "rrp" ), net.interface( i.name() ) );
        net.setProtocol( *net.getProtocol( "simple-periodic" ), net.interface( i.name() ) );
        net.setProtocol( *net.getProtocol( "simple-reactive" ), net.interface( i.name() ) );
    }
    */

    std::string line;
    std::cout << "for help type help or ?\n> ";
    while ( std::getline( std::cin, line ) ) {
        if ( line.empty() ) {
            std::cout << "> ";
            continue;
        } else if ( line == "end" || line == "q" || line == "quit" ) {
            break;
        } else if ( line == "?" || line == "help" ) {
            printHelp();
            std::cout << "> ";
            continue;;
        }

        try {
            if ( line == "msg" || line == "message" ) {
                auto [ ip, msg ] = getMsgToSend();
                sendMessage( ip, msg, pcb );
            } else if ( line == "connect" || line == "disconnect" ) {
                handleConnector( line == "connect" );
            } else if ( netcli.command( line ) ) {
                // Ok, command parsed
            } else {
                // do nothing
                std::cout << "Do nothing in else branch";
            }
        } catch ( const std::exception& e ) {
            std::cout << "Bad input: " << e.what() << std::endl;
        } 


        std::cout << "> ";
    }

    std::cout << "Ending NetworkManager example\n";

    return 0;
}
