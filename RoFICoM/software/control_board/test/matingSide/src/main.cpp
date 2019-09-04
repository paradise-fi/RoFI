#include <iostream>
#include <iomanip>
#include <vector>
#include <driver/spi_master.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <cstring>
#include "util.hpp"

class SpiDevice {
public:
    SpiDevice( spi_device_handle_t spi, gpio_num_t pin ): _handle( spi ), _pin( pin ) {
        esp_log_level_set( "gpio", ESP_LOG_ERROR ); // Silence messages
        _disable();
    }

    struct Guard {
        Guard ( SpiDevice *dev = nullptr ): _dev( dev ) {
            if ( _dev )
                _dev->_enable();
        }

        ~Guard() {
            if ( _dev )
                _dev->_disable();
        }

        Guard( const Guard& ) = delete;
        Guard& operator=( const Guard& ) = delete;
        Guard( Guard&& other ): _dev( nullptr ) {
            std::swap( _dev, other._dev );
        }
        Guard& operator=( Guard&& other ) {
            std::swap( _dev, other._dev );
            return *this;
        }
    private:
        SpiDevice* _dev;

        friend class SpiDevice;
    };

    spi_device_handle_t handle() {
        return _handle;
    }

    Guard enable() {
        return Guard( this );
    }

private:
    void _enable() {
        gpio_config_t cfg{};
        cfg.pin_bit_mask = 1 << _pin;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.mode = GPIO_MODE_OUTPUT;
        esp_err_t ret = gpio_config( &cfg );
        ESP_ERROR_CHECK( ret );
        gpio_set_level( _pin, 0 );
    }

    void _disable() {
        gpio_set_level( _pin, 1 );
        // gpio_config_t cfg{};
        // cfg.pin_bit_mask = 1 << _pin;
        // cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        // cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        // cfg.mode = GPIO_MODE_INPUT;
        // esp_err_t ret = gpio_config( &cfg );
        // ESP_ERROR_CHECK( ret );
    }

    spi_device_handle_t _handle;
    gpio_num_t _pin;
};

void send( spi_device_handle_t dev, const uint8_t *data, int len ) {
    spi_transaction_t t{};
    t.length = len * 8;
    t.tx_buffer = data;
    esp_err_t ret = spi_device_transmit( dev, &t );
    ESP_ERROR_CHECK( ret );
}

template < int LEN >
void send( spi_device_handle_t dev, uint8_t (&data)[LEN] ) {
    send( dev, data, LEN );
}

std::vector< uint8_t> read( spi_device_handle_t dev, int len ) {
    spi_transaction_t t{};
    t.rxlength = len * 8;
    std::vector< uint8_t > result( len );
    t.rx_buffer = result.data();
    esp_err_t ret = spi_device_transmit( dev, &t );
    ESP_ERROR_CHECK( ret );
    return result;
}

void devDelay() {
    vTaskDelay(1 / portTICK_RATE_MS );
}

std::pair< int, int > getVersion( SpiDevice& d ) {
    uint8_t cmd[1] = { 0 };
    auto guard = d.enable();
    send( d.handle(), cmd );
    devDelay();
    auto res = read( d.handle(), 4 );
    int variant = viewAs< uint16_t >( res.data() );
    int revision = viewAs< uint16_t >( res.data() + 2 );
    return { variant, revision };
}

struct ConnectorStatus {
    uint16_t flags;
    int pendingSend;
    int pendingReceive;
    float intVoltage, intCurrect, extVoltage, extCurrent;

    static ConnectorStatus from( const uint8_t* d ) {
        ConnectorStatus s;
        s.flags = viewAs< uint16_t >( d );
        s.pendingSend = viewAs< uint8_t >( d + 2 );
        s.pendingReceive = viewAs< uint8_t >( d + 3 );
        s.intVoltage = viewAs< int16_t >( d + 4 ) / 255.0;
        s.intCurrect = viewAs< int16_t >( d + 6 ) / 255.0;
        s.extVoltage = viewAs< int16_t >( d + 8 ) / 255.0;
        s.extCurrent = viewAs< int16_t >( d + 10 ) / 255.0;
        return s;
    }
};

ConnectorStatus getStatus( SpiDevice& d, uint16_t flags, uint16_t mask ) {
    uint8_t cmd[ 5 ];
    cmd[ 0 ] = 1;
    viewAs< uint16_t >( cmd + 1 ) = flags;
    viewAs< uint16_t >( cmd + 3 ) = mask;
    auto guard = d.enable();
    send( d.handle(), cmd );
    devDelay();
    auto res = read( d.handle(), 12 );
    return ConnectorStatus::from( res.data() );
}

