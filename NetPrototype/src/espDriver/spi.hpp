#pragma once

#include <iostream>
#include <driver/spi_master.h>
#include <cstring>

class SpiBusConfig {
public:
    SpiBusConfig()
        : _c( { -1, -1, -1, -1, -1, 0, 0 } )
    {}

    SpiBusConfig& mosi( int pin ) {  _c.mosi_io_num = pin; return *this; }
    SpiBusConfig& miso( int pin ) { _c.miso_io_num = pin; return *this; }
    SpiBusConfig& sclk( int pin ) { _c.sclk_io_num = pin; return *this; }
    SpiBusConfig& quadWp( int pin ) { _c.quadwp_io_num = pin; return *this; }
    SpiBusConfig& quadHd( int pin ) { _c.quadhd_io_num = pin; return *this; }
    SpiBusConfig& maxTransSize( int size ) { _c.max_transfer_sz = size; return *this; }
    SpiBusConfig& flags( uint32_t flags ) { _c.flags = flags; return *this; }
    operator spi_bus_config_t() const { return _c; }
private:
    spi_bus_config_t _c;
};

std::ostream& operator<<( std::ostream& o, spi_bus_config_t s );

class SpiDeviceInterface {
public:
    SpiDeviceInterface(): _s() {}

    operator spi_device_interface_config_t() const { return _s; }
    SpiDeviceInterface& commandBits( uint8_t p ) { _s.command_bits = p; return *this; }
    SpiDeviceInterface& addressBits( uint8_t p ) { _s.address_bits = p; return *this; }
    SpiDeviceInterface& dummyBits( uint8_t p ) { _s.dummy_bits = p; return *this; }
    SpiDeviceInterface& mode( uint8_t p ) { _s.mode = p; return *this; }
    SpiDeviceInterface& dutyCyclePos( uint8_t p ) { _s.duty_cycle_pos = p; return *this; }
    SpiDeviceInterface& csEnaPretrans( uint8_t p ) { _s.cs_ena_pretrans = p; return *this; }
    SpiDeviceInterface& csEnaPosttrans( uint8_t p ) { _s.cs_ena_posttrans = p; return *this; }
    SpiDeviceInterface& clockSpeedHz( int p ) { _s.clock_speed_hz = p; return *this; }
    SpiDeviceInterface& spicsIoNum( int p ) { _s.spics_io_num = p; return *this; }
    SpiDeviceInterface& flags( uint32_t p ) { _s.flags = p; return *this; }
    SpiDeviceInterface& queueSize( int p ) { _s.queue_size = p; return *this; }
    SpiDeviceInterface& preCb( transaction_cb_t p ) { _s.pre_cb = p; return *this; }
    SpiDeviceInterface& postCb( transaction_cb_t p ) { _s.post_cb = p; return *this; }
private:
    spi_device_interface_config_t _s;
};

std::ostream& operator<<( std::ostream& o, spi_device_interface_config_t s );

static void _spiTrans( spi_device_handle_t dev, uint8_t* what, int count ) {
    spi_transaction_t t{};
    t.tx_buffer = t.rx_buffer = what;
    t.length = 8 * count;
    spi_device_transmit( dev, &t );
}

static void spiRead( spi_device_handle_t dev, uint8_t* where, int count ) {
    _spiTrans( dev, where, count );
}

template < size_t size >
void spiRead( spi_device_handle_t dev, uint8_t ( &where )[ size ] ) {
    _spiTrans( dev, where, size );
}

static void spiWrite( spi_device_handle_t dev, uint8_t* what, int count ) {
    _spiTrans( dev, what, count );
}

template < size_t size >
void spiWrite( spi_device_handle_t dev, uint8_t ( &what )[ size ] ) {
    _spiTrans( dev, what, size );
}