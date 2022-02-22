#include <catch2/catch.hpp>
#include <peripherals/dynamixel/packet.hpp>

using namespace rofi::hal::dynamixel;

TEST_CASE( "Simple packet construction" ) {
    Packet p( 1, Instruction::Write,
        uint8_t( 0x40 ), uint8_t( 0x00 ), uint8_t( 0x01 ) );
    REQUIRE( p.valid() );
    REQUIRE( p.rawSize() == 13 );
    REQUIRE( p.id() == 1 );
    REQUIRE( p.instruction() == Instruction::Write );
    REQUIRE( p.size() == 6 );

    std::vector< uint8_t > ref({
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x06, 0x00, 0x03, 0x40, 0x00, 0x01, 0xDB, 0x66
    });
    std::vector< uint8_t > pData( p.raw(), p.raw() + p.rawSize() );
    REQUIRE( ref == pData );
}

TEST_CASE( "Ping packet" ) {
    auto p = Packet::ping( 1 );
    REQUIRE( p.valid() );
    REQUIRE( p.rawSize() == 10 );
    REQUIRE( p.id() == 1 );
    REQUIRE( p.instruction() == Instruction::Ping );
    REQUIRE( p.size() == 3 );

    std::vector< uint8_t > ref({
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4E
    });
    std::vector< uint8_t > pData( p.raw(), p.raw() + p.rawSize() );
    REQUIRE( ref == pData );
}

TEST_CASE( "Ping packet with ID 0" ) {
    auto p = Packet::ping( 0 );
    REQUIRE( p.valid() );
    REQUIRE( p.rawSize() == 10 );
    REQUIRE( p.id() == 0 );
    REQUIRE( p.instruction() == Instruction::Ping );
    REQUIRE( p.size() == 3 );

    PacketParser parser;
    for ( int i = 0; i != p.rawSize(); i++ ) {
        parser.parseByte( p.raw()[ i ] );
    }
    REQUIRE( parser.done() );
    Packet p2 = parser.getPacket();

    REQUIRE( p2.valid() );
    REQUIRE( p2.rawSize() == 10 );
    REQUIRE( p2.id() == 0 );
    REQUIRE( p2.instruction() == Instruction::Ping );
    REQUIRE( p2.size() == 3 );
}


TEST_CASE( "Simple packet parsing" ) {
    PacketParser parser;

    std::vector< uint8_t > ref({
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x06, 0x00, 0x03, 0x40, 0x00, 0x01, 0xDB, 0x66
    });
    unsigned idx = 0;
    for ( uint8_t byte : ref ) {
        idx++;
        bool r = parser.parseByte( byte );
        CAPTURE( idx );
        if ( idx == ref.size() )
            REQUIRE( r );
        else
            REQUIRE( !r );
    }
    REQUIRE( parser.done() );
    Packet p = parser.getPacket();

    REQUIRE( p.valid() );
    REQUIRE( p.rawSize() == 13 );
    REQUIRE( p.id() == 1 );
    REQUIRE( p.instruction() == Instruction::Write );
    REQUIRE( p.size() == 6 );

    std::vector< uint8_t > pData( p.raw(), p.raw() + p.rawSize() );
    REQUIRE( ref == pData );
}

TEST_CASE( "Ping packet parsing" ) {
    PacketParser parser;

    std::vector< uint8_t > ref({
        0xFF, 0xFF, 0xFD, 0x00, 0x01, 0x03, 0x00, 0x01, 0x19, 0x4E
    });
    unsigned idx = 0;
    for ( uint8_t byte : ref ) {
        idx++;
        bool r = parser.parseByte( byte );
        CAPTURE( idx );
        if ( idx == ref.size() )
            REQUIRE( r );
        else
            REQUIRE( !r );
    }
    REQUIRE( parser.done() );
    Packet p = parser.getPacket();

    REQUIRE( p.valid() );
    REQUIRE( p.rawSize() == 10 );
    REQUIRE( p.id() == 1 );
    REQUIRE( p.instruction() == Instruction::Ping );
    REQUIRE( p.size() == 3 );

    std::vector< uint8_t > pData( p.raw(), p.raw() + p.rawSize() );
    REQUIRE( ref == pData );
}