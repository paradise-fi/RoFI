#include <catch2/catch.hpp>
#include <smtReconfig.hpp>

using namespace rofi::smtr;

TEST_CASE("SmtConfiguration building") {
    Configuration rofiCfg;
    Context c;

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

TEST_CASE( "phiNoIntersect" ) {
    Context ctx;
    Configuration rofiCfg;
    rofiCfg.addModule( 0, 0, 0, 42 );
    rofiCfg.addModule( 0, 0, 0, 43 );
    auto smtCfg = buildConfiguration( ctx, rofiCfg, 0 );

    SECTION( "No intersection" ) {
        z3::solver s( ctx.ctx );
        s.add( phiNoIntersect( ctx, smtCfg ) );

        // Place shoes in space
        auto msA = smtCfg.modules[ 0 ].shoes[ ShoeId::A ];
        auto msB = smtCfg.modules[ 0 ].shoes[ ShoeId::B ];
        s.add( msA.x == ctx.ctx.real_val( 0 ) );
        s.add( msA.y == ctx.ctx.real_val( 0 ) );
        s.add( msA.z == ctx.ctx.real_val( 0 ) );
        s.add( msB.x == ctx.ctx.real_val( 1 ) );
        s.add( msB.y == ctx.ctx.real_val( 0 ) );
        s.add( msB.z == ctx.ctx.real_val( 0 ) );

        auto nsA = smtCfg.modules[ 1 ].shoes[ ShoeId::A ];
        auto nsB = smtCfg.modules[ 1 ].shoes[ ShoeId::B ];
        s.add( nsA.x == ctx.ctx.real_val( 0 ) );
        s.add( nsA.y == ctx.ctx.real_val( 1 ) );
        s.add( nsA.z == ctx.ctx.real_val( 0 ) );
        s.add( nsB.x == ctx.ctx.real_val( 1 ) );
        s.add( nsB.y == ctx.ctx.real_val( 1 ) );
        s.add( nsB.z == ctx.ctx.real_val( 0 ) );

        REQUIRE( s.check() == z3::sat );
    }

    SECTION( "Intersection" ) {
        z3::solver s( ctx.ctx );
        s.add( phiNoIntersect( ctx, smtCfg ) );

        // Place shoes in space
        auto msA = smtCfg.modules[ 0 ].shoes[ ShoeId::A ];
        auto msB = smtCfg.modules[ 0 ].shoes[ ShoeId::B ];
        s.add( msA.x == ctx.ctx.real_val( 0 ) );
        s.add( msA.y == ctx.ctx.real_val( 0 ) );
        s.add( msA.z == ctx.ctx.real_val( 0 ) );
        s.add( msB.x == ctx.ctx.real_val( 1 ) );
        s.add( msB.y == ctx.ctx.real_val( 0 ) );
        s.add( msB.z == ctx.ctx.real_val( 0 ) );

        auto nsA = smtCfg.modules[ 1 ].shoes[ ShoeId::A ];
        auto nsB = smtCfg.modules[ 1 ].shoes[ ShoeId::B ];
        s.add( nsA.x == ctx.ctx.real_val( 0 ) );
        s.add( nsA.y == ctx.ctx.real_val( 1, 2 ) );
        s.add( nsA.z == ctx.ctx.real_val( 0 ) );
        s.add( nsB.x == ctx.ctx.real_val( 1 ) );
        s.add( nsB.y == ctx.ctx.real_val( 1, 2 ) );
        s.add( nsB.z == ctx.ctx.real_val( 0 ) );

        REQUIRE( s.check() == z3::unsat );
    }
}

TEST_CASE( "Shoe consistency" ) {
    Context ctx;
    Configuration rofiCfg;
    SECTION( "Default position" ) {
        rofiCfg.addModule( 0, 0, 0, 0 );
        auto smtCfg = buildConfiguration( ctx, rofiCfg, 0 );
        z3::solver s( ctx.ctx );

        s.add( phiShoeConsistent( ctx, smtCfg ) );
        s.add( phiSinCos( ctx, smtCfg ) );
        s.add( ctx.constraints() );
        auto module = smtCfg.modules[ 0 ];
        auto sA = module.shoes[ ShoeId::A ];
        auto sB = module.shoes[ ShoeId::B ];

        s.add( sA.x == 0 && sA.y == 0 && sA.z == 0 );
        s.add( sA.qa == 1 && sA.qb == 0 && sA.qc == 0 && sA.qd == 0 );
        s.add( sB.x == 0 && sB.y == 0 && sB.z == 1 );
        s.add( sB.qa == 0 && sB.qb == 0 && sB.qc == 1 && sB.qd == 0 );

        s.add( module.alpha.sin == 0 && module.alpha.sinhalf == 0 );
        s.add( module.alpha.cos == 1 && module.alpha.coshalf == 1 );

        s.add( module.beta.sin == 0 && module.beta.sinhalf == 0 );
        s.add( module.beta.cos == 1 && module.beta.coshalf == 1 );

        s.add( module.gamma.sin == 0 && module.gamma.sinhalf == 0 );
        s.add( module.gamma.cos == 1 && module.gamma.coshalf == 1 );

        REQUIRE( s.check() == z3::sat );
    }

    SECTION( "Invalid position" ) {
        rofiCfg.addModule( 0, 0, 0, 0 );
        auto smtCfg = buildConfiguration( ctx, rofiCfg, 0 );
        z3::solver s( ctx.ctx );

        s.add( phiShoeConsistent( ctx, smtCfg ) );
        s.add( phiSinCos( ctx, smtCfg ) );
        s.add( ctx.constraints() );
        auto module = smtCfg.modules[ 0 ];
        auto sA = module.shoes[ ShoeId::A ];
        auto sB = module.shoes[ ShoeId::B ];

        s.add( sA.x == 0 && sA.y == 0 && sA.z == 0 );
        s.add( sA.qa == 1 && sA.qb == 0 && sA.qc == 0 && sA.qd == 0 );
        s.add( sB.x == 0 && sB.y == 0 && sB.z == 1 );
        s.add( sB.qa == 1 && sB.qb == 0 && sB.qc == 0 && sB.qd == 0 );

        s.add( module.alpha.sin == 0 && module.alpha.sinhalf == 0 );
        s.add( module.alpha.cos == 1 && module.alpha.coshalf == 1 );

        s.add( module.beta.sin == 0 && module.beta.sinhalf == 0 );
        s.add( module.beta.cos == 1 && module.beta.coshalf == 1 );

        s.add( module.gamma.sin == 0 && module.gamma.sinhalf == 0 );
        s.add( module.gamma.cos == 1 && module.gamma.coshalf == 1 );

        REQUIRE( s.check() == z3::unsat );
    }
}

TEST_CASE( "Connector consistency" ) {
    Context ctx;
    Configuration rofiCfg;
    rofiCfg.addModule( 0, 0, 0, 42 );
    rofiCfg.addModule( 0, 0, 0, 43 );
    SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );
    SECTION( "X- to Z-" ) {
        z3::solver s( ctx.ctx );
        s.add( ctx.constraints() );
        s.add( phiSinCos( ctx, smtCfg ) );
        s.add( phiConnectorConsistent( ctx, smtCfg ) );
        s.add( smtCfg.connection( 0, ShoeId::A, XMinus, 1, ShoeId::A, ZMinus, North ) );

        auto sA = smtCfg.modules[ 0 ].shoes[ ShoeId::A ];
        s.add( sA.x == 0 && sA.y == 0 && sA.z == 0 );
        s.add( sA.qa == 1 && sA.qb == 0 && sA.qc == 0 && sA.qd == 0 );

        auto sB = smtCfg.modules[ 1 ].shoes[ ShoeId::B ];
        s.add( sB.x == 1 && sB.y == 0 && sB.z == 0 );
        s.add( sB.qa == - ctx.sqrt2 / 2 && sB.qb == 0 &&
            sB.qc == -ctx.sqrt2 / 2 && sB.qd == 0 );

        REQUIRE( s.check() == z3::sat );
    }
}

