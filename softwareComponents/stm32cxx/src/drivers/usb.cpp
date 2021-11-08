#include <drivers/usb.hpp>

void UsbEndpoint::setup() {
    usbd_ep_config( &_parent->_device, _descriptor.bEndpointAddress,
        _descriptor.bmAttributes, _descriptor.wMaxPacketSize );
    if ( isDevToHost() )
        _parent->_devToHostEndp[ _descriptor.bEndpointAddress & 0x7 ] = this;
    else
        _parent->_hostToDevEndp[ _descriptor.bEndpointAddress & 0x7 ] = this;
}

void UsbEndpoint::teardown() {
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

int UsbEndpoint::write( const void *buff, int length ) {
    Dbg::error("Address: %d", int(_descriptor.bEndpointAddress) );
    assert( isDevToHost() );
    // We can use const cast, as the driver does not modify the buffer
    return usbd_ep_write( &_parent->_device, _descriptor.bEndpointAddress,
        const_cast< void * >( buff ), length );
}

void UsbEndpoint::stall() {
    usbd_ep_stall( &_parent->_device, _descriptor.bEndpointAddress );
}

void UsbEndpoint::unstall() {
    usbd_ep_unstall( &_parent->_device, _descriptor.bEndpointAddress );
}
