#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <system/arrayMap.hpp>
#include <system/linearMap.hpp>
#include <map>
#include <functional>
#include <system/assert.hpp>
#include <cstring>
#include <memory>

#include <usb.h>
#include <stm32.h>

#include <usb.port.hpp>

#include <system/dbg.hpp>

#define Map ArrayMap

class UsbDevice;
class UsbInteface;
class UsbEndpoint;
class UsbConfiguration;

inline uint8_t descriptorType( const usb_interface_descriptor& ) {
    return USB_DTYPE_INTERFACE;
}

inline uint8_t descriptorType( const usb_config_descriptor& ) {
    return USB_DTYPE_CONFIGURATION;
}

/**
 * Implements a common functionality for descriptors that feature:
 * - one base decriptor
 * - zero or multiple arbitrary descriptors
 * - a list of child descriptors
 * It also provides setup and teardown functions propagated to all children.
 */
template< typename Self, typename Descriptor, typename Child >
class VariableDescriptorEntity {
public:
    Self& self() {
        return *static_cast< Self * >( this );
    }
    const Self& self() const {
        return *static_cast< Self * >( this );
    }

    void clear() {
        _descriptorData.clear();
        _children.clear();

        Descriptor d{};
        d.bLength = sizeof( Descriptor );
        d.bDescriptorType = descriptorType( d );

        pushDescriptor( d );
    }

    /** Push an arbitrary descriptor **/
    template < typename T >
    Self& pushDescriptor( const T& t ) {
        int newSize = _descriptorSize + sizeof( T );
        _descriptorData.resize( newSize );
        memcpy( _descriptorData.data() + _descriptorSize, &t, sizeof( T ) );
        _descriptorSize = newSize;
        return self();
    }

    std::pair< uint8_t*, int > getDescriptor() {
        _descriptorData.resize( _descriptorSize );
        for ( auto& c : _children ) {
            int offset = _descriptorData.size();
            auto [ childDes, childDesSize ] = c->getDescriptor();
            _descriptorData.resize( _descriptorData.size() + childDesSize );
            memcpy( _descriptorData.data() + offset, childDes, childDesSize );
        }
        self()._updateDescriptor();
        return { _descriptorData.data(), _descriptorData.size() };
    }

    template < typename F >
    Self& onSetup( F f ) {
        _onSetup = f;
        return self();
    }

    template < typename F >
    Self& onTeardown( F f ) {
        _onTeardown = f;
        return self();
    }

    void setup() {
        self()._setupPre();
        for ( auto& child : _children )
            child->setup();
        if ( _onSetup )
            _onSetup( self() );
        self()._setupPost();
    }

    void teardown() {
        self()._teardownPre();
        for ( auto& child : _children )
            child->teardown();
        if ( _onTeardown )
            _onTeardown( self() );
        self()._teardownPost();
    }
protected:
    Descriptor& _baseDescriptor() {
        return *reinterpret_cast< Descriptor* >( _descriptorData.data() );
    }

    std::vector< uint8_t > _descriptorData;
    int _descriptorSize = 0;
    // We wrap the children in unique_ptr to ensure stable addresses
    std::vector< std::unique_ptr< Child > > _children;

    std::function< void( Self& ) > _onSetup;
    std::function< void( Self& ) > _onTeardown;
};

class StringDescriptor {
public:
    StringDescriptor( const char16_t *text, int length ) {
        _data.resize( 2 + 2 * length );
        _size() = 2 + 2 * length;
        _type() = USB_DTYPE_STRING;
        auto source = reinterpret_cast< const uint8_t * >( text );
        std::copy_n( source, 2 * length, _text() );
    }

    template < int N >
    StringDescriptor( const char16_t (&text)[ N ] ): StringDescriptor( text, N ) {}

    StringDescriptor( const std::u16string& text ):
        StringDescriptor( text.data(), text.size() )
    {}

    template < int N >
    bool operator==( const char16_t (&text)[ N ] ) const {
        return _equals( text, N );
    }

    bool operator==( const std::u16string& text ) const {
        return _equals( text.data(), text.size() );
    }

    bool operator==( const StringDescriptor& d ) const {
        return _data == d._data;
    }

    std::pair< uint8_t*, int > getDescriptor() {
        return { _data.data(), _data.size() };
    }

private:
    uint8_t& _size() {
        return _data[ 0 ];
    }

