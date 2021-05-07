#include <catch2/catch.hpp>

#include <joints.h>
#include <iostream>

void printMatrix( Matrix m ) {
    for ( int i = 0; i < m.n_rows; i++ ) {
        for ( int j = 0; j < m.n_cols; j++ ) {
            std::cout << round( m(i, j) ) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

TEST_CASE( "Base RigidJoint" ) {
    // ToDo: Write the test
    auto j = rofi::RigidJoint( rotate( M_PI_2, { 1, 0, 0, 1 } ) * translate( { 20, 0, 0 } ) );
    CHECK( equals( j.sourceToDest(), { { 1, 0,  0, 20 }
                                     , { 0, 0, -1,  0 }
                                     , { 0, 1,  0,  0 }
                                     , { 0, 0,  0,  1 } } ) );
    CHECK( equals( j.sourceToDest() * j.destToSource(), identity ) );
}

TEST_CASE( "Base RotationJoint" ) {
    // ToDo: Write the test
    // Vector sourceOrigin, Vector sourceAxis, Vector destOrigin, Vector desAxis, double min, double max
    auto j = rofi::RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, -M_PI_2, M_PI_2 );
    std::cout << "joint created is " << j << "\n";
    j.positions = { 0 * M_PI }; // set the angle of the rotation
    CHECK( equals( j.sourceToDest(), identity ) );
    CHECK( equals( j.destToSource(), identity ) );
}

TEST_CASE( "sourceToDest and destToSource" ) {
    SECTION( "Unit size" ) {
        auto j = rofi::RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 1, 0, 0 }, { 1, 0, 0 }, -M_PI_2, M_PI_2 );
        j.positions = { 0 * M_PI };
        CHECK( equals( j.destToSource(), j.sourceToDest() ) );
        j.positions = { 2 * M_PI };
        CHECK( equals( j.destToSource(), j.sourceToDest() ) );
        j.positions = { M_PI_2 };
        CHECK( equals( j.destToSource() * j.sourceToDest(), identity ) );
        CHECK_FALSE( equals( j.destToSource(), j.sourceToDest() ) );
    }

    SECTION( "Asymetric" ) {
        auto j = rofi::RotationJoint( { 0, 0, 0 }, { 0, 0, 0 }, { 5, 0, 0 }, { 4, 0, 0 }, -M_PI_2, M_PI_2 );
        j.positions = { 0 * M_PI };
        CHECK( equals( j.destToSource(), j.sourceToDest() ) );
        j.positions = { 2 * M_PI };
        CHECK( equals( j.destToSource(), j.sourceToDest() ) );
        j.positions = { M_PI_2 };
        CHECK( equals( j.destToSource() * j.sourceToDest(), identity ) );
        CHECK_FALSE( equals( j.destToSource(), j.sourceToDest() ) );
    }
}