TEST_CASE( "phiIs Connected" ) {
    Context ctx;

    SECTION( "Disconnected 1" ) {
        Configuration rofiCfg;
        rofiCfg.addModule( 0, 0, 0, 42 );
        rofiCfg.addModule( 0, 0, 0, 43 );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );

        z3::solver s ( ctx.ctx );
        s.add( ctx.constraints() );
        s.add( phiIsConnected( ctx, smtCfg ) );
        s.add( phiEqual( ctx, smtCfg, rofiCfg ).simplify() );

        auto res = s.check();
        // CAPTURE( s.to_smt2() );
        // CAPTURE( s.get_model() );
        REQUIRE( res == z3::unsat );
    }

    SECTION( "Disconnected 2" ) {
        Configuration rofiCfg;
        rofiCfg.addModule( 0, 0, 0, 42 );
        rofiCfg.addModule( 0, 0, 0, 43 );
        rofiCfg.addModule( 0, 0, 0, 44 );
        rofiCfg.addEdge( { 42, A, XPlus, North, XPlus, A, 43 } );
        rofiCfg.addEdge( { 42, B, XPlus, North, XPlus, B, 43 } );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );

        z3::solver s ( ctx.ctx );
        s.add( ctx.constraints() );
        s.add( phiIsConnected( ctx, smtCfg ) );
        s.add( phiEqual( ctx, smtCfg, rofiCfg ) );

        auto res = s.check();
        // CAPTURE( s.get_model() );
        REQUIRE( res == z3::unsat );
    }

    SECTION( "Connected 1" ) {
        Configuration rofiCfg;
        rofiCfg.addModule( 0, 0, 0, 42 );
        rofiCfg.addModule( 0, 0, 0, 43 );
        rofiCfg.addEdge( { 42, A, XPlus, North, XPlus, A, 43 } );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );

        z3::solver s ( ctx.ctx );
        s.add( ctx.constraints() );
        s.add( phiIsConnected( ctx, smtCfg ) );
        s.add( phiEqual( ctx, smtCfg, rofiCfg ).simplify() );

        // CAPTURE( s.to_smt2() );
        REQUIRE( s.check() == z3::sat );
    }

    SECTION( "Connected 2 " ) {
        Configuration rofiCfg;
        rofiCfg.addModule( 0, 0, 0, 42 );
        rofiCfg.addModule( 0, 0, 0, 43 );
        rofiCfg.addModule( 0, 0, 0, 44 );
        rofiCfg.addEdge( { 42, A, XPlus, North, XPlus, A, 43 } );
        rofiCfg.addEdge( { 43, B, XPlus, North, XPlus, B, 44 } );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );

        z3::solver s ( ctx.ctx );
        s.add( ctx.constraints() );
        s.add( phiIsConnected( ctx, smtCfg ) );
        s.add( phiEqual( ctx, smtCfg, rofiCfg ) );

        REQUIRE( s.check() == z3::sat );
    }
}