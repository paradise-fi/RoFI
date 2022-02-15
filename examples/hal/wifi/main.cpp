#include <iostream>
#include <sstream>
#include <cmath>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <fi.hpp>
#include <util.hpp>
#include <socketServer.hpp>

#include <rofi_hal.hpp>

struct JointController {
    JointController( rofi::hal::Joint j ): j( j ) {
        move();
    }

    float clamp( float value ) {
        if ( value > 3.14 * 1.1 / 2 )
            return 3.14 * 1.1 / 2;
        if ( value < -3.14 * 1.1 / 2 )
            return -3.14 * 1.1 / 2;
        return value;
    }

    void moveTo( float value ) {
        pos = clamp( value );
        move();
    }

    void moveUp() {
        pos += 3.14 / 2 / 5;
        if ( pos > 3.14 * 1.1 / 2 )
            pos = 3.14 / 2 * 1.1;
        move();
    }

    void moveDown() {
        pos -= 3.14 / 2 / 5;
        if ( pos < -3.14 * 1.1 / 2 )
            pos = -3.14 / 2 * 1.1;
        move();
    }

    void move() {
        try {
            std::cout << "Moving to " << ( pos * 180 / 3.14 ) << "\n";
            j.setPosition( pos , 1, []( rofi::hal::Joint ){} );
        }
        catch( const std::runtime_error& e ) {
            std::cout << "Cannot set position, check connection: " << e.what() << "\n";
        }
    }

    rofi::hal::Joint j;
    float pos = 0;
};

extern "C" void app_main() {

    esp_log_level_set("esp_netif_lwip", ESP_LOG_VERBOSE);

    std::cout << "Program starts\n";
    rofi::fi::initNvs();
    rofi::fi::WiFiConnector c;
    if ( c.sync( 5 ).connect(SSID, PASSWORD) )
        std::cout << "Connected: " << c.ipAddrStr() << "\n";
    else
        std::cout << "Connection failed\n";
    c.waitForIp();
    std::cout << "Got IP: " << c.ipAddrStr() << "\n";

    esp_wifi_set_ps (WIFI_PS_NONE);

    if ( std::string( SSID ) == "wlan_fi" )
        rofi::fi::authWlanFi( WLAN_USER, WLAN_PASS );
    std::cout << "Autenticated!\n";

    auto rof = rofi::hal::RoFI::getLocalRoFI();
    JointController left( rof.getJoint( 0 ) );
    JointController right( rof.getJoint( 1 ) );
    auto connector1 = rof.getConnector( 0 );
    auto connector2 = rof.getConnector( 1 );
    auto connector5 = rof.getConnector( 4 );

    rofi::net::SocketServer server;
    server.setClientRoutine( [&]( rofi::net::SocketClient client, sockaddr_in addr ) {
        try {
            std::cout << "Got new client\n";
            client.write( "Hello from RoFI\n" );
            rofi::util::LineReader lineReader;
            lineReader.bind( [&]( std::string line ) {
                if ( line.empty() )
                    return;
                std::istringstream is( line );
                std::string command;
                is >> command;
                std::cout << "Command: " << command << "\n";
                if ( command == "move" ) {
                    std::string shoe;
                    float position;
                    is >> shoe >> position;
                    position = position / 180 * M_PI;
                    std::cout << "  " << shoe << " " << position << "\n";
                    if ( shoe == "left" )
                        left.moveTo( position );
                    else if ( shoe == "right" )
                        right.moveTo( position );
                }

                if ( command == "connector" ) {
                    std::string action, connector;
                    is >> connector >> action;
                    if ( connector == "right" ) {
                        if ( action == "expand" )
                            connector2.connect();
                        else
                            connector2.disconnect();
                    }
                    else if ( connector == "left" ) {
                        if ( action == "expand" )
                            connector5.connect();
                        else
                            connector5.disconnect();
                    }
                }
                // switch ( line[ 0 ] ) {
                //     case 'f':
                //         left.moveDown();
                //         break;
                //     case 'g':
                //         left.moveUp();
                //         break;
                //     case 'h':
                //         right.moveDown();
                //         break;
                //     case 'j':
                //         right.moveUp();
                //         break;
                //     case 'e':
                //         connector1.connect();
                //         connector2.connect();
                //         break;
                //     case 'r':
                //         connector1.disconnect();
                //         connector2.disconnect();
                //         break;
                //     case 'u':
                //         connector5.connect();
                //         break;
                //     case 'i':
                //         connector5.disconnect();
                //         break;
                // }
            } );
            while( true ) {
                rofi::fi::delayMs( 1 );
                auto s = client.read();
                std::cout << "Got: " << s << "\n";
                lineReader.push( s );
            }
            client.close();
        } catch( std::runtime_error& e ) {
            std::cout << "E:" << e.what() << "\n";
        }
        std::cout << "End of client connection\n";
    } );
    server.bind( 4444 );
    while ( true ) {
        rofi::fi::delayMs( 500 );
    }
}