#pragma once

#include <stdexcept>
#include <type_traits>
#include <functional>
#include <vector>

#include <driver/gpio.h>
#include <lwip/pbuf.h>
#include <esp_log.h>

#include "espDriver/spi.hpp"
#include "espDriver/gpio.hpp"
#include "freeRTOS.hpp"
#include "utility.hpp"

namespace _rofi {

class PBuf {
public:
    PBuf( pbuf* buff = nullptr ) : _buff( buff ) {}
    PBuf( int size ) : _buff( pbuf_alloc( PBUF_RAW, size, PBUF_POOL ) ) {
        if ( !_buff )
            throw std::bad_alloc();
    }

    PBuf( const PBuf& ) = delete;
    PBuf& operator=( const PBuf& ) = delete;

    PBuf( PBuf&& o ): _buff( o._buff ) { o._buff = nullptr; }
    PBuf& operator=( PBuf&& o ) { swap( o ); return *this; }

    ~PBuf() { if ( _buff ) pbuf_free( _buff ); }

    void swap( PBuf& o ) {
        using std::swap;
        swap( _buff, o._buff );
    }

    pbuf* release() {
        auto b = _buff;
        _buff = nullptr;
        return b;
    }

    operator bool() const { return _buff; }
    bool simple() const { return _buff->tot_len == _buff->len; }
    void* payload() { return _buff->payload; }
    int size() { return _buff->tot_len; }

    uint8_t& operator[]( int idx ) { return get( *this, idx ); }
    const uint8_t& operator[]( int idx ) const { return get( *this, idx ); }

    struct Chunk {
        uint8_t* mem(){ return reinterpret_cast< uint8_t * >( _buf->payload ); };
        int size() { return _buf->len; }
        const pbuf* _buf;
    };

    class ChunkIt {
    public:
        using value_type = Chunk;
        using reference = Chunk&;
        using pointer = Chunk*;
        typedef int difference_type;
        typedef std::forward_iterator_tag iterator_category;

        ChunkIt( pbuf* buf = nullptr ) : _c{ buf } { }

        ChunkIt operator++() {
            ChunkIt i = *this;
            _c._buf = _c._buf->next;
            return i;
        }
        ChunkIt& operator++( int ) {
            _c._buf = _c._buf->next;
            return *this;
        }
        reference operator*() { return _c; }
        pointer operator->() { return &_c; }
        bool operator==( const ChunkIt& o ) const { return _c._buf == o._c._buf; }
        bool operator!=( const ChunkIt& o ) const { return !( *this == o ); }
    private:
        Chunk _c;
    };

    ChunkIt chunksBegin() { return ChunkIt( _buff ); }
    ChunkIt chunksEnd() { return ChunkIt(); }
private:
    template< typename Self,
              typename R = typename std::conditional<
                std::is_const< Self >::value, const uint8_t , uint8_t >::type >
    static R& get( Self& s, int idx ) {
        auto* chunk = s._buff;
        while( idx > chunk->len ) {
            idx -= chunk->len;
            chunk = chunk->next;
        }
        return static_cast< R* >( chunk->payload )[ idx ];
    }

    pbuf* _buff;
};

/*
 * Sequential executor cannot take std::function as it cannot be constructed
 * in the interrupt context due to heap allocation.
 */
template < typename Arg >
class SequentialExecutor {
public:
    using F = void (*)(Arg&);

    SequentialExecutor( int queueSize ):
        _jobs( queueSize )
    {
        auto res = xTaskCreate( _run, "SequentialExecutor", 2048, this, 10, nullptr );
        if ( res != pdPASS )
            throw std::runtime_error( "Cannot allocate task" );
    }

    void run( F f, const Arg& arg, rtos::ExContext c = rtos::ExContext::Normal ) {
        _jobs.push( { f, arg }, c );
    }

    void run( F f, Arg&& arg, rtos::ExContext c = rtos::ExContext::Normal ) {
        _jobs.push( { f, std::move( arg ) }, c );
    }

private:
    static void _run( void* arg ) {
        auto* self = reinterpret_cast< SequentialExecutor* >( arg );
        while ( true ) {
            auto task = self->_jobs.pop();
            ( *task.first )( task.second );
        }
    }

    rtos::Queue< std::pair< F, Arg > > _jobs;
};

struct DockVersion {
    int variant;
    int protocolRevision;

