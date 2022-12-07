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

    rofi::net::NetworkManager net( rofi::hal::RoFI::getLocalRoFI() );
    auto rtProto = net.addProtocol( rofi::net::SimplePeriodic( 2000 ) );
    auto leaderProto = net.addProtocol( rofi::net::LeaderElect( id, leaderIp, leaderMask ) );

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
    net.setProtocol( *leaderProto );
    net.setProtocol( *rtProto );

    if ( id == 1 ) {
        udpEx6::runMaster();
    } else {
        udpEx6::runSlave( "fc07::a" );
    }

    std::cout << "Ending communication example\n";
}