    const uint8_t& _size() const {
        return _data[ 0 ];
    }

    uint8_t& _type() {
        return _data[ 1 ];
    }

    uint8_t _type() const {
        return _data[ 1 ];
    }

    uint8_t* _text() {
        return &_data[ 2 ];
    }

    const uint8_t* _text() const {
        return &_data[ 2 ];
    }

    bool _equals( const char16_t* text, int length ) const {
        if ( length != int( _size() ) )
            return false;
        for ( int i = 0; i != length; i++ ) {
            if ( _text()[ i ] != text[ i ] )
                return false;
        }
        return true;
    }

    std::vector< uint8_t > _data;
};

using LangId = uint16_t;
using UsbString = Map< LangId, std::u16string >;

class UsbEndpoint {
public:
    std::pair< uint8_t*, int > getDescriptor() {
        return { reinterpret_cast< uint8_t* >( &_descriptor ),
                 sizeof( _descriptor ) };
    }

    UsbEndpoint& setAddress( uint8_t address ) {
        _descriptor.bEndpointAddress = address;
        return *this;
    }

    UsbEndpoint& setAttributes( uint8_t attributes ) {
        _descriptor.bmAttributes = attributes;
        return *this;
    }

    UsbEndpoint& setPacketSize( uint16_t size ) {
        _descriptor.wMaxPacketSize = size;
        return *this;
    }

    int getPacketSize() const {
        return _descriptor.wMaxPacketSize;
    }

    UsbEndpoint& setInterval( uint8_t interval ) {
        _descriptor.bInterval = interval;
        return *this;
    }

    UsbEndpoint& setTxChainSize( int size ) {
        assert( size > 0 );
        _txChainSize = size;
        return *this;
    }

    void setup();
    void teardown();

    bool isDevToHost() {
        return ( _descriptor.bEndpointAddress & USB_REQ_DIRECTION ) == USB_REQ_DEVTOHOST;
    }

    auto& parentDevice() {
        return *_parent;
    }

    template < typename F >
    UsbEndpoint& onRx( F f ) {
        _onRx = f;
        return *this;
    }

    template < typename F >
    UsbEndpoint& onTxFinished( F f ) {
        _onTx = f;
        return *this;
    }

    int read( void *buff, int maxLength );

    bool write( memory::Pool::Block b, int size );
    bool write( const void *buff, int length ) {
        assert( length != 0 );
        auto block = memory::Pool::allocate( length );

        assert( block.get() );
        memcpy( block.get(), buff, length );
        return write( std::move( block ), length );
    }

    int writeRaw( const unsigned char *buff, int length );

    void stall();
    void unstall();

private:
     UsbEndpoint( UsbDevice *parent ): _parent( parent ), _txChainSize( 8 ) {
        _descriptor.bLength = sizeof( _descriptor );
        _descriptor.bDescriptorType = USB_DTYPE_ENDPOINT;
        _descriptor.bInterval = 0xFF;
    }

    void _handleTx();
    void _handleRx();

    using EvtHandler = std::function< void( UsbEndpoint& ) >;
    using TxChain = RingBuffer< std::pair< memory::Pool::Block, int > >;

    usb_endpoint_descriptor _descriptor;
    UsbDevice *_parent;
    EvtHandler _onTx, _onRx;
    int _txChainSize;
    TxChain _txChain;

    friend class UsbInterface;
    friend class UsbDevice;
    friend class UsbConfiguration;
};

