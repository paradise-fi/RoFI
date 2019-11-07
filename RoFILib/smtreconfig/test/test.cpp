#include <catch2/catch.hpp>
#include <smtReconfig.hpp>
#include <fmt/format.h>

using namespace rofi::smtr;

std::string captureShoe( const std::string& name, const z3::model& m,
    const Shoe&  s )
{
    std::string pos = fmt::format("{}_pos = Matrix([[{}], [{}], [{}], [1]])",
        name,
        m.eval( s.x ).get_decimal_string( 5 ),
        m.eval( s.y ).get_decimal_string( 5 ),
        m.eval( s.z ).get_decimal_string( 5 )
    );

    std::string rot = fmt::format("{}_rot = Quaternion( {}, {}, {}, {} )",
        name,
        m.eval( s.qa ).get_decimal_string( 5 ),
        m.eval( s.qb ).get_decimal_string( 5 ),
        m.eval( s.qc ).get_decimal_string( 5 ),
        m.eval( s.qd ).get_decimal_string( 5 )
    );
    return pos + "\n" + rot;
}

void dumpFormula( const std::string& name, z3::solver& s ) {
    std::ofstream f( "/tmp/" + name + ".smt2" );
    f << s.to_smt2() << "\n";
}

void dumpText( const std::string& name, const std::string& what ) {
    std::ofstream f( "/tmp/" + name + ".txt" );
    f << what << "\n";
}

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

