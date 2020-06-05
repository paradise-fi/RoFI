#pragma once

#include <vector>
#include <stdexcept>
#include <algorithm>
#include <optional>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <mutex>

#include <driver/uart.h>
#include <driver/gpio.h>

#include <atoms/units.hpp>
#include <atoms/util.hpp>

namespace rofi::herculex {

inline std::string hexDump( const char* c, int len ) {
    std::ostringstream s;
    s.fill( '0' );
    s << std::hex;
    std::string separator = "";
    for ( int i = 0; i != len; i++ ) {
        if ( i != 0 )
            s << ":";
        if ( c[ i ] <= 0xF )
            s << "0";
        s << int( c[ i ] );
    }
    return s.str();
}

inline std::string hexDump( const uint8_t* c, int len ) {
    return hexDump( reinterpret_cast< const char * >( c ), len );
}

/**
 * \brief A bit-compatible representation of the iJog data messages
 *
 * See page 46 of Herculex DRS-0101 manual for reference.
 */
struct iJog {
    int jogData : 15; ///< position or speed in ticks or ticks per second respectively
    unsigned _reserved1 : 1;
    unsigned stop : 1; ///< if specified, all movements are aborted and iJog is ignored
    unsigned speedMode : 1; ///< if true, jogData represents speed instead of position
    unsigned ledGreen : 1;
    unsigned ledBlue : 1;
    unsigned ledRed : 1;
    unsigned invalid : 1;
    unsigned _reserved2 : 2;
    unsigned id : 8; ///< servo ID
    unsigned timeMs : 8; ///< time to perform the movement in time ticks (11.2 ms)
};

inline uint16_t toPosition( Angle a ) {
    if ( a.deg() > 166.7 || a.deg() < -166.7 )
        throw std::runtime_error( "Position out of range" );
    return 512 + a.deg() / 0.325;
}

inline Angle toAngle( uint16_t rawPos ) {
    return Angle::deg( ( rawPos - 512 ) * 0.325 );
}

inline uint16_t toTicks( std::chrono::milliseconds ms ) {
    return ms.count() / 11.2;
}

inline bool containsError( uint16_t status ) {
    uint16_t mask = 0x7F;
    return status & mask;
}

inline std::string errorMessage( uint16_t status ) {
    std::string msg;
    std::string separator;
    auto add = [&]( const std::string& m ) {
        msg += separator + m;
        separator = "\n";
    };
    char err = status;
    char detail = status >> 8;
    if ( err & ( 1 << 0 ) )
        add( "Exceeded input voltage limit");
    if ( err & ( 1 << 1 ) )
        add( "Exceeded allows POT limit" );
    if ( err & ( 1 << 2) )
        add( "Exceeded tempreature limit" );
    if ( err & ( 1 << 3 ) ) {
        if ( detail & ( 1 << 2 ) )
            add( "Checksum error" );
        if ( detail & ( 1 << 3 ) )
            add( "Unknown command" );
        if ( detail & ( 1 << 3 ) )
            add( "Exceeded REG range" );
        if ( detail & ( 1 << 5 ) )
            add( "Garbage detected" );
    }
    if ( err & ( 1 << 4 ) )
        add( "Overload detected" );
    if ( err & ( 1 << 5 ) )
        add( "Driver fault detected" );
    if ( err & ( 1 << 6 ) )
        add( "EEP REG distrted" );
    return msg;
}

using Id = uint8_t;

/**
 * \brief Available commands
 *
 * See Herculex DRS-0101 manual for reference.
 */
enum class Command: uint8_t {
    EEP_WRITE = 0x01,
    EEP_READ,
    RAM_WRITE,
    RAM_READ,
    I_JOG,
    S_JOG,
    STAT,
    ROLLBACK,
    REBOOT,
    ACK_EEP_WRITE = 0x41,
    ACK_EEP_READ,
    ACK_RAM_WRITE,
    ACK_RAM_READ,
    ACK_I_JOG,
    ACK_S_JOG,
    ACK_STAT,
    ACK_ROLLBACK,
    ACK_REBOOT
};

inline std::string str( Command c ) {
    switch( c ) {
        case Command::EEP_WRITE: return "EEP_WRITE";
        case Command::EEP_READ: return "EEP_READ";
        case Command::RAM_WRITE: return "RAM_WRITE";
        case Command::RAM_READ: return "RAM_READ";
        case Command::I_JOG: return "I_JOG";
        case Command::S_JOG: return "S_JOG";
        case Command::STAT: return "STAT";
        case Command::ROLLBACK: return "ROLLBACK";
        case Command::REBOOT: return "REBOOT";
        case Command::ACK_EEP_WRITE: return "ACK_EEP_WRITE";
        case Command::ACK_EEP_READ: return "ACK_EEP_READ";
        case Command::ACK_RAM_WRITE: return "ACK_RAM_WRITE";
        case Command::ACK_RAM_READ: return "ACK_RAM_READ";
        case Command::ACK_I_JOG: return "ACK_I_JOG";
        case Command::ACK_S_JOG: return "ACK_S_JOG";
        case Command::ACK_STAT: return "ACK_STAT";
        case Command::ACK_ROLLBACK: return "ACK_ROLLBACK";
        case Command::ACK_REBOOT: return "ACK_REBOOT";
    }
    return "Unkwnow command";
}

/**
 * \brief Permanent registers addresses
 *
 * See Herculex DRS-0101 manual for reference.
 */
enum class EepRegister: uint8_t {
    ModelNo1                 = 0,
    ModelNo2                 = 1,
    Version1                 = 2,
    Version2                 = 3,
    BaudRate                 = 4,
    ID                       = 6,
    AckPolicy                = 7,
    AlarmLedPolicy           = 8,
    TorquePolicy             = 9,
    MaxTemperature           = 11,
    MinVoltage               = 12,
    MaxVoltage               = 13,
    AccelerationRatio        = 14,
    MaxAccelerationTime      = 15,
    DeadZone                 = 16,
    SaturatorOffset          = 17,
    SaturatorSlope           = 18,
    PwmOffset                = 20,
    MinPwm                   = 21,
    MaxPwm                   = 22,
    OverloadPwmThreshold     = 24,
    MinPosition              = 26,
    MaxPosition              = 28,
    PositionKp               = 30,
    PositionKd               = 32,
    PositionKi               = 34,
    PositionFF1stGain        = 36,
    PositionFF2ndGain        = 38,
    LedBlinkPeriod           = 44,
    AdcFaultCheckPeriod      = 45,
    PacketGarbageCheckPeriod = 46,
    StopDetectionPeriod      = 47,
    OverloadProtectionPeriod = 48,
    StopThreshold            = 49,
    InPositionMargin         = 50,
    CalibrationDifference    = 53
};

/**
 * \brief RAM register addresses
 *
 * See Herculex DRS-0101 manual for reference.
 */
enum class RamRegister: uint8_t {
    ID                       = 0,
    AckPolicy                = 1,
    AlarmLedPolicy           = 2,
    TorquePolicy             = 3,
    MaxTemperature           = 5,
    MinVoltage               = 6,
    MaxVoltage               = 7,
    AccelerationRatio        = 8,
    MaxAccelerationTime      = 9,
    DeadZone                 = 10,
    SaturatorOffset          = 11,
    SaturatorSlope           = 12,
    PwmOffset                = 14,
    MinPwm                   = 15,
    MaxPwm                   = 16,
    OverloadPwmThreshold     = 18,
    MinPosition              = 20,
    MaxPosition              = 22,
    PositionKp               = 24,
    PositionKd               = 26,
    PositionKi               = 28,
    PositionFF1stGain        = 30,
    PositionFF2ndGain        = 32,
    LedBlinkPeriod           = 38,
    AdcFaultCheckPeriod      = 39,
    PacketGarbageCheckPeriod = 40,
    StopDetectionPeriod      = 41,
    OverloadProtectionPeriod = 42,
    StopThreshold            = 43,
    InPositionMargin         = 44,
    CalibrationDifference    = 47,
    StatusError              = 48,
    StatusDetail             = 49,
    TorqueControl            = 52,
    LedControl               = 53,
    Voltage                  = 54,
    Temperature              = 55,
    CurrentControlMode       = 56,
    Tick                     = 57,
    CalibratedPosition       = 58,
    AbsolutePosition         = 60,
    DifferentialPosition     = 62,
    Pwm                      = 64,
    AbsoluteGoalPosition     = 68,
    AbsoluteDesiredTrajPos   = 70,
    DesiredVelocity          = 72
};

enum StatusError {
    None             = 0,
    InputVoltage     = 0b00000001,
    PotLimit         = 0b00000010,
    TemperatureLimit = 0b00000100,
    InvalidPacket    = 0b00001000,
    Overload         = 0b00010000,
    DriverFault      = 0b00100000,
    EEPDistorted     = 0b01000000,
    Reserved         = 0b10000000
};

/**
 * \brief Packet for Herculex serial communication
 *
 * See page 18 of the Herculex DSR-0101 manual for precise reference. Packet has
 * a 7-byte header:
 *
 * - 2 static bytes `0xFF 0xFF`
 * - 1 byte of size (including header)
 * - 1 byte servo ID
 * - 1 byte CMD
 * - 1 byte checksum 1
 * - 1 byte checksum 2
 */
class Packet {
public:
    /**
     * \brief Construct empty packet.
     */
    Packet() {
        _data.reserve( 16 );
    }

