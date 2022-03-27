#include "simplesim/packet_filters/py_filter.hpp"

#define CATCH_CONFIG_ENABLE_ALL_STRINGMAKERS
#include <catch2/catch.hpp>

using rofi::simplesim::PacketFilter;
using rofi::simplesim::packetf::PyFilter;

rofi::messages::Packet createPacket( std::string data )
{
    auto packet = rofi::messages::Packet();
    packet.set_message( std::move( data ) );
    return packet;
}

TEST_CASE( "Python packet filter" )
{
    SECTION( "Default" )
    {
        auto defFilter = PyFilter(
                "def filter(packet, sender, receiver):\n    return (packet, 0)\n" );
        rofi::simplesim::PacketFilter::FilterFunction filter = defFilter;

        SECTION( "Empty packet" )
        {
            auto expected = PacketFilter::DelayedPacket{};
            auto result = filter( PacketFilter::SendPacketData() );

            CHECK( result.delay == expected.delay );
            CHECK( result.packet.message() == expected.packet.message() );
        }
        SECTION( "Simple packet" )
        {
            auto expected = PacketFilter::DelayedPacket{ .packet = createPacket( "My packet!" ) };
            auto result = filter( PacketFilter::SendPacketData{ .sender = {},
                                                                .receiver = {},
                                                                .packet = expected.packet } );

            CHECK( result.delay == expected.delay );
            CHECK( result.packet.message() == expected.packet.message() );
        }
    }

    SECTION( "Constant delay" )
    {
        rofi::simplesim::PacketFilter::FilterFunction filter = PyFilter(
                "def filter(packet, sender, receiver):\n    return (packet, 1000)\n" );

        SECTION( "Empty packet" )
        {
            auto expected = PacketFilter::DelayedPacket{ .packet = {},
                                                         .delay = std::chrono::seconds( 1 ) };
            auto result = filter( PacketFilter::SendPacketData() );

            CHECK( result.delay == expected.delay );
            CHECK( result.packet.message() == expected.packet.message() );
        }
        SECTION( "Simple packet" )
        {
            auto expected = PacketFilter::DelayedPacket{ .packet = createPacket( "My packet!" ),
                                                         .delay = std::chrono::seconds( 1 ) };
            auto result = filter( PacketFilter::SendPacketData{ .sender = {},
                                                                .receiver = {},
                                                                .packet = expected.packet } );

            CHECK( result.delay == expected.delay );
            CHECK( result.packet.message() == expected.packet.message() );
        }
    }
    SECTION( "Throw away packet" )
    {
        rofi::simplesim::PacketFilter::FilterFunction filter = PyFilter(
                "def filter(packet, sender, receiver):\n    return None\n" );

        SECTION( "Empty packet" )
        {
            auto result = filter( PacketFilter::SendPacketData() );
            CHECK( result.delay == std::chrono::milliseconds( -1 ) );
        }
        SECTION( "Simple packet" )
        {
            auto result = filter(
                    PacketFilter::SendPacketData{ .sender = {},
                                                  .receiver = {},
                                                  .packet = createPacket( "My packet!" ) } );
            CHECK( result.delay == std::chrono::milliseconds( -1 ) );
        }
    }

    SECTION( "Error on loading" )
    {
        CHECK_THROWS( PyFilter( "" ), "Empty input" );
        CHECK_THROWS( PyFilter( "def func():\n    pass\n" ), "Filter not defined" );
    }
    SECTION( "Error on execution" )
    {
        // Used for inlining PyFilter
        auto pyGuard = rofi::simplesim::packetf::GlobalPyInterpreterGuard::instance();

        // Can throw in either in constructor or at first call to `filter`
        CHECK_THROWS( PyFilter( "def filter():\n    return None\n" ).filter( {} ),
                      "Wrong number of arguments" );
    }
}
