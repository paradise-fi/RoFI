#include <catch2/catch.hpp>

#include <configuration/serialization.hpp>
#include <iostream>

using namespace rofi::configuration;
using namespace rofi::configuration::roficom;

namespace {

TEST_CASE( "UniversalModule – Demo" ) {
    ModuleId idCounter = 0;
    Rofibot bot;
    auto& m1 = bot.insert( UniversalModule( idCounter++, 0_deg, 90_deg,   0_deg ) );
    auto& m2 = bot.insert( UniversalModule( idCounter++, 0_deg,  0_deg, 180_deg ) );
    auto con = m2.connectors()[ 1 ];
    connect( m1.connectors()[ 0 ], con, Orientation::South );
    connect< RigidJoint >( m1.bodies()[ 0 ], { 0, 0, 0 }, identity );
    REQUIRE_NOTHROW( bot.prepare() );

    auto j = serialization::toJSON( bot );
    std::string js_str = j.dump( 4 );

    Rofibot botj = serialization::fromJSON< Rofibot >( j );
    auto j_cpy   = serialization::toJSON( botj );
    std::string js_str_cpy = j_cpy.dump( 4 );

    CHECK( j == j_cpy );
    CHECK( js_str == js_str_cpy );

    REQUIRE( bot.modules().size() == botj.modules().size() );
    REQUIRE( bot.roficoms().size() == botj.roficoms().size() );
    REQUIRE( bot.referencePoints().size() == botj.referencePoints().size() );
    REQUIRE_NOTHROW( botj.prepare() );

    for ( auto& m : bot.modules() ) {
        ModuleId id = m.module->getId();
        CHECK( equals( bot.getModulePosition( id ), botj.getModulePosition( id ) ) );
    }

}

} // namespace