    /**
     * \brief Construct a packet from a binary data.
     */
    Packet( const uint8_t* data, int len ) {
        std::copy_n( data, len, std::back_inserter( _data ) );
    }

    /**
     *  \brief Construct a packet.
     *
     *  Use the types of data the same as they are specified in the manual - the
     *  actual size in the packet is inferred from the data type.
     */
    template < typename... Args >
    Packet( Id id, Command c, Args... data ) {
        _buildEmptyHeader( id, c );
        _push( data... );
        checksum1() = _calculateChecksum1();
        checksum2() = _calculateChecksum2();
    }

    /**
     * \brief Append a value to the packet data section.
     */
    template < typename T >
    void push( T t ) {
        _push( t );
        checksum1() = _calculateChecksum1();
        checksum2() = _calculateChecksum2();
    }

    int size() const { return _data[ 2 ]; }
    Id id() const { return _data[ 3 ]; }
    Command command() const { return static_cast< Command >( _data[ 4 ] ); }

    /**
     * \brief Get a pointer to constructed binary representation of the packet
     */
    const char* raw() const { return reinterpret_cast< const char * >( _data.data() ); }
    /**
     * \brief Get size of the constructed binary representation of the packet
     */
    int rawSize() const { return _data.size(); }

    /**
     * \brief Parse incoming byte and reconstruct a packet.
     *
     * \return True packet parsing was completed and the packet can be used,
     * false otherwise.
     */
    bool parseByte( uint8_t byte ) {
        if ( _data.size() == 0 ) {
            if ( byte != 0xFF )
                return false;
            _data.push_back( byte );
            return false;
        }
        if ( _data.size() == 0 ) {
            if ( byte != 0xFF ) {
                _data = {};
                return false;
            }
            _data.push_back( byte );
            return false;
        }
        if ( _data.size() < 7 || _data.size() < size() ) {
            _data.push_back( byte );
            return _data.size() == size();
        }
        throw std::runtime_error( "Packet full" );
    }

