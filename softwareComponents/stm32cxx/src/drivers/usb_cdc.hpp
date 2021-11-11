#pragma once

#include <drivers/usb.hpp>
#include <drivers/uart.hpp>
#include <usb_cdc.h>

template < typename AllocatorT >
class UsbCdcInterface {
public:
    using Allocator = AllocatorT;
    using Mem = typename Allocator::Block;

    struct Setup {
        uint8_t mgmtEp, txEp, rxEp;
        uint8_t packetSize = 64;
    };

    UsbCdcInterface( UsbConfiguration& cfg, Setup s )
        : _packetSize( s.packetSize )
    {
        cfg.pushDescriptor( usb_iad_descriptor{
            .bLength = sizeof(struct usb_iad_descriptor),
            .bDescriptorType        = USB_DTYPE_INTERFASEASSOC,
            .bFirstInterface        = 0,
            .bInterfaceCount        = 2,
            .bFunctionClass         = USB_CLASS_CDC,
            .bFunctionSubClass      = USB_CDC_SUBCLASS_ACM,
            .bFunctionProtocol      = USB_PROTO_NONE,
            .iFunction              = NO_DESCRIPTOR,
        } );

        auto& mgmtInterface = cfg.pushInterface()
            .setAlternate( 0 )
            .setClass( USB_CLASS_CDC, USB_CDC_SUBCLASS_ACM )
            .setProtocol( USB_PROTO_NONE )
            .pushDescriptor( usb_cdc_header_desc{
                .bFunctionLength        = sizeof(struct usb_cdc_header_desc),
                .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
                .bDescriptorSubType     = USB_DTYPE_CDC_HEADER,
                .bcdCDC                 = VERSION_BCD(1,1,0),
            } )
            .pushDescriptor( usb_cdc_call_mgmt_desc{
                .bFunctionLength        = sizeof(struct usb_cdc_call_mgmt_desc),
                .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
                .bDescriptorSubType     = USB_DTYPE_CDC_CALL_MANAGEMENT,
                .bmCapabilities         = 0,
                .bDataInterface         = 1,
            } )
            .pushDescriptor( usb_cdc_acm_desc{
                .bFunctionLength        = sizeof(struct usb_cdc_acm_desc),
                .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
                .bDescriptorSubType     = USB_DTYPE_CDC_ACM,
                .bmCapabilities         = 0,
            } )
            .pushDescriptor( usb_cdc_union_desc{
                .bFunctionLength        = sizeof(struct usb_cdc_union_desc),
                .bDescriptorType        = USB_DTYPE_CS_INTERFACE,
                .bDescriptorSubType     = USB_DTYPE_CDC_UNION,
                .bMasterInterface0      = 0,
                .bSlaveInterface0       = 1,
            } )
            .onControl([&]( UsbInterface& interf, const usbd_ctlreq& req ) {
                switch (req.bRequest) {
                case USB_CDC_SET_CONTROL_LINE_STATE: {
                    bool succ = false;
                    if ( _handleControlLineState )
                        succ = _handleControlLineState(
                            req.wValue & 0x1, // DTR
                            req.wValue & 0x2, // RTS
                            _lineState & 0x1, // prev DTR
                            _lineState & 0x2  // prev RTS
                        );
                    _lineState = req.wValue;
                    return succ ? usbd_ack : usbd_fail;
                }
                case USB_CDC_SET_LINE_CODING: {
                    bool succ = false;
                    usb_cdc_line_coding coding;
                    memcpy( &coding, req.data, sizeof( coding ) );
                    if ( _handleLineCodingChange )
                        succ = _handleLineCodingChange( coding, _lineCoding );
                    if ( succ )
                        _lineCoding = coding;
                    return succ ? usbd_ack : usbd_fail;
                }
                case USB_CDC_GET_LINE_CODING: {
                    interf.parent().nativeDevice().status.data_ptr = &_lineCoding;
                    interf.parent().nativeDevice().status.data_count = sizeof( _lineCoding );
                    return usbd_ack;
                }
                default:
                    return usbd_fail;
                }
            });

        _mgmtEp = &mgmtInterface.pushEndpoint()
            .setAddress( 0x82 )
            .setAttributes( USB_EPTYPE_INTERRUPT )
            .setPacketSize( 8 )
            .setInterval( 0xFF );

        auto& commInterface = cfg.pushInterface()
            .setAlternate( 0 )
            .setClass( USB_CLASS_CDC_DATA, USB_SUBCLASS_NONE )
            .setProtocol( USB_PROTO_NONE );

        _rxEp = &commInterface.pushEndpoint()
            .setAddress( s.rxEp & ( ~0x80 ) )
            .setAttributes( USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF )
            .setPacketSize( _packetSize )
            .setInterval( 1 )
            .onRx( [&]( UsbEndpoint& ) {
                if ( _handleRx )
                    _handleRx();
            });
        _txEp = &commInterface.pushEndpoint()
            .setAddress( s.txEp | 0x80 )
            .setAttributes( USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF )
            .setPacketSize( _packetSize )
            .setInterval( 1 )
            .onTxFinished( [&]( UsbEndpoint& ) {
                if ( _handleTxFinished )
                    _handleTxFinished();
            } );
    }

