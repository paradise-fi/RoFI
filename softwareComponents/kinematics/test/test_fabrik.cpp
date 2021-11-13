#include <catch2/catch.hpp>

#include "fabrik.hpp"

/* Util */


std::ostream& operator<<( std::ostream& o, Configuration config ){
    o << IO::toString( config );
    return o;
}

TEST_CASE( "Failed inputs" ){
    CHECK_THROWS( tentacleMonster( "invalid" ) );
}

/* Config paths */

const std::string root = std::getenv( "ROFI_ROOT" );
const std::string single_path = root + "/data/configurations/kinematics/one_module.rofi";
const std::string doubleZZ = root + "/data/configurations/kinematics/2zz.rofi";
const std::string doubleZZN = root + "/data/configurations/kinematics/2zzN.rofi";
const std::string doubleZZE = root + "/data/configurations/kinematics/2zzE.rofi";
const std::string doubleZZW = root + "/data/configurations/kinematics/2zzW.rofi";

const std::string doubleZXS = root + "/data/configurations/kinematics/2zxS.rofi";


/* Loading configurations */

TEST_CASE( "single module" ){
    tentacleMonster one( single_path );
    REQUIRE( one.tentacles.size() == 1 );
    REQUIRE( one.tentacles.front().size() == 2 );
    CHECK( !one.tentacles.front().front().nextEdge );
    CHECK( !one.tentacles.front().back().nextEdge );
}

TEST_CASE( "ZtoZ" ){
    tentacleMonster z( doubleZZ );
    REQUIRE( z.tentacles.size() == 1 );
    REQUIRE( z.tentacles.front().size() == 4 );
    CHECK(  z.tentacles.front()[ 1 ].nextEdge );
    CHECK( !z.tentacles.front()[ 0 ].nextEdge );
    CHECK( !z.tentacles.front()[ 2 ].nextEdge );
    CHECK( !z.tentacles.front()[ 3 ].nextEdge );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().id1() == 1 );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().id2() == 2 );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().dock1() == ZMinus );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().dock2() == ZMinus );
}

TEST_CASE( "ZtoX" ){
    tentacleMonster z( doubleZXS );
    REQUIRE( z.tentacles.size() == 1 );
    REQUIRE( z.tentacles.front().size() == 4 );
    CHECK( !z.tentacles.front()[ 0 ].nextEdge );
    CHECK(  z.tentacles.front()[ 1 ].nextEdge );
    CHECK( !z.tentacles.front()[ 2 ].nextEdge );
    CHECK( !z.tentacles.front()[ 3 ].nextEdge );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().id1() == 1 );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().id2() == 2 );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().dock1() == ZMinus );
    CHECK( z.tentacles.front()[ 1 ].nextEdge.value().dock2() == XPlus );
}

void reach( tentacleMonster& monster, double x = 0.0, double y = 0.0, double z = 0.0,
            double xRot = 0.0, double yRot = 0.0, double zRot = 0.0, int arm = 0 )
{
    CHECK( monster.reach( arm, Vector{ x, y, z, 1.0 }, xRot, yRot, zRot ) );
    Matrix target =
        translate( x * X ) *
        translate( y * Y ) *
        translate( z * Z ) *
        rotate( zRot, Z ) *
        rotate( yRot, Y ) *
        rotate( xRot, X );

    CHECK( equals( monster.tentacles.front().front().trans, identity ) );
    CHECK( equals( monster.tentacles.front().back().trans, target ) );
};

/* Single module tests */
const Vector zero = Vector{ 0.0, 0.0, 0.0, 1.0 };

TEST_CASE( "single 0 0 1 0 0 0" ){
    tentacleMonster one( single_path );

    reach( one, 0.0, 0.0, 1.0 );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.computeMatrices();
    CHECK( one.config == test );
}

TEST_CASE( "single 0 0 1 0 0 90" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, Vector{ 0.0, 0.0, 1.0, 1.0 }, 0.0, 0.0, M_PI_2 ) );
    Matrix target =
        rotate( M_PI_2, Z ) *
        translate( Z ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Gamma, 90.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}


TEST_CASE( "single 0 0 1 0 0 -90" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, Vector{ 0.0, 0.0, 1.0, 1.0 }, 0.0, 0.0, -M_PI_2 ) );
    Matrix target =
        rotate( -M_PI_2, Z ) *
        translate( Z ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Gamma, -90.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}


TEST_CASE( "single 0 0 1 0 0 180" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, Vector{ 0.0, 0.0, 1.0, 1.0 }, 0.0, 0.0, M_PI ) );
    Matrix target =
        translate( Z ) *
        rotate( M_PI, Z ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Gamma, 180.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}

TEST_CASE( "single 0 0 1 45 0 0" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, Vector{ 0.0, 0.0, 1.0, 1.0 }, M_PI_4 ) );
    Matrix target =
        translate( Z ) *
        rotate( M_PI_4, X ) *
        identity;


    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Beta, 45.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}

