#pragma once

#include <vector>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <sstream>
#include <iomanip>

namespace rofi::hal::dynamixel {

using Id = uint8_t;

enum class Instruction: uint8_t {
    Ping = 0x01,
    Read,
    Write,
    RegWrite,
    Action,
    FactoryReset,
    Reboot = 0x08,
    Clear = 0x10,
    ControlTableBackup = 0x20,
    Status = 0x55,
    SyncRead = 0x82,
    SyncWrite,
    FastSyncRead = 0x8A,
    BulkRead = 0x92,
    BulkWrite = 0x93,
    FastBulkRead = 0x9A
};

enum class Register: uint16_t {
    ModelNumber = 0,
    ModelInformation = 2,
    FirmwareVersion = 6,
    ID = 7,
    BaudRate = 8,
    ReturnDelayTime = 9,
    DriveMode = 10,
    OperatingMode = 11,
    SecondaryID = 12,
    ProtocolType = 13,
    HomingOffset = 20,
    MovingThreshold = 24,
    TemperatureLimit = 31,
    MaxVoltageLimit = 32,
    MinVoltageLimit = 34,
    PWMLimit = 36,
    VelocityLimit = 44,
    MaxPositionLimit = 48,
    MinPositionLimit = 52,
    StartupConfiguration = 60,
    Shutdown = 63,
    TorqueEnable = 64,
    LED = 65,
    StatusReturnLevel = 68,
    RegisteredInstruction = 69,
    HardwareErrorStatus = 70,
    VelocityIGain = 76,
    VelocityPGain = 78,
    PositionDGain = 80,
    PositionIGain = 82,
    PositionPGain = 84,
    Feedforward2ndGain = 88,
    Feedforward1stGain = 90,
    BusWatchdog = 98,
    GoalPWM = 100,
    GoalVelocity = 104,
    ProfileAcceleration = 108,
    ProfileVelocity = 112,
    GoalPosition = 116,
    RealtimeTick = 120,
    Moving = 122,
    MovingStatus = 123,
    PresentPWM = 124,
    PresentLoad = 126,
    PresentVelocity = 128,
    PresentPosition = 132,
    VelocityTrajectory = 136,
    PositionTrajectory = 140,
    PresentInputVoltage = 144,
    PresentTemperature = 146,
    BackupReady = 147,
};

inline constexpr uint16_t registerSize( Register r ) {
    switch (r) {
        case Register::ModelNumber: return 2;
        case Register::ModelInformation: return 4;
        case Register::FirmwareVersion: return 1;
        case Register::ID: return 1;
        case Register::BaudRate: return 1;
        case Register::ReturnDelayTime: return 1;
        case Register::DriveMode: return 1;
        case Register::OperatingMode: return 1;
        case Register::SecondaryID: return 1;
        case Register::ProtocolType: return 1;
        case Register::HomingOffset: return 4;
        case Register::MovingThreshold: return 4;
        case Register::TemperatureLimit: return 1;
        case Register::MaxVoltageLimit: return 2;
        case Register::MinVoltageLimit: return 2;
        case Register::PWMLimit: return 2;
        case Register::VelocityLimit: return 4;
        case Register::MaxPositionLimit: return 4;
        case Register::MinPositionLimit: return 4;
        case Register::StartupConfiguration: return 1;
        case Register::Shutdown: return 1;
        case Register::TorqueEnable: return 1;
        case Register::LED: return 1;
        case Register::StatusReturnLevel: return 1;
        case Register::RegisteredInstruction: return 1;
        case Register::HardwareErrorStatus: return 1;
        case Register::VelocityIGain: return 2;
        case Register::VelocityPGain: return 2;
        case Register::PositionDGain: return 2;
        case Register::PositionIGain: return 2;
        case Register::PositionPGain: return 2;
        case Register::Feedforward2ndGain: return 2;
        case Register::Feedforward1stGain: return 2;
        case Register::BusWatchdog: return 1;
        case Register::GoalPWM: return 2;
        case Register::GoalVelocity: return 4;
        case Register::ProfileAcceleration: return 4;
        case Register::ProfileVelocity: return 4;
        case Register::GoalPosition: return 4;
        case Register::RealtimeTick: return 2;
        case Register::Moving: return 1;
        case Register::MovingStatus: return 1;
        case Register::PresentPWM: return 2;
        case Register::PresentLoad: return 2;
        case Register::PresentVelocity: return 4;
        case Register::PresentPosition: return 4;
        case Register::VelocityTrajectory: return 4;
        case Register::PositionTrajectory: return 4;
        case Register::PresentInputVoltage: return 2;
        case Register::PresentTemperature: return 1;
        case Register::BackupReady: return 1;
    }
    __builtin_trap();
}

std::string interpretRegisterValue( Register r, int v ) {
    switch (r) {
        case Register::ReturnDelayTime:
            return std::to_string( v * 2 ) + " us";
        case Register::MovingThreshold:
        case Register::VelocityLimit:
        case Register::GoalVelocity:
        case Register::ProfileVelocity:
        case Register::PresentVelocity:
        case Register::VelocityTrajectory: {
            std::ostringstream s;
            s << std::setprecision( 2 ) << std::to_string( v * 0.229 ) << " rev/min";
            return s.str();
        }
        case Register::TemperatureLimit:
        case Register::PresentTemperature:
            return std::to_string( v ) + " °C";
        case Register::MaxVoltageLimit:
        case Register::MinVoltageLimit:
        case Register::PresentInputVoltage: {
            std::ostringstream s;
            s << std::setprecision( 2 ) << std::to_string( v / 10 ) << " V";
            return s.str();
        }
        default:
            return "";
    }
}


enum class Error: uint8_t {
    Nothing = 0,
    ResultFail = 1,
    Instruction,
    CRC,
    DataRange,
    DataLength,
    DataLimit,
    Access
};

enum HardwareError: uint8_t {
    InputVoltage    = 1 << 0,
    Overheating     = 1 << 2,
    MotorEncoder    = 1 << 3,
    ElectricalShock = 1 << 4,
    Overloaded      = 1 << 5
};

// The following function is taken as is from datasheet with minor tweaks to
// types in order to not trigger warnings
inline uint16_t update_crc(uint16_t crc_accum, const uint8_t *data_blk_ptr, int data_blk_size)
{
    int i, j;
    static uint16_t crc_table[256] = {
        0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
        0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
        0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
        0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
        0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
        0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
        0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
        0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
        0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
        0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
        0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
        0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
        0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
        0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
        0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
        0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
        0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
        0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
        0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
        0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
        0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
        0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
        0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
        0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
        0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
        0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
        0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
        0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
        0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
        0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
        0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
        0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202
    };

    for(j = 0; j < data_blk_size; j++)
    {
        i = (uint16_t(crc_accum >> 8) ^ data_blk_ptr[j]) & 0xFF;
        crc_accum = uint16_t((crc_accum << 8) ^ crc_table[i]);
    }

    return crc_accum;
}

/**
 * \brief Packet for Dynamixel serial communicatoin
 *
 */
class Packet {
public:
    /**
     * \brief Construct a packet from a binary data.
     */
    Packet( const uint8_t* data, int len ) {
        _data.resize( len );
        memcpy( _data.data(), data, len );
    }

