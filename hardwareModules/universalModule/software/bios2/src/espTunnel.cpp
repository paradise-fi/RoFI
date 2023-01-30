#include "espTunnel.hpp"

#include <stm32cxx.config.hpp>
#include <system/idle.hpp>
#include <drivers/hal.hpp>

bool EspTunnelManager::handleCodingChange( usb_cdc_line_coding current,
                                  usb_cdc_line_coding old )
{
    if ( old.dwDTERate != current.dwDTERate ) {
        if ( current.dwDTERate > 4500000u ) {
            return false;
        }
        _uart.setBaudrate( current.dwDTERate );
    }

    if ( current.bDataBits != 8 ) {
        return false;
    }

    if ( getStopBits( old ) != getStopBits( current ) ) {
        auto stopBits = getStopBits( current );
        if ( stopBits.has_value() )
            _uart.setStopBits( *stopBits );
        else
            return false;
    }

    if ( getParity( old ) != getParity( current ) ) {
        auto parity = getParity( current );
        if ( parity.has_value() )
            _uart.setParity( *parity );
        else
            return false;
    }
    return true;
}

void EspTunnelManager::handleUsbRx() {
    if ( _toSend.available() < 1 ) {
        _pendingUsbRx = true;
        return;
    }
    auto [ m, size ] = _usbCdc->read();
    bool res = _toSend.push_back( std::make_pair( std::move( m ), size ) );
    assert( res );
    if ( !_busyUartTx ) {
        sendPacket();
    }
}

void EspTunnelManager::sendPacket() {
    assert( !_toSend.empty() );
    _busyUartTx = true;
    auto [ data, size ] = _toSend.pop_front();
    _uartWriter.writeBlock( std::move( data ), 0, size,
        [ size, this ]( PrimaryAllocator::Block, int written ) {
            auto& self = *this;
            if ( written != size )
                Dbg::error( "Cannot write: %d, %d", written, size );
            assert( written == size );
            if ( !_toSend.empty() )
                self.sendPacket();
            else
                self._busyUartTx = false;
        } );

    if ( _pendingUsbRx ) {
        _pendingUsbRx = false;
        handleUsbRx();
    }
}

bool EspTunnelManager::handleControlLines( bool dtr, bool rts,
                                           bool /*oldDtr*/, bool oldRts )
{
    if ( oldRts && !rts) { // A reset...
        disableEsp();
        enableBootloader( dtr ); // ...possibly into a bootloader
    }
    IdleTask::schedule( 5, [&]{ enableEsp(); } );
    return true;
}

void EspTunnelManager::startUartReading() {
    auto block = PrimaryAllocator::allocate( ESP_TUNNEL_PACKET_SIZE );
    _uartReader.startCircularReading( 2 * ESP_TUNNEL_PACKET_SIZE, 8,
        [ this ]( const uint8_t* data, int size ) {
            if ( size == 0 )
                return;
            _usbCdc->send( data, size );
        } );
}

UsbConfiguration& EspTunnelManager::setupUsbConfiguration() {
    auto& usb = UsbDevice::instance()
        .setIrqPriority( 0 )
        .setClass( USB_CLASS_IAD, USB_SUBCLASS_IAD )
        .setProtocol( USB_PROTO_IAD )
        .setVidPid( USB_VID, USB_PID )
        .setVersion( 1, 0, 0 )
        .setManufacturer( {
            { USB_LANGID_ENG_US, USB_MFR_STR }
        } )
        .setProduct( {
            { USB_LANGID_ENG_US, USB_PROD_STR }
        } )
        .setSerialNumber( {
            { USB_LANGID_ENG_US, USB_SER_STR }
        } );

    auto& cfg = usb.pushConfiguration()
        .setAttributes( USB_CFG_ATTR_RESERVED | USB_CFG_ATTR_SELFPOWERED )
        .setMaxPowerMa( 100 );
    _usbConfiguration = &cfg;
    return cfg;
}

UsbCdcInterface< PrimaryAllocator >& EspTunnelManager::setupTunnel() {
    using Cdc = typename decltype( _usbCdc )::element_type;

    _usbCdc.reset( new Cdc( *_usbConfiguration, {
        .mgmtEp = ESP_TUNNEL_MGMT_EP,
        .txEp = ESP_TUNNEL_TX_EP,
        .rxEp = ESP_TUNNEL_RX_EP,
        .packetSize = ESP_TUNNEL_PACKET_SIZE
    } ) );
    _usbCdc->onCodingChange( [this]( auto current, auto old ) {
            return handleCodingChange( current, old );
        } )
        .onRx( [this]() {
            return handleUsbRx();
        } )
        .onControlLineChange( [&]( bool rts, bool dtr, bool oldRts, bool oldDtr ) {
            return handleControlLines( rts, dtr, oldRts, oldDtr );
        } );

    _uart.setIrqPriority( STM32CXX_IRQ_HIGH_PRIORITY + 2 );
    _uart.enable();
    startUartReading();

    ESP_TUNNEL_EN_PIN.setupPPOutput();
    ESP_TUNNEL_BOOT_PIN.setupPPOutput();

    enableBootloader( false );
    enableEsp();


    return *_usbCdc;
}

void EspTunnelManager::enableEsp( bool v ) {
    ESP_TUNNEL_EN_PIN.write( v );
}

void EspTunnelManager::disableEsp() {
    enableEsp( false );
}

void EspTunnelManager::enableBootloader( bool v ) {
    ESP_TUNNEL_BOOT_PIN.write( !v );
}

void EspTunnelManager::disableBootloader() {
    enableBootloader( false );
}