TEST_CASE( "single 0 1 0 0 0 0" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, zero + Y ) );
    Matrix target =
        translate( Y ) *
        identity;
    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, 90.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}

TEST_CASE( "single 0 1 0 -45 0 0" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, zero + Y, -M_PI_4 ) );
    Matrix target =
        translate( Y ) *
        rotate( -M_PI_4, X ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, 45.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}


TEST_CASE( "single 0 1 0 -90 0 0" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, zero + Y, -M_PI_2 ) );
    Matrix target =
        translate( Y ) *
        rotate( -M_PI_2, X ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -90.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}


TEST_CASE( "single 0 1 0 180 0 0" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, zero + Y, M_PI ) );
    Matrix target =
        translate( Y ) *
        rotate( M_PI, X ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}


TEST_CASE( "single 0 1 0 -90 0 90" ){
    tentacleMonster one( single_path );
    CHECK( one.reach( 0, zero + Y, -M_PI_2, M_PI_2, 0.0 ) );
    Matrix target =
        translate( Y ) *
        rotate( M_PI_2, Y ) *
        rotate( -M_PI_2, X ) *
        identity;

    CHECK( equals( one.tentacles.front().front().trans, identity ) );
    CHECK( equals( one.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( single_path );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Gamma, 90.0 ) ) );
    test.computeMatrices();
    CHECK( one.config == test );
}

/* Two module arms */

TEST_CASE( "2zzS 0 0 3 0 0 0" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, zero + 3 * Z ) );
    Matrix target =
        translate( 3 * Z ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );
}


TEST_CASE( "2zzS 0 1 2 0 0 0" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, Vector{ 0.0, 1.0, 2.0, 1.0 } ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 * Y ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );


    Configuration test;
    std::ifstream input( doubleZZ );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Gamma, 180.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Beta, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, 180.0 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}


TEST_CASE( "2zzS 0 1 2 -90 0 0" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, Vector{ 0.0, 1.0, 2.0, 1.0 }, -M_PI_2 ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 * Y ) *
        rotate( -M_PI_2, X ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );


    Configuration test;
    std::ifstream input( doubleZZ );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Gamma, 180.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, 180.0 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}


TEST_CASE( "2zzS 0 2 1 -90 0 0" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, Vector{ 0.0, 2.0, 1.0, 1.0 }, -M_PI_2 ) );
    Matrix target =
        translate( 1 * Z ) *
        translate( 2 * Y ) *
        rotate( -M_PI_2, X ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );


    Configuration test;
    std::ifstream input( doubleZZ );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -10.4805 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -54.3286 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, -49.3147 ) ) );
    test.execute( Action( Action::Rotate( 2, Beta, 24.1238 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}


TEST_CASE( "2zzS 1 0 2 0 0 90" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, Vector{ 1.0, 0.0, 2.0, 1.0 }, 0.0, 0.0, M_PI_2 ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 * X ) *
        rotate( M_PI_2, Z ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZ );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Gamma, 90.0 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}

TEST_CASE( "2zzS 2 0 1 0 0 90" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, Vector{ 2.0, 0.0, 1.0, 1.0 }, 0.0, 0.0, M_PI_2 ) );
    Matrix target =
        translate( 1 * Z ) *
        translate( 2 * X ) *
        rotate( M_PI_2, Z ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZ );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Gamma, 90.0 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}


TEST_CASE( "2zzS 0 sqrt2 sqrt2 45 0 0" ){
    tentacleMonster two( doubleZZ );
    CHECK( two.reach( 0, Vector{ 0.0, 1/sqrt( 2.0 ), 1/sqrt( 2.0 ), 1.0 }, -M_PI_4, 0.0, 0.0 ) );
    Matrix target =
        translate( 1.0 / sqrt( 2.0 ) * Z ) *
        translate( 1.0 / sqrt( 2.0 ) * Y ) *
        rotate( -M_PI_4, X ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZ );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90 ) ) );
    test.execute( Action( Action::Rotate( 1, Alpha, 45 ) ) );

    test.computeMatrices();

    CHECK( two.config == test );
}



TEST_CASE( "2zzN 0 1 2 0 0 0" ){
    tentacleMonster two( doubleZZN );
    CHECK( two.reach( 0, Vector{ 0.0, 1.0, 2.0, 1.0 } ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 * Y ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );


    Configuration test;
    std::ifstream input( doubleZZN );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Alpha, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Beta, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Gamma, 180.0 ) ) ); // ?

    test.computeMatrices();
    CHECK( two.config == test );
}



