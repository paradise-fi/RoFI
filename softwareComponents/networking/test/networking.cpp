#include <catch2/catch.hpp>

#include <networking/networkManager.hpp>
#include <lwip++.hpp>
#include <iostream>
#include <memory>
#include <vector>

#include <networking/protocol.hpp>
#include <networking/protocols/simpleReactive.hpp>
#include <networking/protocols/simplePeriodic.hpp>
#include <networking/protocols/rrp.hpp>

namespace {
    using namespace rofinet;
    using namespace rofi::hal;
    TEST_CASE( "Test Ip6Addr" ) {
        SECTION( "wrapper behaves correctly" ) {
            ip6_addr_t ip;
            ip6_addr_set_zero( &ip );

            Ip6Addr addr( "::" );
            CHECK(  addr == Ip6Addr( ip ) );
            CHECK( ip6_addr_eq( &addr, &ip ) == 1 );
        }

        SECTION( "Ip6Addr mask contructor" ) {
            CHECK( Ip6Addr( "::" ) == Ip6Addr( static_cast< uint8_t >( 0 ) ) );
            CHECK( Ip6Addr( "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" ) == Ip6Addr( 128 ) );
            CHECK( Ip6Addr( "ffff:ffff:ffff:ffff::" ) == Ip6Addr( 64 ) );
            CHECK( Ip6Addr( "ffff:ffff:ffff:ffff:8000::" ) == Ip6Addr( 65 ) );
            CHECK( Ip6Addr( "ffff:ffff:ffff:ffff:c000::" ) == Ip6Addr( 66 ) );
            CHECK( Ip6Addr( "f000::" ) == Ip6Addr( 4 ) );
            CHECK( Ip6Addr( "e000::" ) == Ip6Addr( 3 ) );
        }
    }

    TEST_CASE( "Protocols" ) {
        Interface interface( PhysAddr( 1, 2, 3, 4, 5, 6 ) );
        std::vector< std::unique_ptr< Protocol > > protocols;
        protocols.push_back( std::make_unique< RRP >( RRP() ) );
        protocols.push_back( std::make_unique< SimplePeriodic >( SimplePeriodic() ) );
        protocols.push_back( std::make_unique< SimpleReactive >( SimpleReactive() ) );

        SECTION( "Manage / Ignore " ) {
            for ( auto& p : protocols ) {
                CHECK( !p->manages( interface ) );
                p->addInterface( interface );
                CHECK( p->manages( interface ) );
                REQUIRE( p->removeInterface( interface ) );
                CHECK( !p->manages( interface ) );
            }
        }

        SECTION( "Manage / Ignore with IP on the interface" ) {
            interface.addAddress( "fe80:abba::1", 64 );
            for ( auto& p : protocols ) {
                CHECK( !p->manages( interface ) );
                p->addInterface( interface );
                CHECK( p->manages( interface ) );
                REQUIRE( p->removeInterface( interface ) );
                CHECK( !p->manages( interface ) );
            }
        }
    }

} // namespace
