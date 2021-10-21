#include <drivers/usb.hpp>

void UsbEndpoint::setup() {
    Dbg::error( "Configuring endpoint %02x", _descriptor.bEndpointAddress );
    usbd_ep_config( &_parent->_device, _descriptor.bEndpointAddress,
        _descriptor.bmAttributes, _descriptor.wMaxPacketSize );
    if ( isDevToHost() )
        _parent->_devToHostEndp[ _descriptor.bEndpointAddress & 0x7 ] = this;
    else
        _parent->_hostToDevEndp[ _descriptor.bEndpointAddress & 0x7 ] = this;
}

void UsbEndpoint::teardown() {
    Dbg::error( "Tearing down endpoint %02x", _descriptor.bEndpointAddress );
    usbd_ep_deconfig( &_parent->_device, _descriptor.bEndpointAddress );
    if ( isDevToHost() )
        _parent->_devToHostEndp[ _descriptor.bEndpointAddress & 0x7 ] = nullptr;
    else
        _parent->_hostToDevEndp[ _descriptor.bEndpointAddress & 0x7 ] = nullptr;
}

int UsbEndpoint::read( void *buff, int maxLength ) {
    assert( !isDevToHost() );
    return usbd_ep_read( &_parent->_device, _descriptor.bEndpointAddress,
        buff, maxLength );
}

int UsbEndpoint::write( void *buff, int length ) {
    assert( isDevToHost() );
    return usbd_ep_write( &_parent->_device, _descriptor.bEndpointAddress,
        buff, length );
}

void UsbEndpoint::stall() {
    usbd_ep_stall( &_parent->_device, _descriptor.bEndpointAddress );
}

void UsbEndpoint::unstall() {
    usbd_ep_unstall( &_parent->_device, _descriptor.bEndpointAddress );
}