    static DockVersion from( const uint8_t* d ) {
        return { as< uint16_t >( d ), as< uint16_t >( d + 2 ) };
    }
};

struct DockStatus {
    uint16_t flags;
    int pendingSend;
    int pendingReceive;
    float intVoltage, intCurrect, extVoltage, extCurrent;

    static DockStatus from( const uint8_t* d ) {
        DockStatus s;
        s.flags = as< uint16_t >( d );
        s.pendingSend = as< uint8_t >( d + 2 );
        s.pendingReceive = as< uint8_t >( d + 3 );
        s.intVoltage = as< int16_t >( d + 4 ) / 255.0;
        s.intCurrect = as< int16_t >( d + 6 ) / 255.0;
        s.extVoltage = as< int16_t >( d + 8 ) / 255.0;
        s.extCurrent = as< int16_t >( d + 10 ) / 255.0;
        return s;
    }
};

class Dock {
public:
    Dock( spi_host_device_t bus, gpio_num_t cs ):
        _cs( cs )
    {
        spi_device_interface_config_t devCfg = SpiDeviceInterface()
            .clockSpeedHz( 100000 )
            .mode( 0 )
            .queueSize( 1 );
        spi_bus_add_device( bus, &devCfg, &_dev );

        gpio_config_t cfg = defaultCsConfig( _cs );
        gpio_config( &cfg );
        gpio_set_level( _cs, 1 );
        gpio_isr_handler_add( _cs, onInterrupt, this );
    }

    void sendBlob( uint16_t contentType, PBuf&& blob ) {
        runTransaction( sendBlobTran, contentType, std::move( blob ) );
    }

    void version() {
        runTransaction( versionTran );
    }

    template < typename F >
    void onVersion( F f ) { _onVersion = f; }

    template < typename F >
    void onStatus( F f ) { _onStatus = f; }

    template < typename F >
    void onInterrupt( F f ) { _onInterrupt = f; }

    template < typename F >
    void onReceive( F f ) { _onReceive = f; }
private:
    struct TransactionContext {
        Dock* self;
        int contentType;
        PBuf mem;
    };

    enum class Command: uint8_t {
        Version = 0,
        Status = 1,
        Interrupt = 2,
        Send = 3,
        Receive = 4
    };

    enum InterruptFlag {
        CONNECT = 1 << 0,
        BLOB = 1 << 1
    };

    static void slaveDelay() {
        vTaskDelay( 50 / portTICK_PERIOD_MS );
    }

    static SequentialExecutor< TransactionContext >& getExecutor() {
        static SequentialExecutor< TransactionContext > _executor( 10 );
        return _executor;
    }

    void runTransaction( void (*f)( TransactionContext& ), int contentType,
        PBuf&& mem, rtos::ExContext c = rtos::ExContext::Normal )
    {
        getExecutor().run( f, { this, contentType, std::move( mem ) }, c );
    }

    void runTransaction( void (*f)( TransactionContext& ),
        rtos::ExContext c = rtos::ExContext::Normal )
    {
        getExecutor().run( f, { this, 0, {} }, c );
    }

    static void sendBlobTran( TransactionContext& ctx ) {
        auto self = ctx.self;
        SpiTransactionGuard _( self->_cs );

        uint8_t header[ 1 ];
        as< Command >( header ) = Command::Send;
        spiWrite( self->_dev, header );

        slaveDelay();

        uint8_t blobHeader[ 4 ];
        as< uint16_t >( blobHeader ) = ctx.contentType;
        as< uint16_t >( blobHeader + 2 ) = ctx.mem.size();
        spiWrite( self->_dev, blobHeader );

        for ( auto it = ctx.mem.chunksBegin(); it != ctx.mem.chunksEnd(); ++it ) {
            spiWrite( self->_dev, it->mem(), it->size() );
        }
    }

    static void receiveBlobTran( TransactionContext& ctx ) {
        auto self = ctx.self;
        SpiTransactionGuard _( self->_cs );

        uint8_t header[ 1 ];
        as< Command >( header ) = Command::Receive;
        spiWrite( self->_dev, header );

        slaveDelay();

        uint8_t payloadHeader[ 4 ];
        spiRead( self->_dev, payloadHeader );

        auto contentType = as< uint16_t >( payloadHeader );
        auto size = as< uint16_t >( payloadHeader + 2 );
        if ( size == 0 || size > 2048 )
            return;

        PBuf payload( size );
        for ( auto it = payload.chunksBegin(); it != payload.chunksEnd(); ++it ) {
            spiRead( self->_dev, it->mem(), it->size() );
        }

        if ( self->_onReceive )
            self->_onReceive( *self, contentType, std::move( payload ) );
    }