class UsbInterface:
    public VariableDescriptorEntity<
                UsbInterface, usb_interface_descriptor, UsbEndpoint >
{
public:

    void _updateDescriptor() {
        _baseDescriptor().bInterfaceNumber = _intIdx;
        _baseDescriptor().bNumEndpoints = _children.size();
    }

    UsbEndpoint& pushEndpoint() {
        _children.emplace_back( new UsbEndpoint( _parent ) );
        return *_children.back();
    }

    UsbInterface& setAlternate( uint8_t alternate ) {
        _baseDescriptor().bAlternateSetting = alternate;
        return *this;
    }

    UsbInterface& setClass( uint8_t iClass, uint8_t iSubClass ) {
        _baseDescriptor().bInterfaceClass = iClass;
        _baseDescriptor().bInterfaceSubClass = iSubClass;
        return *this;
    }

    UsbInterface& setProtocol( uint8_t protocol ) {
        _baseDescriptor().bInterfaceProtocol = protocol;
        return *this;
    }

    template < typename F >
    UsbInterface& onControl( F f ) {
        _onControl = f;
        return *this;
    }

    auto& parent() {
        return *_parent;
    }
private:
    UsbInterface( UsbDevice *parent, int intIdx ):
        _parent( parent ), _intIdx( intIdx )
    {
        clear();
    }

    void _setupPre() {}
    void _setupPost() {}
    void _teardownPre() {}
    void _teardownPost() {}

    usbd_respond _handleControl( const usbd_ctlreq& req ) {
        if ( _onControl )
            return _onControl( *this, req );
        return usbd_fail;
    }

    UsbDevice *_parent;
    int _intIdx;

    std::function< usbd_respond( UsbInterface&, const usbd_ctlreq& ) > _onControl;


    friend class UsbConfiguration;
    friend class VariableDescriptorEntity;
};

class UsbConfiguration:
    public VariableDescriptorEntity<
            UsbConfiguration, usb_config_descriptor, UsbInterface > {
public:
    void _updateDescriptor() {
        _baseDescriptor().bConfigurationValue = _cfgIdx;
        _baseDescriptor().wTotalLength = _descriptorData.size();
        _baseDescriptor().bNumInterfaces = _children.size();
    }

    UsbConfiguration& setAttributes( uint8_t attributes ) {
        _baseDescriptor().bmAttributes = attributes;
        return *this;
    }

    UsbConfiguration& setMaxPowerMa( int maxPower ) {
        _baseDescriptor().bMaxPower = USB_CFG_POWER_MA( maxPower );
        return *this;
    }

    UsbInterface& pushInterface() {
        _children.emplace_back( new UsbInterface( _parent, _children.size()) );
        return *_children.back();
    }

    int getIndex() {
        return _cfgIdx;
    }

private:
    UsbConfiguration( UsbDevice *parent, int cfgIdx ):
        _parent( parent ), _cfgIdx( cfgIdx )
    {
        clear();
    }

    void _setupPre() {}
    void _setupPost() {}
    void _teardownPre() {}
    void _teardownPost() {}

    usbd_respond _handleControl( const usbd_ctlreq& req ) {
        if ( ( USB_REQ_RECIPIENT & req.bmRequestType ) == USB_REQ_INTERFACE ) {
            if ( req.wIndex <= _children.size() ) {
                return _children[ req.wIndex ]->_handleControl( req );
            }
        }
        return usbd_fail;
    }

    UsbDevice *_parent;
    int _cfgIdx;

    friend class UsbDevice;
    friend class VariableDescriptorEntity;
};

class UsbDevice: public detail::UsbDevice< UsbDevice > {
public:
    void enable() {
        _setupGpio();
        usbd_init( &_device, &_getDriver(), _deviceDescriptor.bMaxPacketSize0,
                   _buffer, sizeof( _buffer ) );

        _currentConfiguration = -1;

        usbd_reg_config( &_device, _setConfTrampoline );
        usbd_reg_control( &_device, _controlTrampoline );
        usbd_reg_descr( &_device, _descriptorTrampoline );
        for ( int evt = 0; evt != usbd_evt_count; evt++ )
            usbd_reg_event( &_device, evt, _eventTrampoline );

        for ( int i = 1; i != 8; i++ ) { // We ommit EP0 (control endpoint)
            usbd_reg_endpoint( &_device, i, _endpointEventTrampoline );
        }

        _enableInterrupt();

        usbd_enable( &_device, true );
        usbd_connect( &_device, true );
    }

    void disable() {
        usbd_connect( &_device, false );
        usbd_enable( &_device, false );

        _disableInterrupt();

        usbd_reg_config( &_device, nullptr );
        usbd_reg_control( &_device, nullptr );
        usbd_reg_descr( &_device, nullptr );
        for ( int evt = 0; evt != usbd_evt_count; evt++ )
            usbd_reg_event( &_device, evt, nullptr );
    }

    static UsbDevice& instance() {
        static UsbDevice device;
        return device;
    }

    UsbDevice& setIrqPriority( int priority ) {
        _setIrqPriority( priority );
        return *this;
    }

    UsbDevice& setControlpacketSize( uint8_t size ) {
        _deviceDescriptor.bMaxPacketSize0 = size;
        return *this;
    }