    bool valid() const {
        return _data.size() >= 7 && _data.size() == size();
        // ToDo: Check checksum
    }

    std::string dump() const {
        return str( command() ) + ": " + hexDump( _data.data() + 7, _data.size() - 7 );
    }

    /**
     * \brief Get data from packet payload.
     *
     * Interpret data on byte offset as type T and return it.
     * \tparam T type of the result. It has to a primitive type.
     * \param offset byte-offset into the data
     */
    template < typename T >
    T get( int offset = 0 ) {
        T val;
        memcpy( &val, _data.data() + 7 + offset, sizeof( T ) );
        return val;
    }

    /**
     * \brief Construct packet for reading permanent memory
     */
    static Packet eepRead( Id id, EepRegister address, uint8_t length ) {
        return Packet( id, Command::EEP_READ, address, length );
    }

    /**
     * \brief Construct packet for writing permanent memory
     */
    template < typename... Args >
    static Packet eepWrite( Id id, EepRegister address, Args... args ) {
        constexpr uint8_t size = sum< sizeof( Args )... >::value;
        return Packet( id, Command::EEP_WRITE, address, size, args... );
    }

    /**
     * \brief Construct packet for reading RAM
     */
    static Packet ramRead( Id id, RamRegister address, uint8_t length ) {
        return Packet( id, Command::RAM_READ, address, length );
    }