TEST_CASE( "Connector & shoe consistency" ) {
    Context ctx;
    Configuration rofiCfg;
    rofiCfg.addModule( 0, 0, 0, 41 );
    rofiCfg.addModule( 0, 0, 0, 42 );

    SECTION("Single connection from A") {
        rofiCfg.addEdge( { 41, A, XPlus, South, XMinus, A, 42 } );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );
        SECTION( "Connected" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );

            auto res = s.check();
            REQUIRE( res == z3::sat );
        }

        SECTION( "Connected + shoe consistent" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );
            s.add( phiShoeConsistent( ctx, smtCfg ) );

            auto res = s.check();
            REQUIRE( res == z3::sat );
        }

        SECTION( "Connected + shoe consistent + connector consistent" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );
            s.add( phiShoeConsistent( ctx, smtCfg ) );
            s.add( phiConnectorConsistent( ctx, smtCfg ) );

            dumpFormula( "CS-Consistency-SingleA", s );

            auto res = s.check();
            REQUIRE( res == z3::sat );

            z3::model m = s.get_model();

            const auto& shoeA0 = smtCfg.modules[ 0 ].shoes[ A ];
            const auto& shoeB0 = smtCfg.modules[ 0 ].shoes[ B ];
            const auto& shoeA1 = smtCfg.modules[ 1 ].shoes[ A ];
            const auto& shoeB1 = smtCfg.modules[ 1 ].shoes[ B ];

            std::string out =
                captureShoe( "A0", m, shoeA0 ) + "\n" +
                captureShoe( "B0", m, shoeB0 ) + "\n" +
                captureShoe( "A1", m, shoeA1 ) + "\n" +
                captureShoe( "B1", m, shoeB1 );
            CAPTURE( out );
            dumpText( "CS-Consistency-SingleA", out );

            CHECK( m.eval( shoeA0.x ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.z ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qa ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeA0.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qc ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeB0.x ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.z ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB0.qa ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.qc ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB0.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeA1.x ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeA1.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.z ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qa ).get_decimal_string( 1 ) == "-1" );
            CHECK( m.eval( shoeA1.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qc ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeB1.x ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB1.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.z ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB1.qa ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.qc ).get_decimal_string( 1 ) == "-1" );
            CHECK( m.eval( shoeB1.qd ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( smtCfg.connection( 0, A, XPlus, 1, A, XMinus, South ) ).bool_value() );
        }
    }

    SECTION("Single connection from B") {
        rofiCfg.addEdge( { 41, B, XMinus, South, XPlus, B, 42 } );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );
        SECTION( "Connected" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );

            auto res = s.check();
            REQUIRE( res == z3::sat );
        }

        SECTION( "Connected + shoe consistent" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );
            s.add( phiShoeConsistent( ctx, smtCfg ) );

            auto res = s.check();
            REQUIRE( res == z3::sat );
        }

        SECTION( "Connected + shoe consistent + connector consistent" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );
            s.add( phiShoeConsistent( ctx, smtCfg ) );
            s.add( phiConnectorConsistent( ctx, smtCfg ) );

            dumpFormula( "CS-Consistency-SingleB", s );

            auto res = s.check();
            REQUIRE( res == z3::sat );

            z3::model m = s.get_model();

            const auto& shoeA0 = smtCfg.modules[ 0 ].shoes[ A ];
            const auto& shoeB0 = smtCfg.modules[ 0 ].shoes[ B ];
            const auto& shoeA1 = smtCfg.modules[ 1 ].shoes[ A ];
            const auto& shoeB1 = smtCfg.modules[ 1 ].shoes[ B ];

            std::string out =
                captureShoe( "A0", m, shoeA0 ) + "\n" +
                captureShoe( "B0", m, shoeB0 ) + "\n" +
                captureShoe( "A1", m, shoeA1 ) + "\n" +
                captureShoe( "B1", m, shoeB1 );
            CAPTURE( out );
            dumpText( "CS-Consistency-SingleB", out );

            CHECK( m.eval( shoeA0.x ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.z ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qa ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeA0.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qc ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeB0.x ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.z ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB0.qa ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.qc ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB0.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeA1.x ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeA1.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.z ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qa ).get_decimal_string( 1 ) == "-1" );
            CHECK( m.eval( shoeA1.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qc ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeB1.x ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB1.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.z ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB1.qa ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.qc ).get_decimal_string( 1 ) == "-1" );
            CHECK( m.eval( shoeB1.qd ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( smtCfg.connection( 0, A, XPlus, 1, A, XMinus, South ) ).bool_value() );
        }
    }

    SECTION("Double connection") {
        rofiCfg.addEdge( { 41, A, XPlus, South, XMinus, A, 42 } );
        rofiCfg.addEdge( { 41, B, XMinus, South, XPlus, B, 42 } );
        SmtConfiguration smtCfg = buildConfiguration( ctx, rofiCfg, 0 );
        SECTION( "Connected" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );

            auto res = s.check();
            REQUIRE( res == z3::sat );
        }

        SECTION( "Connected + shoe consistent" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );
            s.add( phiShoeConsistent( ctx, smtCfg ) );

            auto res = s.check();
            REQUIRE( res == z3::sat );
        }

        SECTION( "Connected + shoe consistent + connector consistent" ) {
            z3::solver s( ctx.ctx );
            s.add( ctx.constraints() );
            s.add( phiRootModule( ctx, smtCfg, 0 ) );
            s.add( phiEqual( ctx, smtCfg, rofiCfg ) );
            s.add( phiIsConnected( ctx, smtCfg ) );
            s.add( phiShoeConsistent( ctx, smtCfg ) );
            s.add( phiConnectorConsistent( ctx, smtCfg ) );


            dumpFormula( "CS-Consistency-Double", s );
            auto res = s.check();
            REQUIRE( res == z3::sat );

            z3::model m = s.get_model();

            const auto& shoeA0 = smtCfg.modules[ 0 ].shoes[ A ];
            const auto& shoeB0 = smtCfg.modules[ 0 ].shoes[ B ];
            const auto& shoeA1 = smtCfg.modules[ 1 ].shoes[ A ];
            const auto& shoeB1 = smtCfg.modules[ 1 ].shoes[ B ];

            std::string out =
                captureShoe( "A0", m, shoeA0 ) + "\n" +
                captureShoe( "B0", m, shoeB0 ) + "\n" +
                captureShoe( "A1", m, shoeA1 ) + "\n" +
                captureShoe( "B1", m, shoeB1 );
            CAPTURE( out );
            dumpText( "CS-Consistency-Double", out );

            CHECK( m.eval( shoeA0.x ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.z ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qa ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeA0.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qc ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA0.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeB0.x ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.z ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB0.qa ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB0.qc ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB0.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeA1.x ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeA1.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.z ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qa ).get_decimal_string( 1 ) == "-1" );
            CHECK( m.eval( shoeA1.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qc ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeA1.qd ).get_decimal_string( 1 ) == "0" );

            CHECK( m.eval( shoeB1.x ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB1.y ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.z ).get_decimal_string( 1 ) == "1" );
            CHECK( m.eval( shoeB1.qa ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.qb ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( shoeB1.qc ).get_decimal_string( 1 ) == "-1" );
            CHECK( m.eval( shoeB1.qd ).get_decimal_string( 1 ) == "0" );
            CHECK( m.eval( smtCfg.connection( 0, A, XPlus, 1, A, XMinus, South ) ).bool_value() );
        }
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