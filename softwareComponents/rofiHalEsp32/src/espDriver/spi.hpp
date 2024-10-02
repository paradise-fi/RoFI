#pragma once

#include <iostream>
#include <sstream>
#include <iomanip>

#include <driver/spi_master.h>
#include <cstring>

namespace rofi::esp32 {

class SpiBus {
public:
    SpiBus(): _s{ -1, -1, -1, -1, -1, 0, 0, 0 } {}
    operator spi_bus_config_t() const { return _s; }
    SpiBus& mosiIoNum( int p ) { _s.mosi_io_num = p; return *this; }
    SpiBus& misoIoNum( int p ) { _s.miso_io_num = p; return *this; }
    SpiBus& sclkIoNum( int p ) { _s.sclk_io_num = p; return *this; }
    SpiBus& quadwpIoNum( int p ) { _s.quadwp_io_num = p; return *this; }
    SpiBus& quadhdIoNum( int p ) { _s.quadhd_io_num = p; return *this; }
    SpiBus& maxTransferSz( int p ) { _s.max_transfer_sz = p; return *this; }
    SpiBus& flags( uint32_t p ) { _s.flags = p; return *this; }
    SpiBus& intrFlags( int p ) { _s.intr_flags = p; return *this; }
private:
    spi_bus_config_t _s;
};

std::ostream& operator<<( std::ostream& o, spi_bus_config_t s );

class SpiDeviceInterface {
public:
    SpiDeviceInterface():
        _s{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 }
    {}
    operator spi_device_interface_config_t() const { return _s; }
    SpiDeviceInterface& commandBits( uint8_t p ) { _s.command_bits = p; return *this; }
    SpiDeviceInterface& addressBits( uint8_t p ) { _s.address_bits = p; return *this; }
    SpiDeviceInterface& dummyBits( uint8_t p ) { _s.dummy_bits = p; return *this; }
    SpiDeviceInterface& mode( uint8_t p ) { _s.mode = p; return *this; }
    SpiDeviceInterface& dutyCyclePos( uint8_t p ) { _s.duty_cycle_pos = p; return *this; }
    SpiDeviceInterface& csEnaPretrans( uint8_t p ) { _s.cs_ena_pretrans = p; return *this; }
    SpiDeviceInterface& csEnaPosttrans( uint8_t p ) { _s.cs_ena_posttrans = p; return *this; }
    SpiDeviceInterface& clockSpeedHz( int p ) { _s.clock_speed_hz = p; return *this; }
    SpiDeviceInterface& inputDelayNs( int p ) { _s.input_delay_ns = p; return *this; }
    SpiDeviceInterface& flags( uint32_t p ) { _s.flags = p; return *this; }
    SpiDeviceInterface& queueSize( int p ) { _s.queue_size = p; return *this; }
    SpiDeviceInterface& preCb( transaction_cb_t p ) { _s.pre_cb = p; return *this; }
    SpiDeviceInterface& postCb( transaction_cb_t p ) { _s.post_cb = p; return *this; }
private:
    spi_device_interface_config_t _s;
};

std::ostream& operator<<( std::ostream& o, spi_device_interface_config_t s );

namespace detail {
    inline void _spiTrans( spi_device_handle_t dev, uint8_t* what, int count ) {
        spi_transaction_t t{};
        t.tx_buffer = t.rx_buffer = what;
        t.length = 8 * count;
        spi_device_transmit( dev, &t );
    }
}

inline std::string hexDump(uint8_t *where, int count ) {
    std::stringstream stream;
    std::string separator;
    for ( int i = 0; i != count; i++ ) {
        stream << separator << "0x"
            << std::setfill ('0') << std::setw(2)
            << std::hex << int(where[i]);
        separator = ", ";
    }
    return stream.str();
}

inline std::string charDump(uint8_t *where, int count ) {
    std::stringstream stream;
    std::string separator;
    for ( int i = 0; i != count; i++ ) {
        stream << where[ i ];
    }
    return stream.str();
}

inline void spiRead( spi_device_handle_t dev, uint8_t* where, int count ) {
    spi_transaction_t t = {};
    t.rx_buffer = where;
    memset( where, 0xAA, count );
    t.length = t.rxlength = 8 * count;
    spi_device_transmit( dev, &t );
}

template < size_t size >
void spiRead( spi_device_handle_t dev, uint8_t ( &where )[ size ] ) {
    spi_transaction_t t = {};
    t.rx_buffer = where;
    memset( where, 0xAA, size );
    t.length = t.rxlength = 8 * size;
    spi_device_transmit( dev, &t );
}

inline void spiWrite( spi_device_handle_t dev, uint8_t* what, int count ) {
    spi_transaction_t t = {};
    t.tx_buffer = what;
    t.length = 8 * count;
    spi_device_transmit( dev, &t );
}

template < size_t size >
void spiWrite( spi_device_handle_t dev, uint8_t ( &what )[ size ] ) {
    spi_transaction_t t = {};
    t.tx_buffer = what;
    t.length = 8 * size;
    spi_device_transmit( dev, &t );
}

} // namespace rofi::esp32