    UsbDevice& setClass( uint8_t deviceClass, uint8_t deviceSubclass ) {
        _deviceDescriptor.bDeviceClass = deviceClass;
        _deviceDescriptor.bDeviceSubClass = deviceSubclass;
        _deviceDescriptor.bcdUSB = VERSION_BCD( 2,0,0 );
        return *this;
    }

    UsbDevice& setProtocol( uint8_t protocol ) {
        _deviceDescriptor.bDeviceProtocol = protocol;
        return *this;
    }

    UsbDevice& setVidPid( uint16_t vid, uint16_t pid ) {
        _deviceDescriptor.idVendor = vid;
        _deviceDescriptor.idProduct = pid;
        return *this;
    }

    UsbDevice& setVersion( int major, int minor, int patch ) {
        _deviceDescriptor.bcdDevice = VERSION_BCD( major, minor, patch );
        return *this;
    }

    UsbDevice& setManufacturer( const UsbString& string ) {
        _deviceDescriptor.iManufacturer = addString( string );
        return *this;
    }

    UsbDevice& setProduct( const UsbString& string ) {
        _deviceDescriptor.iProduct = addString( string );
        return *this;
    }

    UsbDevice& setSerialNumber( const UsbString& string ) {
        _deviceDescriptor.iSerialNumber = addString( string );
        return *this;
    }

    UsbConfiguration& pushConfiguration() {
        _configurations.emplace_back( new UsbConfiguration( this, _configurations.size() + 1 ) );
        return *_configurations.back();
    }

    UsbConfiguration& getConfiguration( int idx ) {
        return *_configurations[ idx - 1];
    }

    void clearConfigurations() {
        _configurations.clear();
    }

    std::pair< uint8_t*, int > getDescriptor() {
        _deviceDescriptor.bNumConfigurations = _configurations.size();
        return { reinterpret_cast< uint8_t* >( &_deviceDescriptor ),
                 sizeof( _deviceDescriptor ) };
    }

    uint16_t addString( const Map< LangId, StringDescriptor >& s ) {
        auto it = std::find( _stringDescs.begin(), _stringDescs.end(), s );
        if ( it != _stringDescs.end() ) {
            return it - _stringDescs.begin() + 1;
        }
        _stringDescs.push_back( s );
        return _stringDescs.size();
    }

    uint16_t addString( const UsbString& string ) {
        Map< LangId, StringDescriptor > s;
        for ( const auto& [ langId, sd ] : string )
            s.emplace( langId, StringDescriptor( sd ) );
        return addString( s );
    }

    bool isConfigured() const {
        return _device.status.device_state == usbd_state_configured;
    }

    auto& nativeDevice() {
        return _device;
    }
private:
    UsbDevice() {
        _deviceDescriptor.bLength = sizeof( _deviceDescriptor );
        _deviceDescriptor.bDescriptorType = USB_DTYPE_DEVICE;
        _deviceDescriptor.bMaxPacketSize0 = 8;
        clearConfigurations();
    }

    static usbd_respond _setConfTrampoline( usbd_device *, uint8_t cfg ) {
        return instance()._setConf( cfg );
    }

    static usbd_respond _controlTrampoline( usbd_device *, usbd_ctlreq *req,
                                            usbd_rqc_callback *callback )
    {
        return instance()._onControlRequest( req, callback );
    }

    static usbd_respond _descriptorTrampoline( usbd_ctlreq *req, void **address,
                                               uint16_t *length )
    {
        return instance()._getDescriptor( req, address, length );
    }

    static void _endpointEventTrampoline( usbd_device *, uint8_t event,
                                           uint8_t ep )
    {
        instance()._handleEndpointEvent( event, ep );
    }

    static void _eventTrampoline( usbd_device *, uint8_t event, uint8_t ep ) {
        instance()._handleEvent( event, ep );
    }

    usbd_respond _setConf( uint8_t requestedConfiguration ) {
        if ( requestedConfiguration > _configurations.size() )
            return usbd_fail; // Such configuration does not exists

        if ( _currentConfiguration > 0 )
            _configurations[ _currentConfiguration - 1 ]->teardown();
        _configurations[ requestedConfiguration - 1 ]->setup();

        _currentConfiguration = requestedConfiguration;
        return usbd_ack;
    }