    /**
     * \brief Construct packet for writing RAM
     */
    template < typename... Args >
    static Packet ramWrite( Id id, RamRegister address, Args... args ) {
        constexpr uint8_t size = sum< sizeof( Args )... >::value;
        return Packet( id, Command::RAM_WRITE, address, size, args... );
    }

    static Packet iJog( const std::vector< iJog >& jogs ) {
        auto p = Packet( 0xFE, Command::I_JOG );
        for ( const auto& j : jogs )
            p._push( j );
        p.checksum1() = p._calculateChecksum1();
        p.checksum2() = p._calculateChecksum2();
        return p;
    }

    /**
     * \brief Construct packet for rollback
     */
    static Packet rollback( Id id, bool skipId = false, bool skipBaud = false ) {
        return Packet( id, Command::ROLLBACK, uint8_t( skipId ), uint8_t( skipBaud ) );
    }

    /**
     * \brief Construct packet for reboot
     */
    static Packet reboot( Id id ) {
        return Packet( id, Command::REBOOT );
    }

    /**
     * \brief Construct status packet
     */
    static Packet status( Id id ) {
        return Packet( id, Command::STAT );
    }

private:
    template < typename T >
    void _push( T t ) {
        for ( int i = 0; i != sizeof( T ); i++ ) {
            auto c = reinterpret_cast< const char * >( &t ) + i;
            _data.push_back( *c );
        }
        _size() += sizeof( T );
    }

    void _push( const rofi::herculex::iJog& t ) {
        _push< int16_t >( t.jogData );
        uint8_t set =
            t.stop |
            t.speedMode << 1 |
            t.ledGreen << 2 |
            t.ledBlue << 3 |
            t.ledRed << 4 |
            t.invalid << 5;
        _push( set );
        _push< uint8_t >( t.id );
        _push< uint8_t >( t.timeMs );
    }

    void _push() {}

    template < typename T, typename... Args >
    void _push( T t, Args... args ) {
        _push( t );
        _push( args... );
    }

    void _buildEmptyHeader( Id id, Command c ) {
        _data = { 0xFF, 0xFF, 7, id, uint8_t( c ), 0, 0 };
    }

    uint8_t _calculateChecksum1() {
        uint8_t c = 0;
        c ^= uint8_t( size() );
        c ^= uint8_t( id() );
        c ^= uint8_t( command() );
        for ( auto it = _data.begin() + 7; it != _data.end(); ++it )
            c ^= *it;
        return c & 0xFE;
    }

    uint8_t _calculateChecksum2() {
        return ( ~_calculateChecksum1() ) & 0xFE;
    }

    uint8_t& checksum1() { return _data[ 5 ]; }
    uint8_t& checksum2() { return _data[ 6 ]; }

    uint8_t& _size() { return _data[ 2 ]; }

    std::vector< uint8_t > _data;
};

inline std::string hexDump( const Packet& p ) {
    return hexDump( p.raw(), p.rawSize() );
}

/**
 * \brief Herculex servo bus
 *
 * Bus takes control of ESP32 UART peripheral and provides interface for sending
 * and receiving packets. On top of it it provides proxies in form of the Servo
 * class which can help you address individual servo motors.
 */
class Bus {
public:
    Bus( uart_port_t uart, gpio_num_t tx, gpio_num_t rx )
        : _uart( uart )
    {
        uart_config_t uart_config = {
            .baud_rate = 115200,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .rx_flow_ctrl_thresh = 0,
            .use_ref_tick = false
        };
        if ( uart_param_config( _uart, &uart_config ) != ESP_OK )
            throw std::runtime_error( "Cannot initialize bus" );
        if ( uart_set_pin( _uart, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE ) != ESP_OK )
            throw std::runtime_error( "Cannot assign pins to a bus" );
        if ( uart_driver_install( _uart, 256, 256, 0, nullptr, 0 ) != ESP_OK )
            throw std::runtime_error( "Cannot install uart driver" );
    }

    /**
     * \brief Send a packet to the bus
     */
    void send( const Packet& p ) {
        uart_flush_input( _uart );
        uart_write_bytes( _uart, p.raw(), p.rawSize() );
    }

