#include <catch.hpp>
#include <smtReconfig.hpp>

using namespace rofi::smtr;

TEST_CASE("SmtConfiguration building") {
    Configuration rofiCfg;
    z3::context c;

    auto smtEmpty = buildConfiguration( c, rofiCfg, 0 );
    REQUIRE( smtEmpty.modules.size() == 0 );
    REQUIRE( smtEmpty.connections.size() == 0 );

    rofiCfg.addModule( 0, 0, 0, 42 );
    auto smt1 = buildConfiguration( c, rofiCfg, 0 );
    REQUIRE( smt1.modules.size() == 1 );
    REQUIRE( smt1.connections.size() == 0 );

    rofiCfg.addModule( 0, 0, 0, 43 );
    auto smt2 = buildConfiguration( c, rofiCfg, 0 );
    CAPTURE( collectVar( smt2 ) );
    REQUIRE( smt2.modules.size() == 2 );
    REQUIRE( smt2.connections.size() == 1 );

    // Randomly probe the connections
    REQUIRE( smt2.connection( 0, ShoeId::A, XPlus, 1, ShoeId::A, XMinus, North )
        .decl().name().str() == "cfg0_c_42A+X_43A-X_N" );
    REQUIRE( smt2.connection( 1, ShoeId::A, XMinus, 0, ShoeId::A, XPlus, North )
        .decl().name().str() == "cfg0_c_42A+X_43A-X_N" );

    REQUIRE( smt2.connection( 0, ShoeId::A, XPlus, 1, ShoeId::B, XMinus, North )
        .decl().name().str() == "cfg0_c_42A+X_43B-X_N" );
    REQUIRE( smt2.connection( 1, ShoeId::B, XMinus, 0, ShoeId::A, XPlus, North )
        .decl().name().str() == "cfg0_c_42A+X_43B-X_N" );

    REQUIRE( smt2.connection( 0, ShoeId::A, XPlus, 1, ShoeId::B, ZMinus, East )
        .decl().name().str() == "cfg0_c_42A+X_43B-Z_E" );
    REQUIRE( smt2.connection( 1, ShoeId::B, ZMinus, 0, ShoeId::A, XPlus, East )
        .decl().name().str() == "cfg0_c_42A+X_43B-Z_E" );

    rofiCfg.addModule( 0, 0, 0, 44 );
    auto smt3 = buildConfiguration( c, rofiCfg, 0 );
    CAPTURE( collectVar( smt3 ) );
    REQUIRE( smt3.modules.size() == 3 );
    REQUIRE( smt3.connections.size() == 2 );

    REQUIRE( smt3.connection( 0, ShoeId::A, XPlus, 1, ShoeId::A, XMinus, North )
        .decl().name().str() == "cfg0_c_42A+X_43A-X_N" );
    REQUIRE( smt3.connection( 1, ShoeId::A, XMinus, 0, ShoeId::A, XPlus, North )
        .decl().name().str() == "cfg0_c_42A+X_43A-X_N" );

    REQUIRE( smt3.connection( 0, ShoeId::A, XPlus, 1, ShoeId::B, XMinus, North )
        .decl().name().str() == "cfg0_c_42A+X_43B-X_N" );
    REQUIRE( smt3.connection( 1, ShoeId::B, XMinus, 0, ShoeId::A, XPlus, North )
        .decl().name().str() == "cfg0_c_42A+X_43B-X_N" );

    REQUIRE( smt3.connection( 1, ShoeId::A, XPlus, 2, ShoeId::B, ZMinus, North )
        .decl().name().str() == "cfg0_c_43A+X_44B-Z_N" );
    REQUIRE( smt3.connection( 2, ShoeId::B, ZMinus, 1, ShoeId::A, XPlus, North )
        .decl().name().str() == "cfg0_c_43A+X_44B-Z_N" );

    REQUIRE( smt3.connection( 0, ShoeId::A, XPlus, 2, ShoeId::B, ZMinus, North )
        .decl().name().str() == "cfg0_c_42A+X_44B-Z_N" );
    REQUIRE( smt3.connection( 2, ShoeId::B, ZMinus, 0, ShoeId::A, XPlus, North )
        .decl().name().str() == "cfg0_c_42A+X_44B-Z_N" );
}