    usbd_respond _onControlRequest( usbd_ctlreq *req, usbd_rqc_callback */*callback*/ ) {
        if ( ( USB_REQ_RECIPIENT & req->bmRequestType ) == USB_REQ_INTERFACE ) {
            return _configurations[ _currentConfiguration - 1 ]->_handleControl( *req );
        }
        // TBA other type of events?
        return usbd_fail;
    }

    usbd_respond _getDescriptor( usbd_ctlreq *req, void **address, uint16_t *length ) {
        const uint8_t dType = req->wValue >> 8;
        const uint8_t dNumber = req->wValue & 0xFF;
        switch (dType) {
        case USB_DTYPE_DEVICE: {
                std::tie( *address, *length ) = getDescriptor();
                return usbd_ack;
            }
        case USB_DTYPE_CONFIGURATION: {
                if ( dNumber >= _configurations.size() )
                    return usbd_fail;
                auto [ desc, size ] = _configurations[ dNumber ]->getDescriptor();
                *address = desc;
                *length = size;
                return usbd_ack;
            }
        case USB_DTYPE_STRING: {
                if ( dNumber == 0 ) {
                    std::tie( *address, *length ) = _getLangCapabilityDescriptor();
                    return usbd_ack;
                }
                if ( dNumber <= _stringDescs.size() ) {
                    auto langs = _stringDescs[ dNumber - 1 ];
                    auto desc = langs.find( req->wIndex );
                    if ( desc == langs.end() )
                        return usbd_fail;
                    std::tie( *address, *length ) = desc->second.getDescriptor();
                    return usbd_ack;
                }
                return usbd_fail;
            }
        }
        return usbd_fail;
    }

    void _handleEndpointEvent( uint8_t event, uint8_t ep ) {
        if ( event == usbd_evt_eptx ) {
            assert( ( ep & USB_REQ_DIRECTION ) == USB_REQ_DEVTOHOST );
            ep &= 0x07; // Reset direction flag
            if ( ep == 0 || ep >= _devToHostEndp.size() )
                return;
            auto *epStr = _devToHostEndp[ ep ];
            if ( epStr )
                epStr->_handleTx();
        }
        else if ( event == usbd_evt_eprx ) {
            assert( ( ep & USB_REQ_DIRECTION ) == USB_REQ_HOSTTODEV );
            if ( ep == 0 || ep >= _hostToDevEndp.size() )
                return;
            auto *epStr = _hostToDevEndp[ ep ];
            if ( epStr )
                epStr->_handleRx();
        }
    }

    void _handleEvent( uint8_t /*event*/, uint8_t /*ep*/ ) {
        // TBA
    }

    std::vector< LangId > _collectLangIds() const {
        std::vector< LangId > ids;
        for ( const auto& item : _stringDescs ) {
            for ( const auto& [ langId, d ] : item ) {
                // Quadratic solution leads to a smaller code (and often)
                // speed-comparable solution compared to a set.
                if ( indexOf( langId, ids ) == -1 )
                    ids.push_back( langId );
            }
        }
        return ids;
    }

    std::pair< uint8_t *, int > _getLangCapabilityDescriptor() {
        auto ids = _collectLangIds();
        _langIdsDesc.resize( 2 + 2 * ids.size() );
        _langIdsDesc[ 0 ] = 2 + 2 * ids.size();
        _langIdsDesc[ 1 ] = USB_DTYPE_STRING;
        for ( size_t i = 0; i != ids.size(); i++ ) {
            memcpy( _langIdsDesc.data() + 2 + 2 * i, &ids[ i ], sizeof( LangId ) );
        }
        return { _langIdsDesc.data(), _langIdsDesc.size() };
    }

    friend class UsbEndpoint;
    friend class UsbInterface;
    friend class UsbConfiguration;
    friend class detail::UsbDevice< UsbDevice >;

    usbd_device _device;
    uint32_t _buffer[ 32 ];

    usb_device_descriptor _deviceDescriptor = {};

    std::vector< std::unique_ptr< UsbConfiguration > > _configurations;
    int _currentConfiguration;

    std::vector< Map< LangId, StringDescriptor > > _stringDescs;
    std::vector< uint8_t > _langIdsDesc;

    // We keep a list of active endpoints here in order to properly implement
    // endpoint events.
    std::array< UsbEndpoint *, 7 > _hostToDevEndp = {};
    std::array< UsbEndpoint *, 7 > _devToHostEndp = {};
};
