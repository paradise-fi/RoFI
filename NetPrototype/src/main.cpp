#include <iostream>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_log.h>

#include <freertos/task.h>
#include <vector>
#include <tuple>

#include "dock.hpp"
#include "freeRTOS.hpp"
#include "roif.hpp"

#include <lwip/sockets.h>
#include <tcpip_adapter.h>

extern "C" void app_main() {
    tcpip_adapter_init();
    gpio_install_isr_service( 0 );

    spi_bus_config_t spiBus = SpiBusConfig()
        .mosi( 5 )
        .miso( 18 )
        .sclk( 19 );
    auto r = spi_bus_initialize( HSPI_HOST, &spiBus, 1 );
    ESP_ERROR_CHECK( r );

    _rofi::RoIF roif( "192.168.1.1", "255.255.255.0", "192.168.1.5", { GPIO_NUM_12, GPIO_NUM_14 } );
    roif.setUp();
    std::cout << "roif initialized\n";

    char rx_buffer[128];
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    struct sockaddr_in destAddr;
    destAddr.sin_addr.s_addr = inet_addr( "192.168.1.1" );
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons( 4242 );
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
    int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
    if (sock < 0) {
        std::cout << "Cannot create socket: " << strerror( errno ) << "\n";
        while( true );
    }
    std::cout << "Socket created\n";

    const char* payload = "Hello world!";

    while ( true ) {
        int err = sendto(sock, payload, strlen(payload), 0, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err < 0) {
            std::cout << "Error occured during sending: " << errno << ": " << strerror( errno ) << "\n";
            break;
        }
        std::cout << "Message sent\n";
        vTaskDelay( 2000 / portTICK_PERIOD_MS );
    }



    _rofi::Dock d1( HSPI_HOST, GPIO_NUM_14 );
    _rofi::Dock d2( HSPI_HOST, GPIO_NUM_12 );

    d2.onReceive([]( _rofi::Dock&, int, _rofi::PBuf&& buf ) {
        std::cout << "Received: ";
        for ( int i = 0; i != buf.size(); i++ ) {
            std::cout << buf[ i ];
        }
        std::cout << "\n";
    } );

    int counter = 0;
    while( true ) {
        std::cout << "Sending blob\n";

        auto message = _rofi::PBuf( 4 );
        message[ 0 ] = 'A' + ((counter++) % 26);
        message[ 1 ] = 'b';
        message[ 2 ] = 'c';
        message[ 3 ] = 'd';
        d1.sendBlob( 0, std::move( message ) );

        vTaskDelay( 2000 / portTICK_PERIOD_MS );
    }
}