uint16_t getInterrupt( SpiDevice& d ) {
    uint8_t cmd[ 3 ];
    cmd[ 0 ] = 2;
    viewAs< uint16_t >( cmd + 1 ) = 0;
    auto guard = d.enable();
    send( d.handle(), cmd );
    devDelay();
    auto res = read( d.handle(), 2 );
    return viewAs< uint16_t >( res.data() );
}

void sendBlob( SpiDevice& d, uint16_t contentType,
    const std::vector< uint8_t >& blob )
{
    uint8_t cmd[ 1 ] = { 3 };
    auto guard = d.enable();
    send( d.handle(), cmd );
    devDelay();
    uint8_t header[ 4 ];
    viewAs< uint16_t >( header ) = contentType;
    viewAs< uint16_t >( header + 2 ) = blob.size();
    send( d.handle(), header );
    send( d.handle(), blob.data(), blob.size() );
}

std::pair< uint16_t, std::vector< uint8_t > > receiveBlob( SpiDevice& d ) {
    uint8_t cmd[ 1 ] = { 4 };
    auto guard = d.enable();
    send( d.handle(), cmd );
    devDelay();
    auto header = read( d.handle(), 4 );
    int length = viewAs< uint16_t >( header.data() + 2 );
    if ( length > 1500 )
        return { viewAs< uint16_t >( header.data() ), {} };
    auto blob = read( d.handle(), length );
    return { viewAs< uint16_t >( header.data() ), blob };
}

void hexDump( const char *buff, int count ) {
    std::cout << std::hex << "[ ";
    for ( int i = 0; i != count; i++ ) {
        std::cout << std::setw( 2 ) << std::setfill('0') << int(buff[ i ]) << " ";
    }
    std::cout << std::dec << "]\n";
}

std::vector< uint8_t > blob( const std::string& s ) {
    std::vector< uint8_t > res;
    std::copy( s.begin(), s.end(), std::back_inserter( res ) );
    return res;
}

std::string text( const std::vector< uint8_t >& s ) {
    std::string res;
    std::copy( s.begin(), s.end(), std::back_inserter( res ) );
    return res;
}

extern "C" void app_main() {
    spi_device_handle_t spi;
    spi_bus_config_t buscfg { -1, -1, -1, -1, -1, 0, 0, 0 };
    buscfg.miso_io_num = -1;
    buscfg.mosi_io_num = 14;
    buscfg.sclk_io_num = 13;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 64;
    buscfg.flags = SPICOMMON_BUSFLAG_MASTER;

    spi_device_interface_config_t devcfg{};
    // devcfg.clock_speed_hz = 6 * 1000*1000;
    devcfg.clock_speed_hz = 1000*1000;
    devcfg.mode = 0;
    devcfg.queue_size = 2;
    devcfg.flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE;
    // devcfg.cs_ena_pretrans = 16;

    esp_err_t ret = spi_bus_initialize( HSPI_HOST, &buscfg, 0 );
    ESP_ERROR_CHECK( ret );
    ret = spi_bus_add_device( HSPI_HOST, &devcfg, &spi );
    ESP_ERROR_CHECK( ret );

    SpiDevice devA( spi, GPIO_NUM_12 );
    SpiDevice devB( spi, GPIO_NUM_26 );

    std::cout << "Start\n";
    while ( true ) {
        auto version = getVersion( devA );
        std::cout << "Variant A " << version.first << ", revision " << version.second << "\n";
        vTaskDelay(1 / portTICK_RATE_MS);
        version = getVersion( devB );
        std::cout << "Variant B " << version.first << ", revision " << version.second << "\n";
        vTaskDelay(1000 / portTICK_RATE_MS);

        uint16_t interrupt = getInterrupt( devA );
        std::cout << "Interrupt: " << interrupt << "\n";
        // interrupt = getInterrupt( devB );
        // std::cout << "Interrupt: " << interrupt << "\n";
        vTaskDelay(1000 / portTICK_RATE_MS);

        sendBlob( devB, 0, blob("1BCDEFGHIJKLM") );
        vTaskDelay(100 / portTICK_RATE_MS);
        sendBlob( devB, 0, blob("2BCDEFGHIJKLM") );
        vTaskDelay(100 / portTICK_RATE_MS);
        sendBlob( devB, 0, blob("3BCDEFGHIJKLM") );
        vTaskDelay(100 / portTICK_RATE_MS);

        auto status = getStatus( devA, 0, 0 );
        std::cout << "Pending: " << status.pendingSend << ", available: " << status.pendingReceive << "\n";
        vTaskDelay(10 / portTICK_RATE_MS);

        if ( status.pendingReceive > 30 )
            status.pendingReceive = 0;
        for ( int i = 0; i != status.pendingReceive; i++ ) {
            auto recv = receiveBlob( devA );
            std::cout << "Received (" << recv.first << "): " << text(recv.second) << "\n";
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
};