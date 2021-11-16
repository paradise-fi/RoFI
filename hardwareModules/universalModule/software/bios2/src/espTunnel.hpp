#pragma once

#include <drivers/usb_cdc.hpp>
#include <system/ringBuffer.hpp>
#include "board.hpp"

struct EspTunnelManager {
    static auto& instance() {
        static EspTunnelManager tunnel;
        return tunnel;
    }

    UsbConfiguration& setupUsbConfiguration();
    UsbCdcInterface< PrimaryAllocator >& setupTunnel();

    // See https://github.com/espressif/esptool/wiki/ESP32-Boot-Mode-Selection
    void enableEsp( bool v = true );
    void disableEsp();
    void enableBootloader( bool v = true );
    void disableBootloader();

    auto& getCdcInterface() {
        return *_usbCdc;
    }
private:
    EspTunnelManager() = default;

    bool handleCodingChange( usb_cdc_line_coding current, usb_cdc_line_coding old );
    void handleUsbRx();
    void sendPacket();
    bool handleControlLines( bool dtr, bool rts, bool oldDtr, bool oldRts );
    void startUartReading();

    using DataPacket = std::pair< PrimaryAllocator::Block, int >;

    Uart _uart{ ESP_TUNNEL_UART,
        RxOn( ESP_TUNNEL_RX_PIN ),
        TxOn( ESP_TUNNEL_TX_PIN ),
        Baudrate( 115200 ) };
    UartReader< PrimaryAllocator > _uartReader{
        _uart, ESP_TUNNEL_RX_DMA_ALLOCATE };
    UartWriter< PrimaryAllocator > _uartWriter{
        _uart, ESP_TUNNEL_TX_DMA_ALLOCATE };
    RingBuffer< DataPacket, PrimaryAllocator > _toSend { 64 };
    volatile bool _busyUartTx = false;
    volatile bool _pendingUsbRx = false;

    UsbConfiguration *_usbConfiguration;
    std::unique_ptr< UsbCdcInterface< PrimaryAllocator > > _usbCdc;
};