    bool isConfigured() {
        return _txEp->parentDevice().isConfigured();
    }

    void send( const char *str ) {
        return send( reinterpret_cast< const uint8_t * >( str ), strlen( str ) );
    }

    void send( const uint8_t *buff, int size ) {
        if ( isConfigured() )
            _txEp->write( buff, size );
        else
            Dbg::error("Unconfigured");
    }

    void send( memory::Pool::Block b, int size ) {
        if ( isConfigured() )
            _txEp->write( std::move( b ), size );
    }

    int read( uint8_t * buff, int maxSize ) {
        return _rxEp->read( buff, maxSize );
    }

    std::pair< memory::Pool::Block, int > read() {
        auto mem = Allocator::allocate( _packetSize );
        int read = _rxEp->read( mem.get(), _packetSize );
        return {std::move( mem ), read };
    }

    template < typename F >
    UsbCdcInterface& onRx( F f ) {
        _handleRx = f;
        return *this;
    }

    template < typename F >
    UsbCdcInterface& onTxFinished( F f ) {
        _handleTxFinished = f;
        return *this;
    }

    template < typename F >
    UsbCdcInterface& onControlLineChange( F f ) {
        _handleControlLineState = f;
        return *this;
    }

    template < typename F >
    UsbCdcInterface& onCodingChange( F f ) {
        _handleLineCodingChange = f;
        return *this;
    }

private:
    int _packetSize;
    usb_cdc_line_coding _lineCoding;
    uint16_t _lineState;
    UsbEndpoint *_mgmtEp;
    UsbEndpoint *_rxEp;
    UsbEndpoint *_txEp;

    std::function< void() > _handleTxFinished;
    std::function< void() > _handleRx;
    /** The function takes 4 bool values:
     *  - DTR
     *  - RTS
     *  - previous DRT
     *  - previous RTS
     */
    std::function< bool( bool, bool, bool, bool ) > _handleControlLineState;

    /** The function takes a new and old coding **/
    std::function< bool( usb_cdc_line_coding, usb_cdc_line_coding ) > _handleLineCodingChange;
};

inline std::optional< StopBits > getStopBits( const usb_cdc_line_coding& c ) {
    switch( c.bCharFormat ) {
        case USB_CDC_1_STOP_BITS:   return StopBits::N1;
        case USB_CDC_1_5_STOP_BITS: return StopBits::N1_5;
        case USB_CDC_2_STOP_BITS:   return StopBits::N2;
    }
    return {};
}

inline std::optional< Parity > getParity( const usb_cdc_line_coding& c ) {
    switch( c.bParityType ) {
        case USB_CDC_NO_PARITY:   return Parity::None;
        case USB_CDC_ODD_PARITY:  return Parity::Odd;
        case USB_CDC_EVEN_PARITY: return Parity::Even;
    }
    return {};
}
