#include <rofi_hal.hpp>

#include <array>
#include <cassert>
#include <stdexcept>
#include <iostream>
#include <variant>
#include <thread>
#include <atomic>
#include <cmath>
#include <type_traits>

#include <esp_timer.h>

#include <freeRTOS.hpp>

#include <driver/spi_master.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <esp_heap_caps.h>

#include <espDriver/gpio.hpp>
#include <espDriver/spi.hpp>

#include <bsp.hpp>
#include <atoms/util.hpp>
#include <logging.hpp>

namespace {

using namespace rofi::hal;
using namespace std::string_literals;

// Forward declaration
class ConnectorLocal;
class ConnectorBus;

/**
 * \brief Given a GPIO and its default configuration, start and guard SPI
 * transaction.
 *
 * You are expected to hold an instance of this class for the whole transaction.
 * When this objects destructed, the transaction is ended automatically.
 */
class SpiTransactionGuard {
public:
    SpiTransactionGuard( gpio_num_t p, gpio_config_t cfg ):
        _pin( p ), _cfg( cfg ), _rel( false )
    {
        esp_log_level_set( "gpio", ESP_LOG_ERROR ); // Silence messages
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config( &cfg );
        gpio_set_level( _pin, 0 );
    }

    ~SpiTransactionGuard() {
        if ( !_rel )
            end();
    }

    /**
     * \brief End the transaction
     *
     * Return GPIO back to its original state, set it to high.
     */
    void end() {
        gpio_set_level( _pin, 1 );
        gpio_config( &_cfg );

        esp_log_level_set( "gpio", ESP_LOG_INFO ); // return messages messages
        _rel = true;
    }

    SpiTransactionGuard( const SpiTransactionGuard& ) = delete;
    SpiTransactionGuard& operator=( const SpiTransactionGuard& ) = delete;
private:
    gpio_num_t _pin;
    gpio_config_t _cfg;
    bool _rel;
};

/**
 * \brief Connector version specification
 */
struct ConnectorVersion {
    int variant;
    int protocolRevision;

    static ConnectorVersion from( const uint8_t* d ) {
        return { as< uint16_t >( d ), as< uint16_t >( d + 2 ) };
    }
};

enum ConnectorInterruptFlags {
    Connect = 1 << 0,
    NewBlob = 1 << 1
};

enum ConnectorStateFlags {
    PositionExpanded  = 1 << 0,
    InternalConnected = 1 << 1,
    ExternalConnected = 1 << 2,
    LidarDistanceMode = 0b11 << 3,
    MatingSide        = 1 << 8,
    Orientation       = 0b11 << 9,
    LidarStatus       = 0b11 << 11,
};

struct ConnectorStateImpl: public ConnectorState {
    int pendingSend;
    int pendingReceive;

    static ConnectorStateImpl from( const uint8_t* d ) {
        ConnectorStateImpl s;
        uint16_t flags = as< uint16_t >( d );

        if ( flags & ConnectorStateFlags::PositionExpanded )
            s.position = ConnectorPosition::Extended;
        else
            s.position = ConnectorPosition::Retracted;

        s.internal = flags & ConnectorStateFlags::InternalConnected;
        s.external = flags & ConnectorStateFlags::ExternalConnected;
        s.distanceMode = rofi::hal::LidarDistanceMode( flags & ConnectorStateFlags::LidarDistanceMode >> 3 );
        s.connected = flags & ConnectorStateFlags::MatingSide;
        s.orientation = ConnectorOrientation (
            ( flags & ConnectorStateFlags::Orientation ) >> 9 );
        s.lidarStatus = rofi::hal::LidarStatus(
            ( flags & ConnectorStateFlags::LidarStatus ) >> 11
        );

        s.pendingSend = as< uint8_t >( d + 2 );
        s.pendingReceive = as< uint8_t >( d + 3 );
        s.internalVoltage = as< int16_t >( d + 4 ) / 255.0;
        s.internalCurrent = as< int16_t >( d + 6 ) / 255.0;
        s.externalVoltage = as< int16_t >( d + 8 ) / 255.0;
        s.externalCurrent = as< int16_t >( d + 10 ) / 255.0;
        s.distance = as< uint16_t >( d + 12 );

        return s;
    }
};

/**
 * \brief SPI bus driver for RoFICoMs
 *
 * ESP32 native driver has the following limitations:
 * - it supports only up to 6 devices on the bus
 * - it does not support delay & write (necessary for implementing sendPacket)
 *
 * Therefore, we implement Connector bus which provides interface to enqueue a
 * command and executes it. ConnectorBus overcomes the limitation of the native
 * SPI driver.
 *
 * To implement a new command, you have to:
 * - add new `<name>Command` struct in the ConnectorBus
 * - extend the definition of ConnectorBus::Command
 * - override ConnectorBus::run() taking you command struct
 */
class ConnectorBus {
public:
    struct VersionCommand {
        ConnectorLocal *conn;
    };
    struct InterruptCommand {
        ConnectorLocal *conn;
        uint16_t interruptMask;
        int counter;
    };
    struct StatusCommand {
        ConnectorLocal *conn;
        uint16_t flags;
        uint16_t writeMask;
    };
    struct SendCommand {
        ConnectorLocal *conn;
        uint16_t contentType;
        PBuf packet;
    };
    struct ReceiveCommand {
        ConnectorLocal *conn;
    };

