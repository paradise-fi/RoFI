#include <iostream>

#include <networking/networkManager.hpp>
#include <lwip++.hpp>

#include <networking/protocols/simplePeriodic.hpp>
#include <networking/protocols/leaderElect.hpp>

#include "udp6Example.hpp"

using namespace rofi::hal;

int main()
{
    std::cout << "Starting communication example\n";

    tcpip_init( nullptr, nullptr );

    int id = rofi::hal::RoFI::getLocalRoFI().getId();
    std::cout << "ID: " + std::to_string( id ) + "\n";

    Ip6Addr leaderIp = "fc07::a";
    uint8_t leaderMask = 46;

    rofinet::NetworkManager net( rofi::hal::RoFI::getLocalRoFI() );
    net.addProtocol( rofinet::SimplePeriodic( 2000 ) );
    net.addProtocol( rofinet::LeaderElect( id, leaderIp, leaderMask ) );

    if ( id == 1 ) {
        net.addAddress( "fc07:0:0:1::1"_ip, 64, net.interface( "rl0" ) );
    } else if ( id == 2 ) {
        net.addAddress( "fc07:0:0:2::2"_ip, 64, net.interface( "rl0" ) );
    } else if ( id == 3 ) {
        net.addAddress( "fc07:0:0:3::3"_ip, 64, net.interface( "rl0" ) );
    } else if ( id == 4 ) {
        net.addAddress( "fc07:0:0:4::4"_ip, 64, net.interface( "rl0" ) );
    } else {
        std::runtime_error( "more than 4 bots!" );
    }

    net.setUp();

    auto* protocol = net.getProtocol( "leader-elect" );
    for ( auto& i : net.interfaces() ) {
        net.setProtocol( *protocol, i );
    }
    protocol = net.getProtocol( "simple-periodic" );
    for ( auto& i : net.interfaces() ) {
        net.setProtocol( *protocol, i );
    }

    if ( id == 1 ) {
        udpEx6::runMaster();
    } else {
        udpEx6::runSlave( "fc07::a" );
    }

    std::cout << "Ending communication example\n";
}
