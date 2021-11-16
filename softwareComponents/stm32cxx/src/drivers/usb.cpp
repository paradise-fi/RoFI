#include <drivers/usb.hpp>
#include <system/irq.hpp>

void UsbEndpoint::setup() {
    bool res = usbd_ep_config( &_parent->_device, _descriptor.bEndpointAddress,
        _descriptor.bmAttributes, _descriptor.wMaxPacketSize );
    assert( res );
    if ( isDevToHost() ) {
        _parent->_devToHostEndp[ _descriptor.bEndpointAddress & 0x7 ] = this;
        _txChain = TxChain( _txChainSize );
    }
    else
        _parent->_hostToDevEndp[ _descriptor.bEndpointAddress & 0x7 ] = this;
}

void UsbEndpoint::teardown() {
    usbd_ep_deconfig( &_parent->_device, _descriptor.bEndpointAddress );
    if ( isDevToHost() )
        _parent->_devToHostEndp[ _descriptor.bEndpointAddress & 0x7 ] = nullptr;
    else
        _parent->_hostToDevEndp[ _descriptor.bEndpointAddress & 0x7 ] = nullptr;
    _txChain = {};
}

int UsbEndpoint::read( void *buff, int maxLength ) {
    assert( !isDevToHost() );
    return usbd_ep_read( &_parent->_device, _descriptor.bEndpointAddress,
        buff, maxLength );
}

void UsbEndpoint::write( memory::Pool::Block block, int length ) {
    assert( length <= _descriptor.wMaxPacketSize );
    assert( isDevToHost() );
    IrqMask guard;
    if ( _txChain.size() == 0 ) {
        // Try send a packet
        int res = usbd_ep_write(
            &_parent->_device,
            _descriptor.bEndpointAddress,
            const_cast< unsigned char * >( block.get() ),
            length );
        if ( res >= 0 ) {
            assert( res == length );
            return; // Success, we can return
        }
    }
    // There is a TX in progress
    _txChain.push_back_force( { std::move( block ), length } );
}

void UsbEndpoint::stall() {
    usbd_ep_stall( &_parent->_device, _descriptor.bEndpointAddress );
}

void UsbEndpoint::unstall() {
    usbd_ep_unstall( &_parent->_device, _descriptor.bEndpointAddress );
}

void UsbEndpoint::_handleTx() {
    if ( !_txChain.empty() ) {
        IrqMask guard;
        auto [ block, length ] = _txChain.pop_front();
        guard.give();
        int res = usbd_ep_write(
            &_parent->_device,
            _descriptor.bEndpointAddress,
            const_cast< unsigned char * >( block.get() ),
            length );
        assert( res >= 0 );
        assert( res == length );
    }
    if ( _onTx )
        _onTx( *this );
}

void UsbEndpoint::_handleRx() {
    if ( _onRx )
        _onRx( *this );
}