    using Command = std::variant<
        VersionCommand, InterruptCommand, StatusCommand, SendCommand, ReceiveCommand >;

    /**
     * \brief Schedule given command for execution
     *
     * The command is enqueued and in finite time will be processed. You can
     * optionally pass execution context (norma/interrupt)
     */
    void schedule( Command cmd, rtos::ExContext c = rtos::ExContext::Normal ) {
        // if ( std::holds_alternative< VersionCommand >( cmd ) )
        //     abort();
        _commandQueue.push( std::move( cmd ), c );
    }

    /**
     * \brief Construct ConnectorBus
     *
     * \param bus SPI device for the bus
     * \param dataPin GPIO for MISO/MOSI data line
     * \param clkPin GPIO for SPI clock
     * \param clkFrequency clock frequency in Hz (up to 26 MHz)
     */
    ConnectorBus( spi_host_device_t bus, gpio_num_t dataPin, gpio_num_t clkPin,
        int clkFrequency)
        : _commandQueue( 64 ),
          _dmaBuffer( static_cast< uint8_t* >( heap_caps_malloc( 14 , MALLOC_CAP_DMA ) ) )
    {
        using namespace rofi::esp32;

        spi_bus_config_t busConfig = SpiBus()
            .mosiIoNum( dataPin )
            .sclkIoNum( clkPin )
            .flags( SPICOMMON_BUSFLAG_MASTER );
        spi_device_interface_config_t devConfig = SpiDeviceInterface() // Virtual device
            .mode( 0 )
            .flags( SPI_DEVICE_3WIRE | SPI_DEVICE_HALFDUPLEX )
            .queueSize( 1 )
            .clockSpeedHz( clkFrequency );
        auto ret = spi_bus_initialize( bus, &busConfig, 1 );
        ESP_ERROR_CHECK( ret );
        ret = spi_bus_add_device( bus, &devConfig, &_spiDev );
        ESP_ERROR_CHECK( ret );
        ret = gpio_install_isr_service(
            ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LOWMED | ESP_INTR_FLAG_IRAM );
        ESP_ERROR_CHECK( ret );

        // We have to use FreeRTOS task as the task needs high priority
        auto tRet = xTaskCreate([]( void *arg ) {
                    ConnectorBus& self = *reinterpret_cast< ConnectorBus * >( arg );
                    self._run();
                },
                "ConectorBusTask",
                4096,   // Stack size
                this,   // Argument
                3,      // Priority
                nullptr // The task is never accessed again
            );
        if ( tRet != pdPASS )
            abort();
    }

    ~ConnectorBus() {
        free( _dmaBuffer );
    }

private:
    void _run() {
        while ( true ) {
            Command cmd = _commandQueue.pop();
            std::visit( [&]( auto cmd ){ run( std::move( cmd ) ); }, cmd );
        }
    }

    void slaveDelay() {
        const uint32_t delayUs = 200;
        uint32_t target = esp_timer_get_time() + delayUs;
        while( esp_timer_get_time() < target ) {
            asm volatile ("nop");
        }
    }

