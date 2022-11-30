#include <catch2/catch.hpp>

#include <networking/routingTable.hpp>
#include <networking/interface.hpp>

#include "lwip++.hpp"
#include <iostream>

namespace {
    using namespace rofi::net;
    using namespace rofi::hal;

    auto dummyLogFun = []( Logger::Level, const std::string&, const std::string& ) {};
    auto dummyNetmgCB = []( const Interface*, ConnectorEvent ) {};

    TEST_CASE( "RoutingTable" ) {
        RoutingTable rt;

        SECTION( "Add" ) {
            CHECK( rt.empty() );
            CHECK( rt.add( "fe80::1"_ip, 64, "eth0", 10 ) );
            CHECK_FALSE( rt.empty() );
            auto size = rt.size();
            CHECK_FALSE( rt.add( "fe80::1"_ip, 64, "eth0", 10 ) );
            CHECK( size == rt.size() );
            REQUIRE( rt.add( "fe80::2"_ip, 64, "eth0", 10 ) );
            size++;
            CHECK( size == rt.size() );
            auto* ptr = rt.find( "fe80::1"_ip, 64 );
            CHECK( ptr->size() == 1 );
            CHECK( rt.add( "fe80::1"_ip, 64, "eth1", 15 ) );
            ptr = rt.find( "fe80::1"_ip, 64 );
            CHECK( ptr->size() == 2 );
            CHECK( size == rt.size() ); // the # of records is the same, the first one has now two gateways!

            ptr = rt.find( "fe80::1"_ip, 64 );
            REQUIRE( ptr != nullptr );
            REQUIRE( ptr->best().has_value() );
            CHECK( ptr->best()->name() == "eth0" );
            // check that add is stable
            CHECK( rt.add( "fe80::1"_ip, 64, "eth3", 10 ) );
            ptr = rt.find( "fe80::1"_ip, 64 );
            REQUIRE( ptr != nullptr );
            CHECK( ptr->size() == 3 ); // eth0, eth1, and eth3
            CHECK( ptr->best()->name() == "eth0" ); // eth0 remains the best even though newer eth3 has the same cost
        }

        SECTION( "find" ) {
            rt.add( "fe80::1"_ip, 64, "eth0", 10 );
            rt.add( "fe80::2"_ip, 64, "eth1", 10 );
            rt.add( "fe80::1"_ip, 64, "eth3", 10 );

            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 48 ) == nullptr );
            CHECK( rt.find( "fc07::a", 80 ) == nullptr );
        }

        SECTION( "removeInterface" ) {
            rt.add( "fe80::1"_ip, 64, "eth0", 10 );
            rt.add( "fe80::2"_ip, 64, "eth1", 10 );
            rt.add( "fe80::1"_ip, 64, "eth3", 10 );

            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.removeInterface( "fe80::1"_ip, 64, "eth0" ) );
            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.removeInterface( "fe80::2"_ip, 64, "eth1" ) );
            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) == nullptr );
            CHECK( rt.removeInterface( "fe80::1"_ip, 64, "eth3" ) );
            CHECK( rt.find( "fe80::1"_ip, 64 ) == nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) == nullptr );
            CHECK_FALSE( rt.removeInterface( "fe80::1"_ip, 64, "eth1" ) );
        }

        SECTION( "removeNetwork" ) {
            rt.add( "fe80::1"_ip, 64, "eth0", 10 );
            rt.add( "fe80::2"_ip, 64, "eth1", 10 );
            rt.add( "fe80::1"_ip, 64, "eth3", 10 );

            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.removeNetwork( "fe80::1"_ip, 64 ) );
            CHECK( rt.find( "fe80::1"_ip, 64 ) == nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.removeNetwork( "fe80::2"_ip, 64 ) );
            CHECK( rt.find( "fe80::1"_ip, 64 ) == nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) == nullptr );
            CHECK_FALSE( rt.removeNetwork( "fe80::1"_ip, 64 ) );
            CHECK( rt.find( "fe80::1"_ip, 64 ) == nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) == nullptr );
            CHECK_FALSE( rt.removeInterface( "fe80::1"_ip, 64, "eth1" ) );
        }

        SECTION( "purge" ) {
            rt.add( "fe80::1"_ip, 64, "eth0", 10 );
            rt.add( "fe80::2"_ip, 64, "eth1", 10 );
            rt.add( "fe80::1"_ip, 64, "eth3", 10 );
            rt.add( "fe80::2"_ip, 64, "eth3", 11 );

            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::1"_ip, 64 )->best()->name() == "eth0" );
            CHECK( rt.find( "fe80::2"_ip, 64 )->best()->name() == "eth1" );
            rt.purge( "eth0" );
            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 )->best()->name() == "eth1" );
            CHECK( rt.find( "fe80::1"_ip, 64 )->best()->name() == "eth3" );
            rt.purge( "eth1" );
            CHECK( rt.find( "fe80::1"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) != nullptr );
            CHECK( rt.find( "fe80::1"_ip, 64 )->best()->name() == "eth3" );
            CHECK( rt.find( "fe80::2"_ip, 64 )->best()->name() == "eth3" );
            rt.purge( "eth3" );
            CHECK( rt.find( "fe80::1"_ip, 64 ) == nullptr );
            CHECK( rt.find( "fe80::2"_ip, 64 ) == nullptr );
        }

        SECTION( "updating lwip's forwarding table" ) {
            ip_clear();
            REQUIRE( rt.empty() );
            Interface interface1( PhysAddr( 1, 2, 3, 4, 5, 6 ), dummyLogFun, std::nullopt, dummyNetmgCB );
            Interface interface2( PhysAddr( 6, 5, 4, 3, 2, 1 ), dummyLogFun, std::nullopt, dummyNetmgCB );
            auto ip = "fe80::1"_ip;
            // ip_find_route_entry is used in tests because it does not require lwip initialized
            CHECK( ip_find_route_entry( &ip ) == nullptr );
            CHECK( rt.add( ip, 64, interface1.name(), 10 ) );
            REQUIRE( ip_find_route_entry( &ip ) != nullptr );
            CHECK( interface1.name() == ip_find_route_entry( &ip )->gw_name );
            CHECK( rt.add( ip, 64, interface2.name(), 15 ) ); // bigger cost, no change for lwip
            REQUIRE( ip_find_route_entry( &ip ) != nullptr );
            CHECK( interface1.name() == ip_find_route_entry( &ip )->gw_name );
            CHECK( rt.add( ip, 64, interface2.name(), 5 ) ); // lower cost, the best route should change in lwip
            REQUIRE( ip_find_route_entry( &ip ) != nullptr );
            CHECK( interface2.name() == ip_find_route_entry( &ip )->gw_name );
            // remove
            CHECK( rt.removeInterface( ip, 64, interface2.name() ) );
            CHECK( interface1.name() == ip_find_route_entry( &ip )->gw_name );
            // more specific subnet
            CHECK( rt.add( ip, 80, interface2.name(), 100 ) );
            CHECK( interface2.name() == ip_find_route_entry( &ip )->gw_name );
            // purge
            rt.purge( interface2.name() );
            CHECK( interface1.name() == ip_find_route_entry( &ip )->gw_name );
        }
    }

} // namespace