    static void checkStatusTran( TransactionContext& ctx ) {
        auto self = ctx.self;
        SpiTransactionGuard _( self->_cs );

        uint8_t header[ 5 ];
        as< Command >( header + 0 ) = Command::Status;
        as< uint16_t >( header + 1 ) = 0;
        as< uint16_t >( header + 3 ) = 0;
        spiWrite( self->_dev, header );

        slaveDelay();

        uint8_t payload[ 12 ];
        spiRead( self->_dev, payload );

        auto status = DockStatus::from( payload );
        for ( int i = 0; i != status.pendingReceive; i++ )
            self->runTransaction( receiveBlobTran );
        if ( self->_onStatus )
            self->_onStatus( *self, status );
    }

    static void checkInterruptTran( TransactionContext& ctx ) {
        auto self = ctx.self;
        SpiTransactionGuard _( self->_cs );

        uint8_t header[ 3 ];
        as< Command >( header + 0 ) = Command::Interrupt;
        as< uint16_t >( header + 1 ) =
            InterruptFlag::CONNECT |
            InterruptFlag::BLOB    ;
        spiWrite( self->_dev, header );

        slaveDelay();

        uint8_t interrupts[ 2 ];
        spiRead( self->_dev, interrupts );

        uint16_t intFlags = as< uint16_t >( interrupts );
        if ( intFlags & InterruptFlag::BLOB )
            self->runTransaction( checkStatusTran );
        if ( self->_onInterrupt )
            self->_onInterrupt( *self, intFlags );
    }

    static void versionTran( TransactionContext& ctx ) {
        auto self = ctx.self;
        SpiTransactionGuard _( self->_cs );

        uint8_t header[ 1 ];
        as< Command >( header + 0 ) = Command::Version;
        spiWrite( self->_dev, header );

        slaveDelay();

        uint8_t payload[ 4 ];
        spiRead( self->_dev, payload );
        if ( self->_onVersion )
            self->_onVersion( *self, DockVersion::from( payload ) );
    }

    static void IRAM_ATTR onInterrupt( void* arg ) {
        auto* self = reinterpret_cast< Dock* >( arg );
        self->runTransaction( checkInterruptTran, rtos::ExContext::ISR );
    }

    static Gpio defaultCsConfig( gpio_num_t c ) {
        return Gpio()
            .pinBitMask( 1 << c )
            .mode( GPIO_MODE_INPUT_OUTPUT_OD )
            .pullUpEn( GPIO_PULLUP_ENABLE )
            .pullDownEn( GPIO_PULLDOWN_DISABLE )
            .intrType( GPIO_INTR_POSEDGE );
    }

    struct SpiTransactionGuard {
        SpiTransactionGuard( gpio_num_t p ): _pin( p ) {
            esp_log_level_set( "gpio", ESP_LOG_ERROR ); // Silence messages

            gpio_config_t cfg = defaultCsConfig( _pin )
                .intrType( GPIO_INTR_DISABLE );
            gpio_config(&cfg);
            gpio_set_level( _pin, 0 );
        }
        ~SpiTransactionGuard() {
            gpio_set_level( _pin, 1 );
            gpio_config_t cfg = defaultCsConfig( _pin );
            gpio_config(&cfg);
            gpio_set_level( _pin, 1 );

            esp_log_level_set( "gpio", ESP_LOG_INFO ); // return messages messages
        }

        SpiTransactionGuard( const SpiTransactionGuard& ) = delete;
        SpiTransactionGuard& operator=( const SpiTransactionGuard& ) = delete;

        gpio_num_t _pin;
    };

    gpio_num_t _cs;
    spi_device_handle_t _dev;
    std::function< void( Dock&, DockVersion ) > _onVersion;
    std::function< void( Dock&, DockStatus ) > _onStatus;
    std::function< void( Dock&, uint16_t ) > _onInterrupt;
    std::function< void( Dock&, int, PBuf&& ) > _onReceive;
};

} // namespace _rofi