    /**
     * \brief Construct a packet from existing data
     */
    Packet( std::vector< uint8_t >&& data ): _data( std::move( data ) ) {};

    /**
     *  \brief Construct a packet.
     *
     *  Use the types of data the same as they are specified in the manual - the
     *  actual size in the packet is inferred from the data type.
     */
    template < typename... Args >
    Packet( Id id, Instruction inst, Args... data ) {
        _data.reserve( 20 );
        _buildEmptyHeader( id, inst );
        _push( data... );
        _appendChecksum();
    }

    /**
     * \brief Get data from packet payload.
     *
     * Interpret data on byte offset as type T and return it.
     * \tparam T type of the result. It has to a primitive type.
     * \param offset byte-offset into the data
     */
    template < typename T >
    T get( int offset = 0 ) const {
        if (instruction() == Instruction::Status)
            offset += 1; // There is extra error field
        T val;
        memcpy( &val, _data.data() + 8 + offset, sizeof( T ) );
        return val;
    }

    int size() const {
        return *reinterpret_cast< const uint16_t * >( _data.data() + 5 );
    }
    Id id() const { return _data[ 4 ]; }
    Instruction instruction() const {
        return static_cast< Instruction >( _data[ 7 ] );
    }

    Error error() const {
        assert( instruction() == Instruction::Status );
        return Error( _data[ 8 ] & 0b00111111 );
    }