    /**
     * \brief Read a packet from the bus
     *
     * If not packet is received within the timout, std::nullopt is returned.
     * \param ticks timeout in FreeRTOS ticks
     */
    std::optional< Packet > read( int ticks = 100 / portTICK_PERIOD_MS ) {
        Packet p;
        uint8_t buff;
        int size = uart_read_bytes( _uart, &buff, 1, ticks );
        if ( size != 1 )
            return std::nullopt;
        while ( size == 1 && !p.parseByte( buff ) ) {
            size = uart_read_bytes( _uart, &buff, 1, ticks );
        }
        if ( p.valid() )
            return p;
        return std::nullopt;
    }

    /**
     * \brief Proxy for controlling a single servomotor
     *
     * Servo can be safely copied and passed around. It provides methods to
     * directly control the servo without the need to construct packets
     * individually.
     */
    class Servo {
    public:
        /**
         * \brief enable output stage of the servomotor
         *
         * Need to be issued before any other movement command
         */
        void torqueOn() {
            std::scoped_lock _( _bus._mutex );
            _bus.send(
                Packet::ramWrite( _id, RamRegister::TorqueControl, uint8_t( 0x60 ) ) );
        }

        /**
         * \brief disable output stage of the servomotor
         */
        void torqueOff() {
            std::scoped_lock _( _bus._mutex );
            _bus.send(
                Packet::ramWrite( _id, RamRegister::TorqueControl, uint8_t( 0x00 ) ) );
        }

        /**
         * \brief configure limits of the positional control of the motor.
         *
         * The limits are checked by the servo.
         */
        void setLimits( Angle minPos, Angle maxPos ) {
            std::scoped_lock _( _bus._mutex );
            _bus.send(
                Packet::ramWrite( _id, RamRegister::MinPosition,
                                 uint16_t( toPosition( minPos ) ) ) );
            _bus.send(
                Packet::ramWrite( _id, RamRegister::MaxPosition,
                                  uint16_t( toPosition( maxPos ) ) ) );
        }

        /**
         * \brief Get current position of the motor.
         *
         * Note that it this command blocks - it waits for a round-trip to servo
         * & back
         */
        Angle getPosition() {
            std::scoped_lock _( _bus._mutex );
            _bus.send( Packet::ramRead( _id, RamRegister::AbsolutePosition, 2 ) );
            auto p = _bus.read();
            if ( p )
                return toAngle( p->get< uint16_t >( 2 ) );
            throw std::runtime_error( "Cannot read position" );
        }

        /**
         * \brief Get current speed of the motor in angle per second.
         *
         * Note that it this command blocks - it waits for a round-trip to servo
         * & back
         */
        Angle getSpeed() {
            std::scoped_lock _( _bus._mutex );
            _bus.send( Packet::ramRead( _id, RamRegister::DifferentialPosition, 2 ) );
            auto p = _bus.read();
            if ( p )
                return toAngle( p->get< uint16_t >( 2 ) ) / 0.0112;
            throw std::runtime_error( "Cannot read speed" );
        }

        /**
         * \brief Get current torque of the motor.
         *
         * Note that it this command blocks - it waits for a round-trip to servo
         * & back. The torque is estimated based on PWM value.
         */
        float getTorque() {
            std::scoped_lock _( _bus._mutex );
            _bus.send( Packet::ramRead( _id, RamRegister::Pwm, 2 ) );
            auto p = _bus.read();
            if ( p )
                return p->get< uint16_t >( 2 ) / 1023.0;
            throw std::runtime_error( "Cannot read torque" );
        }

        /**
         * \brief Move the servo to given position.
         *
         * If a synchronized movement transaction was started, the movement is
         * not executed until the transaction is started. See
         * Bus::startSynchronized().
         *
         * The duration is an approximate time in which the position should be
         * reached. The servo motor provides best-effort service.
         *
         * Has no effect without calling Servo::torqueOn() first.
         */
        void move( Angle pos, std::chrono::milliseconds dur ) {
            iJog jog{};
            jog.id = _id;
            jog.jogData = toPosition( pos );
            jog.timeMs = toTicks( dur );
            jog.speedMode = false;

            std::scoped_lock _( _bus._mutex );
            if ( !_bus._movement )
                _bus.send( Packet::iJog( { jog } ) );
            else
                _bus._movement.value().push_back( jog );
        }