    enum ProtocolCommand {
        Version = 0,
        Status = 1,
        Interrupt = 2,
        Send = 3,
        Receive = 4
    };

    void run( VersionCommand c ); // Defined after ConnectorLocal
    void run( InterruptCommand c ); // Defined after ConnectorLocal
    void run( StatusCommand c ); // Defined after ConnectorLocal
    void run( SendCommand c );  // Defined after ConnectorLocal
    void run( ReceiveCommand c ); // Defined after ConnectorLocal

    spi_device_handle_t _spiDev;
    rtos::Queue< Command > _commandQueue;
    uint8_t *_dmaBuffer; // Avoid repeated allocation of DMA-capable memory for short transactions
};

/**
 * \brief HAL Connector driver implementation for RoFICoM 2.5 on ESP32
 */
class ConnectorLocal:
    public Connector::Implementation,
    public std::enable_shared_from_this< ConnectorLocal >
{
public:
    virtual rofi::hal::ConnectorState getState() override {
        // State is always ready as it is kept up-to-date by interrupts & pollers
        return _status;
    }

    virtual void connect() override {
        _issueStatusCmd( ConnectorStateFlags::PositionExpanded,
            ConnectorStateFlags::PositionExpanded );
    }

    virtual void disconnect() override {
        _issueStatusCmd( 0, ConnectorStateFlags::PositionExpanded );
    }

    virtual void onConnectorEvent(
        std::function< void ( Connector, ConnectorEvent ) > callback )
    {
        _eventCallback = callback;
    }

    virtual void onPacket(
        std::function< void( Connector, uint16_t, PBuf ) > callback ) override
    {
        _packetCallback = callback;
    }

    virtual void send( uint16_t contentType, PBuf packet ) override {
        ConnectorBus::SendCommand c { this, contentType, std::move( packet ) };
        _bus->schedule( std::move( c ) );
    }

    virtual void connectPower( ConnectorLine line ) override {
        if ( line == ConnectorLine::External )
            _issueStatusCmd( ConnectorStateFlags::ExternalConnected, ConnectorStateFlags::ExternalConnected );
        else
            _issueStatusCmd( ConnectorStateFlags::InternalConnected, ConnectorStateFlags::InternalConnected );
    }

    virtual void disconnectPower( ConnectorLine line ) override {
        if ( line == ConnectorLine::External )
            _issueStatusCmd( 0, ConnectorStateFlags::ExternalConnected );
        else
            _issueStatusCmd( 0, ConnectorStateFlags::InternalConnected );
    }

    virtual void setDistanceMode( rofi::hal::LidarDistanceMode mode ) override {
        _issueStatusCmd( static_cast< uint16_t >( mode ) << 3, ConnectorStateFlags::LidarDistanceMode );
    }

    ConnectorLocal( ConnectorBus *bus, gpio_num_t cs )
        : _bus( bus ), _cs( cs ), _receiveCmdCounter( 0 ), _interruptCounter( 0 )
    {
        _setupCs();

        auto tRet = xTaskCreate([]( void *arg ) {
                    ConnectorLocal& self = *reinterpret_cast< ConnectorLocal * >( arg );

                    while (true) {
                        self._issueStatusCmd( 0, 0 );
                        vTaskDelay( 300 / portTICK_PERIOD_MS );
                    }
                },
                "StatusPoller",
                4096,   // Stack size
                this,   // Argument
                tskIDLE_PRIORITY, // Priority
                nullptr // The task is never accessed again
            );
        if ( tRet != pdPASS )
            abort();
    }
private:
    static rofi::esp32::Gpio defaultCsConfig( gpio_num_t c ) {
        return rofi::esp32::Gpio()
            .pinBitMask( 1ull << c )
            .mode( GPIO_MODE_INPUT_OUTPUT_OD )
            .pullUpEn( GPIO_PULLUP_ENABLE )
            .pullDownEn( GPIO_PULLDOWN_DISABLE )
            .intrType( GPIO_INTR_NEGEDGE );
    }

    void _issueStatusCmd( uint16_t flags, uint16_t writeMask ) {
        ConnectorBus::StatusCommand c;
        c.conn = this;
        c.flags = flags;
        c.writeMask = writeMask;
        _bus->schedule( c );
    }

    void _issueInterruptCmd( rtos::ExContext exCtx = rtos::ExContext::Normal,
        uint16_t interruptMask = ConnectorInterruptFlags::Connect | ConnectorInterruptFlags::NewBlob )
    {
        static int counter = 0;
        _interruptCounter++;
        ConnectorBus::InterruptCommand c;
        c.conn = this;
        c.interruptMask = interruptMask;
        c.counter = counter++;
        _bus->schedule( c, exCtx );
    }

    void _issueReceiveCmd( rtos::ExContext exCtx = rtos::ExContext::Normal ) {
        _receiveCmdCounter++;
        ConnectorBus::ReceiveCommand c;
        c.conn = this;
        _bus->schedule( c, exCtx );
    }

    SpiTransactionGuard startTransaction() {
        return SpiTransactionGuard( _cs, defaultCsConfig( _cs ) );
    }

    void finish( ConnectorBus::VersionCommand, ConnectorVersion ver ) {
        // Not implemented yet - currently, there is no need to distinguish
        // versions of the connectors
    }

    void finish( ConnectorBus::InterruptCommand, uint16_t ) {
        // We ignore the interrupt flags and issue status request - the
        // notification will propagate from them. If we find this producing
        // delays, optimize it in the future.
        _interruptCounter--;
        _issueStatusCmd( 0, 0 );
    }

    void finish( ConnectorBus::StatusCommand, ConnectorStateImpl status ) {
        if ( _status.connected != status.connected ) {
            auto event = status.connected ?
                ConnectorEvent::Connected : ConnectorEvent::Disconnected;
            if ( _eventCallback )
                _eventCallback( rofi::hal::Connector( this->shared_from_this() ),
                    event );
        }
        if (status.pendingReceive == 255) {
            rofi::log::debug("invalid pending receive");
            status.pendingReceive = 0;
        }
        while ( _receiveCmdCounter < status.pendingReceive )
            _issueReceiveCmd();
        _status = status;
    }

    void finish( ConnectorBus::SendCommand ) {
        // Nothing to do
    }

    void finish( ConnectorBus::ReceiveCommand ) {
        _receiveCmdCounter--;
    }

    void finish( ConnectorBus::ReceiveCommand, uint16_t contentType, PBuf packet ) {
        _receiveCmdCounter--;
        _packetCallback( rofi::hal::Connector( this->shared_from_this() ),
            contentType, std::move( packet ) );
    }

    void _setupCs() {
        gpio_config_t cfg = defaultCsConfig( _cs );
        auto ret = gpio_config( &cfg );
        ESP_ERROR_CHECK( ret );
        ret = gpio_isr_handler_add( _cs, _csInterruptHandler, this );
        gpio_set_level( _cs, 1 );
        ESP_ERROR_CHECK( ret );
    }

    static void _csInterruptHandler( void * arg ) {
        auto *self = reinterpret_cast< ConnectorLocal* >( arg );
        // Probably a packet, so try to read it, then check the true reason
        self->_issueReceiveCmd( rtos::ExContext::ISR );
        if ( self->_interruptCounter == 0 )
            self->_issueInterruptCmd( rtos::ExContext::ISR );
    }

    friend class ConnectorBus;

    ConnectorBus *_bus;
    gpio_num_t _cs;
    ConnectorStateImpl _status;
    std::function< void ( Connector, ConnectorEvent ) > _eventCallback;
    std::function< void( Connector, uint16_t, PBuf ) > _packetCallback;
    std::atomic< int > _receiveCmdCounter, _interruptCounter;
};

// Implementation of bus commands

void ConnectorBus::run( VersionCommand c ) {
    using namespace rofi::esp32;

    auto transaction = c.conn->startTransaction();
    const int headerSize = 1;
    as< ProtocolCommand >( _dmaBuffer + 0 ) = ProtocolCommand::Version;
    spiWrite( _spiDev, _dmaBuffer, headerSize );

    slaveDelay();
    const int payloadSize = 4;
    spiRead( _spiDev, _dmaBuffer, payloadSize );
    transaction.end();

    c.conn->finish( c, ConnectorVersion::from( _dmaBuffer ) );
}

void ConnectorBus::run( InterruptCommand c ) {
    using namespace rofi::esp32;

    auto transaction = c.conn->startTransaction();
    const int headerSize = 3;
    as< ProtocolCommand >( _dmaBuffer + 0 ) = ProtocolCommand::Interrupt;
    as< uint16_t >( _dmaBuffer + 1 ) = c.interruptMask;
    spiWrite( _spiDev, _dmaBuffer, headerSize );

    slaveDelay();

    const int interruptsSize = 2;
    spiRead( _spiDev, _dmaBuffer, interruptsSize );
    transaction.end();

    c.conn->finish( c, as< uint16_t >( _dmaBuffer ) );
}

void ConnectorBus::run( StatusCommand c ) {
    using namespace rofi::esp32;

    auto transaction = c.conn->startTransaction();
    const int headerSize = 5;
    as< ProtocolCommand >( _dmaBuffer + 0 ) = ProtocolCommand::Status;
    as< uint16_t >( _dmaBuffer + 1 ) = c.flags;
    as< uint16_t >( _dmaBuffer + 3 ) = c.writeMask;
    spiWrite( _spiDev, _dmaBuffer, headerSize );

    slaveDelay();

    const int payloadSize = 14;
    spiRead( _spiDev, _dmaBuffer, payloadSize );
    transaction.end();

    c.conn->finish( c, ConnectorStateImpl::from( _dmaBuffer ) );
}

void ConnectorBus::run( SendCommand c ) {
    using namespace rofi::esp32;

    if ( c.packet.size() > 2048 ) {
        rofi::log::warning( "Trying to send big packet, ignored" );
        c.conn->finish( c );
        return;
    }
    auto transaction = c.conn->startTransaction();
    const int headerSize = 1;
    as< ProtocolCommand >( _dmaBuffer ) = ProtocolCommand::Send;
    spiWrite( _spiDev, _dmaBuffer, headerSize );

    slaveDelay();

    const int packetHeaderSize = 4;
    as< uint16_t >( _dmaBuffer ) = c.contentType;
    as< uint16_t >( _dmaBuffer + 2 ) = c.packet.size();
    spiWrite( _spiDev, _dmaBuffer, packetHeaderSize );

    for ( auto it = c.packet.chunksBegin(); it != c.packet.chunksEnd(); ++it ) {
        spiWrite( _spiDev, it->mem(), it->size() );
    }
    transaction.end();

    c.conn->finish( c );
}

void ConnectorBus::run( ReceiveCommand c ) {
    using namespace rofi::esp32;
    auto transaction = c.conn->startTransaction();

    const int headerSize = 1;
    as< ProtocolCommand >( _dmaBuffer ) = ProtocolCommand::Receive;
    spiWrite( _spiDev, _dmaBuffer, headerSize );

    slaveDelay();

    // Strangely, the original code did not work. No idea why...
    const int payloadHeaderSize = 4;
    spiRead( _spiDev, _dmaBuffer, payloadHeaderSize );
    auto contentType = as< uint16_t >( _dmaBuffer );
    auto size = as< uint16_t >( _dmaBuffer );
    // But splitting into two transactions works. Some dark magic probably.

    // const int payloadHeaderSize = 2;
    // spiRead( _spiDev, _dmaBuffer, payloadHeaderSize );
    // auto contentType = as< uint16_t >( _dmaBuffer );

    // spiRead( _spiDev, _dmaBuffer, payloadHeaderSize );
    // auto size = as< uint16_t >( _dmaBuffer );

    if ( size == 0 || size > 2048 ) {
        c.conn->finish( c );
        return;
    }

    PBuf payload = PBuf::allocate( size );
    for ( auto it = payload.chunksBegin(); it != payload.chunksEnd(); ++it ) {
        spiRead( _spiDev, it->mem(), it->size() );
    }
    transaction.end();
    c.conn->finish( c, contentType, std::move( payload ) );
}

/**
 * HAL Joint driver implementation based on Servomotor abstraction
 */
class JointLocal :
    public Joint::Implementation,
    public std::enable_shared_from_this< JointLocal >
{
public:
    JointLocal( bsp::ServoBus::Servo servo, const Capabilities& cap,
                float gearRatio ):
        _capabilities( cap ), _servo( std::move( servo ) ),
        _gearRatio( gearRatio ), _changedPwmLimit( false )
    {
        _rebootServo();
    }

    virtual const Capabilities& getCapabilities() override {
        return _capabilities;
    }

    virtual float getVelocity() override {
        return _servo.getSpeed().rad();
    }

    /**
     * \brief set velocity of the joint
     */
    virtual void setVelocity( float velocity ) override {
        // Internally we use position mode - basically we let the servo
        // go to the positive or negative limit with given speed
        _poller = rtos::Timer();
        Angle target = velocity < 0
                            ? _getMinMotorLimit()
                            : _getMaxMotorLimit();
        Angle v = Angle::rad( fabsf( velocity / _gearRatio ) );
        _handleExceptions( [&]{
            _restorePwmLimit();
            _servo.move( target, v );
        });
    }

    virtual float getPosition() override {
        return _handleExceptions( [&]{
            return _servo.getPosition().rad() * _gearRatio;
        });
    }

    virtual void setPosition( float pos, float velocity,
        std::function< void( Joint ) > callback ) override
    {
        assert( velocity > 0 );
        assert( pos >= _capabilities.minPosition && pos <= _capabilities.maxPosition );

        Angle v      = Angle::rad( velocity / _gearRatio );
        Angle target = Angle::rad( pos / _gearRatio );

        _handleExceptions( [&]{
            _restorePwmLimit();
            _servo.move( target, v );
        });
    }

    virtual float getTorque() override {
        return _handleExceptions( [&]{
            return _servo.getTorque() * _capabilities.maxTorque;
        });
    }

    virtual void setTorque( float torque ) override {
        // Internally we limit torque and perform movement with maximal
        // speed to the limit
        float limit = clamp( fabsf( torque ) / _capabilities.maxTorque, 0.0f, 1.0f );
        Angle target = torque < 0
                            ? _getMinMotorLimit()
                            : _getMaxMotorLimit();
        _handleExceptions( [&]{
            _changedPwmLimit = true;
            _servo.torqueOff();
            _servo.setTorqueLimit( limit );
            _servo.torqueOn();
            _servo.move( target, 360_deg );
        });
    }

    virtual void onError(
        std::function< void( Joint, Joint::Error, const std::string& msg ) > callback ) override
    {
        // Poll for hardware errors

        using namespace std::chrono_literals;
        _errorCallback = callback;
        _errorPoller = rtos::Timer( 200ms, rtos::Timer::Type::Periodic,
            [ this ]() {
                auto flags = _servo.status();
                if ( flags == 0 )
                    return;
                _errorCallback(
                    rofi::hal::Joint( this->shared_from_this() ),
                    rofi::hal::Joint::Error::Hardware,
                    bsp::errorName( flags ) );
                _rebootServo();
            } );
        _errorPoller.start();
    }
private:
    template < typename F >
    auto _handleExceptions( F f ) -> decltype( f() ) {
        try {
            return f();
        }
        catch ( const std::runtime_error& e ) {
            if ( _errorCallback )
                _errorCallback( rofi::hal::Joint( this->shared_from_this() ),
                                Joint::Error::Communication,
                                e.what() );
        }
        // Return default value
        return decltype( f() )();
    }

    void _restorePwmLimit() {
        if ( !_changedPwmLimit )
            return;
        _servo.torqueOff();
        _servo.setTorqueLimit( 1.0f );
        _servo.torqueOn();
    }

    Angle _getMaxMotorLimit() {
        return Angle::rad( _capabilities.maxPosition ) / _gearRatio;
    }

    Angle _getMinMotorLimit() {
        return Angle::rad( _capabilities.minPosition ) / _gearRatio;
    }

    void _rebootServo() {
        _servo.reboot();
        _servo.setTorqueLimit( 1 );
        _servo.setExtendedPositionMode();
        _servo.torqueOn();
    }

    Capabilities _capabilities;
    bsp::ServoBus::Servo _servo;
    float _gearRatio;
    bool _changedPwmLimit;
    rtos::Timer _poller;
    rtos::Timer _errorPoller;
    std::function< void( Joint, Joint::Error, const std::string& msg ) > _errorCallback;
};

rofi::hal::Joint::Implementation::Capabilities shoeJointCapability() {
    rofi::hal::Joint::Implementation::Capabilities cap;
    cap.maxPosition = Angle::pi / 2 * 1.1;   // rad
    cap.minPosition = -Angle::pi / 2 * 1.1;  // rad
    cap.maxSpeed    = 2 * Angle::pi;         // rad / s
    cap.minSpeed    = 2 * Angle::pi;         // rad / s
    cap.maxTorque   = 1.2;                   // N * m
    return cap;
}

rofi::hal::Joint::Implementation::Capabilities bodyJointCapability() {
    rofi::hal::Joint::Implementation::Capabilities cap;
    cap.maxPosition = 0.9 * Angle::pi;   // rad
    cap.minPosition = -0.9 * Angle::pi;  // rad
    cap.maxSpeed    = 2 * Angle::pi;     // rad / s
    cap.minSpeed    = 2 * Angle::pi;     // rad / s
    cap.maxTorque   = 1.2;               // N * m
    return cap;
}

class RoFILocal : public RoFI::Implementation {
public:
    RoFILocal() try:
        _servoBus( bsp::buildServoBus() ),
    #ifndef ROFI_HAL_NO_MOTORS
        _joints( {
            std::make_shared< JointLocal >(
                _servoBus.getServo( bsp::alphaId ),
                                    shoeJointCapability(),
                                    bsp::alphaRatio ),
            std::make_shared< JointLocal >(
                _servoBus.getServo( bsp::betaId ),
                                    shoeJointCapability(),
                                    bsp::betaRatio ),
            std::make_shared< JointLocal >(
                _servoBus.getServo( bsp::gammaId ),
                                    bodyJointCapability(),
                                    bsp::gammaRatio )
        } ),
    #endif
        _connectorBus( HSPI_HOST, GPIO_NUM_19, GPIO_NUM_18, 50'000'000 ),
        _connectors( {
            std::make_shared< ConnectorLocal >( &_connectorBus, GPIO_NUM_27 ),
            std::make_shared< ConnectorLocal >( &_connectorBus, GPIO_NUM_25 ),
            std::make_shared< ConnectorLocal >( &_connectorBus, GPIO_NUM_32 ),
            std::make_shared< ConnectorLocal >( &_connectorBus, GPIO_NUM_2 ),
            std::make_shared< ConnectorLocal >( &_connectorBus, GPIO_NUM_33 ),
            std::make_shared< ConnectorLocal >( &_connectorBus, GPIO_NUM_26 )
        } )
    {} catch ( const std::runtime_error& e ) {
        throw std::runtime_error( "Cannot intialize local RoFI Driver: "s + e.what() );
    }

    virtual RoFI::Id getId() const override {
        return 0;
    }

    virtual Joint getJoint( int index ) override {
        assert( index >= 0 );
        assert( index < 3 );
        return Joint( _joints[ index ] );
    }

    virtual Connector getConnector( int index ) override {
        assert( index >= 0 );
        assert( index < 6 );
        return Connector( _connectors[ index ] );
    };

    virtual RoFI::Descriptor getDescriptor() const override {
        return { 3, 6 };
    };

private:
    bsp::ServoBus _servoBus;
    std::array< std::shared_ptr< Joint::Implementation >, 3 > _joints;

    ConnectorBus _connectorBus;
    std::array< std::shared_ptr< Connector::Implementation >, 6 > _connectors;
};

} // namespace

namespace rofi::hal {

RoFI RoFI::getLocalRoFI() {
    // Definition of local RoFI driver
    static std::shared_ptr< RoFILocal > localRoFI = std::make_shared< RoFILocal >();
    return RoFI( localRoFI );
}

RoFI RoFI::getRemoteRoFI( RoFI::Id /*id*/ ) {
    throw std::runtime_error( "Remote calling is not implemented yet" );
}

} // namespace rofi::hal