    /**
     * \brief Get a pointer to constructed binary representation of the packet
     */
    const char* raw() const { return reinterpret_cast< const char * >( _data.data() ); }

    /**
     * \brief Get size of the constructed binary representation of the packet
     */
    int rawSize() const { return static_cast< int >( _data.size() ); }

    bool valid() const {
        bool v = rawSize() >= 10 && rawSize() == 7 + size();
        if ( !v )
            return false;
        uint16_t crc = update_crc( 0, _data.data(), rawSize() - 2 );
        return crc == *reinterpret_cast< const uint16_t * >( _data.data() + rawSize() - 2 );
    }

    static Packet ping( Id id ) {
        return Packet( id, Instruction::Ping );
    }

    static Packet read( Id id, Register r ) {
        return Packet( id, Instruction::Read, r, registerSize( r ) );
    }

    template < typename T >
    static Packet write( Id id, Register r, T value ) {
        auto size = registerSize( r );
        assert( sizeof( T ) == size );
        return Packet( id, Instruction::Write, r, value );
    }

    static Packet reboot( Id id ) {
        return Packet( id, Instruction::Reboot );
    }

    static Packet factoryReset( Id id, uint8_t mode ) {
        return Packet( id, Instruction::FactoryReset, mode );
    }
private:
    void _buildEmptyHeader( Id id, Instruction inst ) {
        const uint8_t header[] = { 0xFF, 0xFF, 0xFD, 0, id, 3, 0, uint8_t( inst ) };
        _data.resize( sizeof( header ) );
        memcpy( _data.data(), header, sizeof( header ) );
    }

    template < typename T >
    void _push( T t ) {
        auto offset = _data.size();
        _data.resize( _data.size() + sizeof( T ) );
        memcpy( _data.data() + offset, &t, sizeof( T ) );
        _size() += sizeof( T );
    }

    void _push() {}

    void _appendChecksum() {
        int expectedSize = 7 + _size();
        _data.resize( expectedSize );

        uint16_t crc = update_crc( 0, _data.data(), expectedSize - 2 );
        memcpy( _data.data() + expectedSize - 2, &crc, 2 );
    }


    template < typename T, typename... Args >
    void _push( T t, Args... args ) {
        _push( t );
        _push( args... );
    }


    uint16_t& _size() {
        return *reinterpret_cast< uint16_t * >( _data.data() + 5 );
    }

    std::vector< uint8_t > _data;
};

class PacketParser {
public:
    /**
     * \brief Parse incoming byte and reconstruct a packet.
     *
     * \return True packet parsing was completed and the packet can be used,
     * false otherwise.
     */
    bool parseByte( uint8_t byte ) {
        static const uint8_t header[ 4 ] = { 0xFF, 0xFF, 0xFD, 0 };
        if ( _data.size() < 4 ) {
            if ( byte == header[ _data.size() ] )
                _data.push_back( byte );
            else {
                _data.clear();
            }
            return false;
        }
        if ( _data.size() < 8 ) {
            _data.push_back( byte );
            return false;
        }
        if ( _data.size() < 7 + _size() ) {
            _data.push_back( byte );
            _finished = _data.size() == 7 + _size();
            return _finished;
        }
        throw std::runtime_error( "Packet full" );
    }

    bool done() const {
        return _finished;
    }

    [[nodiscard]] Packet getPacket() {
        assert( _finished );
        _finished = false;
        return Packet( std::move( _data ) );
    }

private:
    unsigned _size() const {
        assert( _data.size() >= 7 );
        return *reinterpret_cast< const uint16_t * >( _data.data() + 5 );
    }

    std::vector< uint8_t > _data;
    bool _finished = false;
};


} // namespace rofi::hal::dynamixel
