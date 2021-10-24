#pragma once

#include <drivers/usb.hpp>
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
                    _lineState = req.wValue;
                    if ( _handleControlLineState )
                        _handleControlLineState( _lineState );
                    return usbd_ack;
                }
                case USB_CDC_SET_LINE_CODING: {
                    memcpy( &_lineCoding, req.data, sizeof( _lineCoding ) );
                    if ( _handleLineCodingChange )
                        _handleLineCodingChange( _lineCoding );
                    return usbd_ack;
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

        auto& mgmtEp = mgmtInterface.pushEndpoint()
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
            .onRx( [&]( UsbEndpoint& ep ) {
                auto mem = Allocator::allocate( _packetSize );
                int read = ep.read( mem.get(), _packetSize );
                if ( read > 0 && _handleRx )
                    _handleRx( std::move( mem ), read );
            });
        _txEp = &commInterface.pushEndpoint()
            .setAddress( s.rxEp | 0x80 )
            .setAttributes( USB_EPTYPE_BULK | USB_EPTYPE_DBLBUF )
            .setPacketSize( _packetSize )
            .setInterval( 1 )
            .onTxFinished( [&]( UsbEndpoint& ep ) {
                if ( _handleTxFinished )
                    _handleTxFinished();
            } );
    }

    void send( const char *str ) {
        _txEp->write( str, strlen( str ) );
    }

    void send( const uint8_t *buff, int size ) {
        _txEp->write( buff, size );
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
    UsbEndpoint *_rxEp;
    UsbEndpoint *_txEp;

    std::function< void() > _handleTxFinished;
    std::function< void( Mem, int ) > _handleRx;
    std::function< void( uint16_t ) > _handleControlLineState;
    std::function< void( usb_cdc_line_coding ) > _handleLineCodingChange;
};