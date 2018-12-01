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
    static PBuf allocate( int size ) { return PBuf( size ); }
    static PBuf reference( pbuf* b ) { return PBuf( b, true ); }
    static PBuf own( pbuf* b ) { return PBuf( b, false ); }
    static PBuf empty() { return PBuf( nullptr, false ); }

    PBuf( const PBuf& o ): _buff( o._buff ) { pbuf_ref( _buff ); };
    PBuf& operator=( PBuf o ) { swap( o );  return *this; }
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

    pbuf* get() { return _buff; }

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

        ChunkIt operator++( int ) {
            ChunkIt i = *this;
            ++(*this);
            return i;
        }
        ChunkIt& operator++() {
            // See loop end condition: https://www.nongnu.org/lwip/2_0_x/group__pbuf.html
            if ( _c._buf->tot_len == _c._buf->len )
                _c._buf = nullptr;
            else
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
    PBuf( pbuf* buff, bool addReference ) : _buff( buff ) {
        if ( addReference )
            pbuf_ref( _buff );
    }
    PBuf( int size ) : _buff( pbuf_alloc( PBUF_RAW, size, PBUF_POOL ) ) {
        if ( !_buff )
            throw std::bad_alloc();
    }

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
template < typename FType >
class SequentialExecutor {
public:
    SequentialExecutor( int queueSize ):
        _jobs( queueSize )
    {
        auto res = xTaskCreate( _run, "SequentialExecutor", 2048, this, 5, nullptr );
        if ( res != pdPASS )
            throw std::runtime_error( "Cannot allocate task" );
    }

    template < typename F >
    void run( const F& f ) {
        _jobs.emplace( f );
    }

    template < typename F >
    void run( F&& f ) {
        _jobs.emplace( std::move( f ) );
    }

private:
    static void _run( void* arg ) {
        auto* self = reinterpret_cast< SequentialExecutor* >( arg );
        while ( true ) {
            auto job = self->_jobs.pop();
            job();
        }
    }

    rtos::Queue< std::function< FType > > _jobs;
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
        _cs( cs ), _versionSem( 1 ), _isrSem( 2 ), _statusSem( 2 ),
        _sendSem( 2 ), _recvSem( 10 )
    {
        spi_device_interface_config_t devCfg = SpiDeviceInterface()
            .clockSpeedHz( 50000 )
            .mode( 0 )
            .queueSize( 1 );
        spi_bus_add_device( bus, &devCfg, &_dev );

        gpio_config_t cfg = defaultCsConfig( _cs );
        gpio_config( &cfg );
        gpio_set_level( _cs, 1 );
        gpio_isr_handler_add( _cs, csInterruptHandler, this );
        checkStatus();
        isrService();
    }

    void sendBlob( uint16_t contentType, PBuf&& blob ) {
        _sendSem.take();
        runTransaction( [ blob = std::move( blob ), this, contentType ]() mutable {
            auto semGuard = _sendSem.giveGuard();

            if ( blob.size() > 2048 ) {
                std::cout << "Trying to send large packet: " << blob.size() << "\n";
                return;
            }

            SpiTransactionGuard transaction( _cs );

            uint8_t header[ 1 ];
            as< Command >( header ) = Command::Send;
            spiWrite( _dev, header );

            slaveDelay();

            uint8_t blobHeader[ 4 ];
            as< uint16_t >( blobHeader ) = contentType;
            as< uint16_t >( blobHeader + 2 ) = blob.size();
            spiWrite( _dev, blobHeader );

            for ( auto it = blob.chunksBegin(); it != blob.chunksEnd(); ++it ) {
                spiWrite( _dev, it->mem(), it->size() );
            }
            transaction.end();
            slaveDelay();
        } );
    }

    void version() {
        _sendSem.take();
        runTransaction( [this]{
            auto semGuard = _sendSem.giveGuard();
            SpiTransactionGuard transaction( _cs );

            uint8_t header[ 1 ];
            as< Command >( header + 0 ) = Command::Version;
            spiWrite( _dev, header );

            slaveDelay();

            uint8_t payload[ 4 ];
            spiRead( _dev, payload );
            transaction.end();
            slaveDelay();

            if ( _onVersion )
                _onVersion( *this, DockVersion::from( payload ) );
        } );
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
        vTaskDelay( 1 / portTICK_PERIOD_MS / 2 );
    }

    static IRAM_ATTR rtos::IsrDeferrer& isrService() {
        static rtos::IsrDeferrer _def( 10 );
        return _def;
    }

    SequentialExecutor< void() >& getExecutor() {
        static SequentialExecutor< void() > _executor( 30 );
        return _executor;
    }

    template < typename F >
    void runTransaction( F&& f ) {
        getExecutor().run( std::forward< F >( f ) );
    }

    void receiveBlob() {
        if ( !_recvSem.tryTake() ) {
            return;
        }
        runTransaction( [this] {
            auto semGuard = _recvSem.giveGuard();
            SpiTransactionGuard transaction( _cs );

            uint8_t header[ 1 ];
            as< Command >( header ) = Command::Receive;
            spiWrite( _dev, header );

            slaveDelay();

            uint8_t payloadHeader[ 4 ];
            spiRead( _dev, payloadHeader );

            auto contentType = as< uint16_t >( payloadHeader );
            auto size = as< uint16_t >( payloadHeader + 2 );
            if ( size == 0 || size > 2048 ) {
                return;
            }

            auto payload = PBuf::allocate( size );
            for ( auto it = payload.chunksBegin(); it != payload.chunksEnd(); ++it ) {
                spiRead( _dev, it->mem(), it->size() );
            }
            transaction.end();
            slaveDelay();

            if ( _onReceive )
                _onReceive( *this, contentType, std::move( payload ) );
        } );
    }

    void checkStatus() {
        if ( !_statusSem.tryTake() )
            return;
        runTransaction([this] {
            auto semGuard = _statusSem.giveGuard();;
            SpiTransactionGuard transaction( _cs );

            uint8_t header[ 5 ];
            as< Command >( header + 0 ) = Command::Status;
            as< uint16_t >( header + 1 ) = 0;
            as< uint16_t >( header + 3 ) = 0;
            spiWrite( _dev, header );

            slaveDelay();

            uint8_t payload[ 12 ];
            spiRead( _dev, payload );

            transaction.end();
            slaveDelay();

            auto status = DockStatus::from( payload );
            for ( int i = 0; i != status.pendingReceive; i++ )
                receiveBlob();
            if ( _onStatus )
                _onStatus( *this, status );
        } );
    }

    void checkInterrupt() {
        if ( !_isrSem.tryTake() ) {
            return;
        }
        runTransaction( [this] {
            auto semGuard = _isrSem.giveGuard();
            SpiTransactionGuard transaction( _cs );

            uint8_t header[ 3 ];
            as< Command >( header + 0 ) = Command::Interrupt;
            as< uint16_t >( header + 1 ) =
                InterruptFlag::CONNECT |
                InterruptFlag::BLOB    ;
            spiWrite( _dev, header );

            slaveDelay();

            uint8_t interrupts[ 2 ];
            spiRead( _dev, interrupts );
            transaction.end();
            slaveDelay();

            uint16_t intFlags = as< uint16_t >( interrupts );
            if ( intFlags & InterruptFlag::BLOB ) {
                checkStatus();
            }
            if ( _onInterrupt )
                _onInterrupt( *this, intFlags );
        } );
    }

    static void IRAM_ATTR csInterruptHandler( void* arg ) {
        isrService().isr( arg, []( void* arg ) {
            auto* self = reinterpret_cast< Dock* >( arg );
            self->receiveBlob(); // It is probably receive transaction, check it
            self->checkInterrupt();
        } );
    }

    static Gpio defaultCsConfig( gpio_num_t c ) {
        return Gpio()
            .pinBitMask( 1 << c )
            .mode( GPIO_MODE_INPUT_OUTPUT_OD )
            .pullUpEn( GPIO_PULLUP_ENABLE )
            .pullDownEn( GPIO_PULLDOWN_DISABLE )
            .intrType( GPIO_INTR_NEGEDGE );
    }

    struct SpiTransactionGuard {
        SpiTransactionGuard( gpio_num_t p ): _pin( p ), _rel( false ) {
            esp_log_level_set( "gpio", ESP_LOG_ERROR ); // Silence messages

            gpio_config_t cfg = defaultCsConfig( _pin )
                .intrType( GPIO_INTR_DISABLE );
            gpio_config(&cfg);
            gpio_set_level( _pin, 0 );
        }
        ~SpiTransactionGuard() {
            if ( !_rel )
                end();
        }

        void end() {
            gpio_config_t cfg = defaultCsConfig( _pin );
            gpio_config(&cfg);
            gpio_set_level( _pin, 1 );

            esp_log_level_set( "gpio", ESP_LOG_INFO ); // return messages messages
            _rel = true;
        }

        SpiTransactionGuard( const SpiTransactionGuard& ) = delete;
        SpiTransactionGuard& operator=( const SpiTransactionGuard& ) = delete;

        gpio_num_t _pin;
        bool _rel;
    };

    gpio_num_t _cs;
    spi_device_handle_t _dev;

    rtos::Semaphore _versionSem;
    rtos::Semaphore _isrSem;
    rtos::Semaphore _statusSem;
    rtos::Semaphore _sendSem;
    rtos::Semaphore _recvSem;

    std::function< void( Dock&, DockVersion ) > _onVersion;
    std::function< void( Dock&, DockStatus ) > _onStatus;
    std::function< void( Dock&, uint16_t ) > _onInterrupt;
    std::function< void( Dock&, int, PBuf&& ) > _onReceive;
};

} // namespace _rofi