        /**
         * \brief Start turning the servo at given speed.
         *
         * If a synchronized movement transaction was started, the movement is
         * not executed until the transaction is started. See
         * Bus::startSynchronized().
         *
         * \param pwm pwm value in range <-1023, 1023>
         *
         * Has no effect without calling Servo::torqueOn() first.
         */
        void rotate( int pwm ) {
            if ( pwm > 1023 || pwm < -1023 )
                throw std::runtime_error( "Invalid PWM value specified" );
            iJog jog{};
            jog.id = _id;
            jog.jogData = abs( pwm );
            if ( pwm < 0 )
                jog.jogData |= 0x4000;
            jog.timeMs = 255;
            jog.speedMode = true;

            std::scoped_lock _( _bus._mutex );
            if ( !_bus._movement )
                _bus.send( Packet::iJog( { jog } ) );
            else
                _bus._movement.value().push_back( jog );
        }

        /**
         * \brief Reset error flags
         */
        void resetErrors() {
            _bus.send( Packet::ramWrite( _id, RamRegister::StatusError, uint16_t( 0 ) ) );
        }

        /**
         * \brief Get status bitmask
         *
         * See manual page 39 for explanation of meaning of individual bits.
         */
        uint16_t status() {
            std::scoped_lock _( _bus._mutex );
            _bus.send( Packet::status( _id ) );
            auto p = _bus.read( 10 / portTICK_PERIOD_MS );
            if ( p )
                return p->get< uint16_t >( 0 );
            throw std::runtime_error( "Cannot read status" );
        }

        /**
         * \brief Check if the servo is alive on the bus
         */
        bool active() {
            std::scoped_lock _( _bus._mutex );
            _bus.send( Packet::status( _id ) );
            auto p = _bus.read( 10 / portTICK_PERIOD_MS );
            // return p;
            // return p.has_value() - none of these works, hack below
            if ( p )
                return true;
            return false;
        }

        friend class Bus;
    private:
        Servo( Id id, Bus& bus ) : _id( id ), _bus( bus ) {}

        Id _id;
        Bus& _bus;
    };

    /**
     * \brief Synchronized movement transaction RAII guard.
     *
     * You can obtain an instance of this class by calling
     * Bus::startSynchronized(). You can execute the transaction by calling
     * SyncMovement::execute(). Unexecuted transactions are automatically
     * canceled with end of the life of the guard.
     */
    class SyncMovement {
    public:
        friend class Bus;

        /**
         * Execute the transation. For more details about the synchronized
         * movement transaction see Bus::startSynchronized().
         */
        void execute() {
            _bus->syncMovement();
        }

        ~SyncMovement() {
            if ( _bus && _bus->_movement )
                _bus->abortMovement();
        }

        SyncMovement( const SyncMovement& ) = delete;
        SyncMovement( SyncMovement&& other ) {
            std::swap( _bus, other._bus );
        }
    private:
        SyncMovement( Bus* bus ) : _bus( bus ) {
            bus->startMovement();
        }

        Bus* _bus;
    };

    Servo getServo( Id id ) {
        return Servo( id, *this );
    }

    Servo allServos() {
        return Servo( 0xFE, *this );
    }

    /**
     * \brief Start synchronized movement transaction of multiple servos and get
     * transaction guard.
     *
     * To start a synchronized movement of multiple servos, call this method.
     * You obtain the transaction guard. While the guard is alive, all movement
     * command on the servos will be added to the transaction instead of
     * executing them immediately. When all the commands are supplied, call
     * SyncMovement::execute() to perform the tansaction. If you do not call
     * SyncMovement::execute(), the transaction is aborted when the guard goes
     * out of scope.
     */
    [[nodiscard]] SyncMovement startSynchronized() {
        return SyncMovement( this );
    }

private:
    void startMovement() {
        _movement.emplace();
    }
    void abortMovement() {
        _movement = std::nullopt;
    }

    void syncMovement() {
        if ( !_movement )
            throw std::runtime_error( "No synchronized movement started" );
        send( Packet::iJog( _movement.value() ) );
        _movement = std::nullopt;
    }

    uart_port_t _uart;
    std::mutex _mutex;
    std::optional< std::vector< iJog > > _movement;
};

} // namespace rofi::herculex