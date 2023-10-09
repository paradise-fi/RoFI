#include <iostream>

#include <networking/networkManager.hpp>
#include <lwip++.hpp>

#include <networking/protocols/simplePeriodic.hpp>
#include <networking/protocols/leaderElect.hpp>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <peripherals/herculex.hpp>
#include <rofi_hal.hpp>

#include "udp6Example.hpp"

#include "esp_mac.h"

using namespace rofi::hal;


extern "C" void app_main()
{
    sleep( 5 );
    static uint8_t macAddress[ 6 ];
    esp_efuse_mac_get_default( macAddress );

    std::cout << "Starting communication example\n";

    tcpip_init( nullptr, nullptr );

    int id = macAddress[5] == 164 ? 1 : 2;
    std::cout << "ID: " + std::to_string( id ) + "\n";
    std::cout << "rofi has " << rofi::hal::RoFI::getLocalRoFI().getDescriptor().connectorCount << " connectors" << std::endl;

    Ip6Addr leaderIp = "fc07::a"_ip;
    uint8_t leaderMask = 46;


    std::cout << "Before network manager" << std::endl;
    rofi::net::NetworkManager net( rofi::hal::RoFI::getLocalRoFI() );
    std::cout << "Before adding protocol" << std::endl;
    auto rtProto = net.addProtocol( rofi::net::SimplePeriodic( 2000 ) );
    // std::cout << "Before adding protocol II" << std::endl;
    // auto leaderProto = net.addProtocol( rofi::net::LeaderElect( id, leaderIp, leaderMask ) );

    int i = 0;
    for (const auto& c : net.interfaces() ) {
        std::cout << "interface " << i++ << " : " << c.name() << std::endl;
    }

    std::cout << "Before ifs" << std::endl;
    if ( id == 1 ) {
        net.addAddress( "fc07:0:0:1::1"_ip, 64, net.interface( "rl1" ) );
    } else if ( id == 2 ) {
        net.addAddress( "fc07:0:0:2::2"_ip, 64, net.interface( "rl1" ) );
    } else if ( id == 3 ) {
        net.addAddress( "fc07:0:0:3::3"_ip, 64, net.interface( "rl1" ) );
    } else if ( id == 4 ) {
        net.addAddress( "fc07:0:0:4::4"_ip, 64, net.interface( "rl1" ) );
    } else {
        std::runtime_error( "more than 4 bots!" );
    }

    std::cout << "After ifs" << std::endl;

    net.setUp();

    std::cout << "After net set up" << std::endl;
    /*
    net.setProtocol( *leaderProto );
    std::cout << "After set protocol" << std::endl;
    */
    std::cout << "Setting rl1" << std::endl;
    net.setProtocol( *rtProto, net.interface( "rl1" ) );
    std::cout << "Setting rd2" << std::endl;
    net.setProtocol( *rtProto, net.interface( "rd2" ) );

    std::cout << "After set protocol II" << std::endl;

    if ( id == 1 ) {
        udpEx6::runMaster();
    } else {
        udpEx6::runSlave( "fc07:0:0:1::1" );
    }

    std::cout << "Ending communication example\n";
}