TEST_CASE( "2zzN 1/sqrt2 1/sqrt2 2 0 0 0" ){
    tentacleMonster two( doubleZZN );
    CHECK( two.reach( 0, Vector{ 1 / sqrt( 2.0 ), 1 / sqrt( 2.0 ), 2.0, 1.0 } ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 / sqrt( 2.0 ) * Y ) *
        translate( 1 / sqrt( 2.0 ) * X ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZN );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Alpha, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, 45 ) ) );
    test.execute( Action( Action::Rotate( 1, Gamma, 135 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}

TEST_CASE( "2zzE 0 1 2 0 0 0" ){
    tentacleMonster two( doubleZZE );
    CHECK( two.reach( 0, Vector{ 0.0, 1.0, 2.0, 1.0 } ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 * Y ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZE );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, 53.1301 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Alpha, 36.8699 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90.0 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}

TEST_CASE( "2zzE 0 2 1 0 0 0" ){
    tentacleMonster two( doubleZZE );
    CHECK( two.reach( 0, Vector{ 0.0, 2.0, 1.0, 1.0 } ) );
    Matrix target =
        translate( 1 * Z ) *
        translate( 2 * Y ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZE );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, -90.0 ) ) );
    //est.execute( Action( Action::Rotate( 1, Alpha, -0.00318348 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}

/* TODO: fix this?
   The computation doesn't consider having to bend when the direction is
   right but the distance is lower than the shoulder length.
   Might be too specific of an edge case and work without it?
TEST_CASE( "2zzE sqrt2 sqrt2 1 45 0 0" ){
    tentacleMonster two( doubleZZE );
    //two.debug = true;
    CHECK( two.reach( 0, Vector{ 1.0, 0.0, sqrt( 2.0 ), 1.0 }, 0.0, 0.0, M_PI_2 ) );
    Matrix target =
        translate( 1.0  * X ) *
        //translate( 1.0  * Y ) *
        translate( sqrt( 2.0 ) * Z ) *
        rotate( -M_PI_2, Z ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZE );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Gamma, 45 ) ) );
    test.execute( Action( Action::Rotate( 2, Beta, -90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90 ) ) );
    test.execute( Action( Action::Rotate( 1, Alpha, 45 ) ) );

    test.computeMatrices();

    CHECK( two.config == test );
}*/

TEST_CASE( "2zzW 0 1 2 0 0 0" ){
    tentacleMonster two( doubleZZW );
    CHECK( two.reach( 0, Vector{ 0.0, 1.0, 2.0, 1.0 } ) );
    Matrix target =
        translate( 2 * Z ) *
        translate( 1 * Y ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZW );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, 53.1301 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Alpha, 36.8699 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90.0 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}

TEST_CASE( "2zzW 0 2 1 0 0 0" ){
    tentacleMonster two( doubleZZW );
    CHECK( two.reach( 0, Vector{ 0.0, 2.0, 1.0, 1.0 } ) );
    Matrix target =
        translate( 1 * Z ) *
        translate( 2 * Y ) *
        identity;

    CHECK( equals( two.tentacles.front().front().trans, identity ) );
    CHECK( equals( two.tentacles.front().back().trans, target ) );

    Configuration test;
    std::ifstream input( doubleZZW );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 2, Beta, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, 90.0 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, -90 ) ) );

    test.computeMatrices();
    CHECK( two.config == test );
}


/* Z to X connection */
TEST_CASE( "2zxS -1 0 2 0 0 0" ){
    tentacleMonster two( doubleZXS );
    reach( two, -1.0, 0.0, 2.0, 0.0, -M_PI_2, M_PI );

    Configuration test;
    std::ifstream input( doubleZXS );
    IO::readConfiguration( input, test );

    CHECK( two.config == test );
}

TEST_CASE( "2zxS 1 0 2 0 0 0" ){
    tentacleMonster two( doubleZXS );
    reach( two, 1.0, 0.0, 2.0, 0.0, 0.0, -M_PI_2 );

    Configuration test;
    std::ifstream input( doubleZXS );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Gamma, 180 ) ) );
    test.execute( Action( Action::Rotate( 2, Beta, 90 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, -90 ) ) );
    CHECK( two.config == test );
}

TEST_CASE( "2zxS 0 0 sqrt5 0 0 0" ){
    tentacleMonster two( doubleZXS );
    //two.debug = true;
    reach( two, 0.0, 0.0, sqrt( 5.0 ) );

    Configuration test;
    std::ifstream input( doubleZXS );
    IO::readConfiguration( input, test );
    test.execute( Action( Action::Rotate( 1, Alpha, -26.565 ) ) );
    test.execute( Action( Action::Rotate( 1, Beta, 0.000184725 ) ) );
    test.execute( Action( Action::Rotate( 1, Gamma, 90 ) ) );
    test.execute( Action( Action::Rotate( 2, Alpha, 0.000184725 ) ) );
    test.execute( Action( Action::Rotate( 2, Beta, -63.435 ) ) );
    test.execute( Action( Action::Rotate( 2, Gamma, 90.0003 ) ) );
    CHECK( two.config == test );
}
