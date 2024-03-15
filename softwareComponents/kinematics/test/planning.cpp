#include <catch2/catch.hpp>

#include <chrono>

#include <atoms/cmdline_utils.hpp>
#include <configuration/serialization.hpp>
#include <parsing/parsing_lite.hpp>
#include <manipulator.hpp>


namespace {

using namespace rofi::configuration;
using namespace rofi::configuration::roficom;
using namespace rofi::configuration::matrices;
using namespace rofi::kinematics;

TEST_CASE( "Root copy in target" ){
    RofiWorld world;

    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );

    auto h = connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );

    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );
    
    Matrix target = m2.getConnector( "B-X" ).getPosition();
    auto manipulator = Manipulator( world );
 /*    std::vector< RoficomJoint > _joints;
    for( const auto& j : world.roficomConnections() ){
        _joints.push_back(j);*/
    //} 

    auto copy = manipulator.copy( target );
    REQUIRE( copy.world.prepare() );
    
    for( int id : { 0, 1 } ){
        for( size_t idx = 0; idx < world.getModule( id )->components().size(); idx++ ){
            REQUIRE( rofi::configuration::matrices::equals( world.getModule( id )->components()[ idx ].getPosition(),
                                                            copy.world.getModule( id )->components()[ idx ].getPosition() ) );
        }
    }

    target *= rotate( M_PI_2, X );
    copy = manipulator.copy( target );
    REQUIRE( copy.world.prepare() );

    for( int id : { 0, 1 } ){
        for( size_t idx = 0; idx < world.getModule( id )->components().size(); idx++ ){
            REQUIRE( equals( center( copy.world.getModule( id )->components()[ idx ].getPosition() )[ 0 ], 3 ) );
        }
    }
}

TEST_CASE( "Single find_angle" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
    REQUIRE( world.prepare() );


    RofiWorld other;
    auto& m2 = other.insert( UniversalModule( 0, 90_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m2.getConnector( "A-X" ), { 0, 0, 0 }, identity );
    REQUIRE( other.prepare() );

    auto manipulator = Manipulator( world );

    auto copy = manipulator.copy( m2.getConnector("B-X").getPosition() );
    //dynamic_cast<UniversalModule*>( copy.world.getModule( 0 ) )->setAlpha( 90_deg );

    auto [ p, a ] = copy.find_angles( 0, manipulator );
    //std::cout << p.deg() << '\n';
    REQUIRE( equals( p.deg(), 0.0 ) );
    REQUIRE( equals( a.deg(), 0.0 ) );

    auto [ p2, a2 ] = manipulator.find_angles( 0, copy );
    //std::cout << p2.deg() << "a: " << a2.deg() << '\n';
    REQUIRE( equals( p2.deg(), 90.0 ) );
    REQUIRE( equals( a2.deg(), 0.0 ) );
}

TEST_CASE( "Double find angle" ){

    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m4 = other.insert( UniversalModule( 1, 90_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m3.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m3.getConnector( "B-Z" ), m4.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( other.prepare() );

    Manipulator manipulator( world );
    auto copy = manipulator.copy( m4.getConnector("B-X").getPosition() );
    
    auto [ p, a ] = copy.find_angles( 0, manipulator );
    REQUIRE( equals( p.deg(), 0.0 ) );
    REQUIRE( equals( a.deg(), 0.0 ) );
    
    auto [ p2, a2 ] = copy.find_angles( 1, manipulator );
    //std::cout << "p: " << p2.deg() << " a: " << a2.deg() << '\n';
    REQUIRE( equals( p2.deg(), 90.0 ) );
    REQUIRE( equals( a2.deg(), 0.0 ) );
}

TEST_CASE( "Move joints along X axis" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m4 = other.insert( UniversalModule( 1, 90_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m3.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m3.getConnector( "B-Z" ), m4.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( other.prepare() );

    auto manipulator = Manipulator( world );
    auto copy = manipulator.copy( m4.getConnector("B-X").getPosition() );
    
    auto [ p, a ] = copy.find_angles( 0, manipulator );
    REQUIRE( equals( p.deg(), 0.0 ) );
    REQUIRE( equals( a.deg(), 0.0 ) );
    
    auto [ p2, a2 ] = copy.find_angles( 1, manipulator );
    REQUIRE( equals( p2.deg(), 90.0 ) );
    REQUIRE( equals( a2.deg(), 0.0 ) );
    copy.set_joint( 1, p2, a2 );

    auto [ p3, a3 ] = copy.find_angles( 2, manipulator );
    REQUIRE( equals( p3.deg(), 0.0 ) );
    REQUIRE( equals( a3.deg(), 0.0 ) );

    
    auto [ p4, a4 ] = manipulator.find_angles( 0, copy );
    REQUIRE( equals( p4.deg(), 0.0 ) );
    REQUIRE( equals( a4.deg(), 0.0 ) );
    
    auto [ p5, a5 ] = manipulator.find_angles( 1, copy );
    REQUIRE( equals( p5.deg(), 0.0 ) );
    REQUIRE( equals( a5.deg(), 0.0 ) );

    auto [ p6, a6 ] = manipulator.find_angles( 2, copy );
    REQUIRE( equals( p6.deg(), 90.0 ) );
    REQUIRE( equals( a6.deg(), 0.0 ) );
    manipulator.set_joint( 2, p6, a6 );

    REQUIRE( rofi::configuration::matrices::equals( manipulator.joint_position( manipulator.arm.size() - 1 ), m4.getConnector("B-X").getPosition() ) );
}

TEST_CASE( "Move joint along Z" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 90_deg ) );
    auto& m4 = other.insert( UniversalModule( 1, 90_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m3.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m3.getConnector( "B-Z" ), m4.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( other.prepare() );

    auto manipulator = Manipulator( world );
    auto copy = manipulator.copy( m4.getConnector("B-X").getPosition() );
    
    auto [ p, a ] = copy.find_angles( 0, manipulator );
    REQUIRE( equals( p.deg(), 0.0 ) );
    REQUIRE( equals( a.deg(), 0.0 ) );

    auto [ p2, a2 ] = copy.find_angles( 1, manipulator );
    REQUIRE( equals( p2.deg(), 90.0 ) );
    REQUIRE( equals( a2.deg(), 0.0 ) );
    copy.set_joint( 1, p2, a2 );

    auto [ p3, a3 ] = copy.find_angles( 2, manipulator );
    REQUIRE( equals( p3.deg(), 0.0 ) );
    REQUIRE( equals( a3.deg(), 0.0 ) );

    
    auto [ p4, a4 ] = manipulator.find_angles( 0, copy );
    REQUIRE( equals( p4.deg(), 0.0 ) );
    REQUIRE( equals( a4.deg(), 0.0 ) );

    auto [ p5, a5 ] = manipulator.find_angles( 1, copy );
    //std::cout << "p: " << p5.deg() << " a: " << a5.deg() << '\n';
    REQUIRE( equals( p5.deg(), 0.0 ) );
    REQUIRE( equals( a5.deg(), 90.0 ) );
    manipulator.set_joint( 1, p5, a5 );

    auto [ p6, a6 ] = manipulator.find_angles( 2, copy );
    REQUIRE( equals( p6.deg(), 90.0 ) );
    REQUIRE( equals( a6.deg(), 0.0 ) );
    manipulator.set_joint( 2, p6, a6 );

    REQUIRE( rofi::configuration::matrices::equals( manipulator.joint_position( manipulator.arm.size() - 1 ), m4.getConnector("B-X").getPosition() ) );
}

TEST_CASE( "45-deg angles" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 45_deg ) );
    auto& m4 = other.insert( UniversalModule( 1, 45_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m3.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m3.getConnector( "B-Z" ), m4.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( other.prepare() );

    auto manipulator = Manipulator( world );
    //manipulator.debug = true;
    auto copy = manipulator.copy( m4.getConnector("B-X").getPosition() );
    
    auto [ p, a ] = copy.find_angles( 0, manipulator );
    REQUIRE( equals( p.deg(), 0.0 ) );
    REQUIRE( equals( a.deg(), 0.0 ) );

    auto [ p2, a2 ] = copy.find_angles( 1, manipulator );
    REQUIRE( equals( p2.deg(), 45.0 ) );
    REQUIRE( equals( a2.deg(), 0.0 ) );
    copy.set_joint( 1, p2, a2 );

    auto [ p3, a3 ] = copy.find_angles( 2, manipulator );
    REQUIRE( equals( p3.deg(), 0.0 ) );
    REQUIRE( equals( a3.deg(), 0.0 ) );

    
    auto [ p4, a4 ] = manipulator.find_angles( 0, copy );
    REQUIRE( equals( p4.deg(), 0.0 ) );
    REQUIRE( equals( a4.deg(), 0.0 ) );

    auto [ p5, a5 ] = manipulator.find_angles( 1, copy );
    //std::cout << "p: " << p5.deg() << " a: " << a5.deg() << '\n';
    REQUIRE( equals( p5.deg(), 0.0 ) );
    REQUIRE( equals( a5.deg(), 45.0 ) );
    //Vector next = inverse( manipulator.joint_position( 1 ) ) * (-Z);
    //std::cout << "next: " << next;
    manipulator.set_joint( 1, p5, a5 );

    auto [ p6, a6 ] = manipulator.find_angles( 2, copy );
    REQUIRE( equals( p6.deg(), 45.0 ) );
    REQUIRE( equals( a6.deg(), 0.0 ) );
    manipulator.set_joint( 2, p6, a6 );

    //axis = cross_product( manipulator.joint_position( 2 ) * Z, manipulator.joint_position( 2 ) * rotate( a6.rad(), Z ) * rotate( p6.rad(), X ) * Z );
    //std::cout << "Axis:\n" << axis;

    REQUIRE( rofi::configuration::matrices::equals( manipulator.joint_position( manipulator.arm.size() - 1 ), m4.getConnector("B-X").getPosition() ) );
}

TEST_CASE( "Beta rotation" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m3 = other.insert( UniversalModule( 0, 0_deg, 20_deg, 0_deg ) );
    auto& m4 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m3.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m3.getConnector( "B-Z" ), m4.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( other.prepare() );

    auto manipulator = Manipulator( world );
    auto copy = manipulator.copy( m4.getConnector("B-X").getPosition() );

    REQUIRE( manipulator.fabrik( m4.getConnector("B-X").getPosition() ) );
}

TEST_CASE( "Random rotations on first module" ){
    std::srand( 42 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-Z" ), m5.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( other.prepare() );

        m4.setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
        m4.setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
        m4.setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        //manipulator.debug = true;
        //std::cout << "a: " << m4.getAlpha().deg() << " b: " << m4.getBeta().deg() << " c: " << m4.getGamma().deg() << '\n';
        REQUIRE( manipulator.fabrik( m5.getConnector( "B-X" ).getPosition() ) );
    }
}

TEST_CASE( "FABRIK on length 2" ){
    std::srand( 22 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        //auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
        //connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        //auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-Z" ), m5.getConnector( "A-Z" ), roficom::Orientation::North );
        //connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );
        //std::cout << serialization::toJSON( other );

        auto manipulator = Manipulator( world );
        //manipulator.debug = true;
        bool result = manipulator.fabrik( m5.getConnector( "B-X" ).getPosition() );
        REQUIRE( result );

        manipulator = Manipulator( world );
        if( !result ){
            manipulator.debug = true;
            manipulator.fabrik( m5.getConnector( "B-X" ).getPosition() );
        }
    }
}


TEST_CASE( "FABRIK on length 3" ){
    std::srand( 42 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-Z" ), m5.getConnector( "A-Z" ), roficom::Orientation::North );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "Final joint rotation" ){
    
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 10_deg, 10_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 40_deg, 70_deg, 20_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m3 = other.insert( UniversalModule( 0, 0_deg, 10_deg, 10_deg ) );
    auto& m4 = other.insert( UniversalModule( 1, 40_deg, 90_deg, -45_deg ) );
    connect< RigidJoint >( m3.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m3.getConnector( "B-Z" ), m4.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    auto manipulator = Manipulator( world );
    auto copy = manipulator.copy( m4.getConnector( "B-X" ).getPosition() );
    //copy.set_joint( 0, Angle::deg( 0.0 ), Angle::deg( 90.0 ) );
    //std::cout << IO::toString( manipulator.joint_position( 3 ) );
    auto [ x, z ] = manipulator.find_end_angles( copy );
    //std::cout << "x: " << x.deg() << " z: " << z.deg() << '\n';
}


TEST_CASE( "Simple collisions" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    REQUIRE( world.prepare() );
    auto manipulator = Manipulator( world );
    std::vector< Sphere > spheres = { Sphere( { 0.0, 1.0, 0.0 } ), Sphere( { 0.0, -1.0, 0.0 } ), Sphere( { 2.0, 0.0, 0.0 } ) };
    for( const auto& sphere : spheres ){
        manipulator.collision_tree->insert( sphere );
    }
    auto colliding = manipulator.colliding_spheres( 0, 1, manipulator.joint_position( 0 ) * X );
    REQUIRE( std::count( colliding.begin(), colliding.end(), spheres[ 0 ] ) );
    REQUIRE( std::count( colliding.begin(), colliding.end(), spheres[ 1 ] ) );
    REQUIRE( !std::count( colliding.begin(), colliding.end(), spheres[ 2 ] ) );
}

TEST_CASE( "Joint limits 1" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    REQUIRE( world.prepare() );
    auto manipulator = Manipulator( world );
    std::vector< Sphere > spheres = { Sphere( { 0.0, 1.0, 0.0 } ), Sphere( { 0.0, -1.0, 0.0 } ), Sphere( { 2.0, 0.0, 0.0 } ) };
    for( const auto& sphere : spheres ){
        manipulator.collision_tree->insert( sphere );
    }
    auto [ l,h ] = manipulator.find_limit( 0, manipulator.joint_position( 0 ) * X );
    REQUIRE( l.deg() > -30.0 );
    REQUIRE( h.deg() < 30.0 );
    //std::cout << "first: " << l.deg() << " second: " << h.deg() << '\n';
}

TEST_CASE( "Joint limits 2" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );    
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    auto manipulator = Manipulator( world );
    std::vector< Sphere > spheres = { Sphere( { 0.0, 2.0, 0.0 } ), Sphere( { 0.0, -2.0, 0.0 } ) };
    for( const auto& sphere : spheres ){
        manipulator.collision_tree->insert( sphere );
    }
    auto [ l,h ] = manipulator.find_limit( 0, manipulator.joint_position( 0 ) * X );
    REQUIRE( l.deg() > -60.0 );
    REQUIRE( h.deg() < 60.0 );
    //std::cout << "first: " << l.deg() << " second: " << h.deg() << '\n';
}

TEST_CASE( "Joint limits 3" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    auto manipulator = Manipulator( world );
    std::vector< Sphere > spheres = { Sphere( { 1.0, 1.0, 1.0, 1.0 } ), Sphere( { 1.0, -1.0, -1.0, 1.0 } ) };
    for( const auto& sphere : spheres ){
        manipulator.collision_tree->insert( sphere );
    }

    Vector local = inverse( manipulator.joint_position( 1 ) ) * spheres[ 0 ].center;
    auto [ p, a ] = simplify( polar( local ), azimuth( local ) );
    Vector axis = cross_product( Z, rotate( a, Z ) * rotate( p, X ) * Z );
    auto colliding = manipulator.colliding_spheres( 1, 2, manipulator.joint_position( 1 ) * axis );
    auto [ l,h ] = manipulator.find_limit( 1, manipulator.joint_position( 1 ) * axis );
    REQUIRE( h.deg() < 45.0 );
}

TEST_CASE( "Collision avoidance" ){
    RofiWorld world;
    auto& m1 = world.insert( UniversalModule( 0, 90_deg, 0_deg, 0_deg ) );
    auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

    Cube& c = world.insert( Cube( 101 ) );

    connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect< RigidJoint >( c.components()[ 0 ], { 1.5, 0, 0 }, identity );
    connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
    connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( world.prepare() );

    RofiWorld other;
    auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
    auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
    auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
    connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
    connect( m4.getConnector( "B-Z" ), m5.getConnector( "A-Z" ), roficom::Orientation::North );
    connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
    REQUIRE( other.prepare() );
    
    auto manipulator = Manipulator( world );
    auto result = manipulator.fabrik( m5.getBodyB().getPosition() );
    REQUIRE( result );
}

TEST_CASE( "FABRIK on length 3 with East connection" ){
    std::srand( 45 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-Z" ), roficom::Orientation::East );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-Z" ), m5.getConnector( "A-Z" ), roficom::Orientation::East );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "Z-X connection simple" ){
    for( auto conn : { "A-X", "A+X" } ){
        for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
            RofiWorld world;
            auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
            auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );

            connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
            connect( m1.getConnector( "B-Z" ), m2.getConnector( conn ), ori );
            REQUIRE( world.prepare() );

            auto manipulator = Manipulator( world );
            auto copy = manipulator.copy( m2.getConnector( "B-X" ).getPosition() );
            auto [ p, a ] = manipulator.find_angles( 1, copy );
            //std::cout << "p: " << p.deg() << " a: " << a.deg() << "\n";
            CHECK( equals( p.deg(), 0.0 ) );
            CHECK( equals( a.deg(), 0.0 ) );
        }
        
        for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
            RofiWorld world;
            auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
            auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
            connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
            connect( m1.getConnector( "B-Z" ), m2.getConnector( conn ), ori );
            REQUIRE( world.prepare() );

            RofiWorld other;
            auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 180_deg ) );
            auto& m4 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
            connect< RigidJoint >( m3.getConnector( "A-X" ), { 0, 0, 0 }, identity );
            connect( m3.getConnector( "B-Z" ), m4.getConnector( conn ), ori );

            //auto r = float( rand() % 360 ) - 180;
            //m3.setGamma( Angle::deg( r ) );
            REQUIRE( other.prepare() );

            auto manipulator = Manipulator( world );
            auto copy = manipulator.copy( m4.getConnector( "B-X" ).getPosition() );
            
            auto [ p, a ] = manipulator.find_angles( 1, copy );
            //std::cout << "a: " << a.deg() << "\n";
            CHECK( equals( p.deg(), 0.0 ) );
            CHECK( equals( a.deg(), 180.0 ) );
        }
    }
}

TEST_CASE( "Random Z-X rotations" ){
    srand( 46 );
    for( auto conn : { "A-X", "A+X" } ){
        for( int i = 0; i < 10; i++ ){
            for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
                RofiWorld world;
                auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
                auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
                connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
                connect( m1.getConnector( "B-Z" ), m2.getConnector( conn ), ori );
                REQUIRE( world.prepare() );

                RofiWorld other;
                auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
                auto& m4 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
                connect< RigidJoint >( m3.getConnector( "A-X" ), { 0, 0, 0 }, identity );
                connect( m3.getConnector( "B-Z" ), m4.getConnector( conn ), ori );

                auto c = float( rand() % 360 ) - 180;
                auto b = float( rand() % 120 ) - 60;
                m3.setGamma( Angle::deg( c ) );
                m3.setBeta( Angle::deg( b ) );
                REQUIRE( other.prepare() );

                auto manipulator = Manipulator( world );
                auto copy = manipulator.copy( m4.getConnector( "B-X" ).getPosition() );
                
                auto [ p, a ] = manipulator.find_angles( 1, copy );
                //std::cout << "ori: " << int( ori ) << " b: " << b << " c: " << c << '\n';
                //std::cout << "p: " << p.deg() << " a: " << a.deg() << "\n";
                CHECK( equals( p.deg(), b ) );
                CHECK( equals( a.deg(), c ) );
            }
        }
    }
}

TEST_CASE( "X-Z connection simple" ){
    for( auto conn : { "B-X", "B+X" 
    } ){
        for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
            RofiWorld world;
            auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
            auto& m2 = world.insert( UniversalModule( 1, 90_deg, 0_deg, 0_deg ) );

            connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
            connect( m1.getConnector( conn ), m2.getConnector( "A-Z" ), ori );
            REQUIRE( world.prepare() );

            auto manipulator = Manipulator( world );
            auto copy = manipulator.copy( m2.getConnector( "B-X" ).getPosition() );
            auto [ p, a ] = manipulator.find_angles( 1, copy );
            //std::cout << "p: " << p.deg() << " a: " << a.deg() << "\n";
            CHECK( equals( p.deg(), 0.0 ) );
            CHECK( equals( a.deg(), 0.0 ) );
        }
        for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
            RofiWorld world;
            auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
            auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
            connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
            connect( m1.getConnector( conn ), m2.getConnector( "A-Z" ), ori );
            REQUIRE( world.prepare() );

            RofiWorld other;
            auto& m3 = other.insert( UniversalModule( 0, 0_deg, 90_deg, 0_deg ) );
            auto& m4 = other.insert( UniversalModule( 1, 90_deg, 0_deg, 0_deg ) );
            connect< RigidJoint >( m3.getConnector( "A-X" ), { 0, 0, 0 }, identity );
            connect( m3.getConnector( conn ), m4.getConnector( "A-Z" ), ori );
            REQUIRE( other.prepare() );

            auto manipulator = Manipulator( world );
            auto copy = manipulator.copy( m4.getConnector( "B-X" ).getPosition() );
            
            auto [ p, a ] = manipulator.find_angles( 1, copy );
            //std::cout << "p: " << p.deg() << " a: " << a.deg() << "\n";
            CHECK( equals( p.deg(), 90.0 ) );
            CHECK( equals( a.deg(), 0.0 ) );
        }
    }
}

TEST_CASE( "Random X-Z rotations" ){
    srand( 46 );
    for( auto conn : { "B-X", "B+X"
     } ){
        for( int i = 0; i < 10; i++ ){
            for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
                RofiWorld world;
                auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
                auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
                connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
                connect( m1.getConnector( conn ), m2.getConnector( "A-Z" ), ori );
                REQUIRE( world.prepare() );

                RofiWorld other;
                auto& m3 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
                auto& m4 = other.insert( UniversalModule( 1, 90_deg, 0_deg, 0_deg ) );
                connect< RigidJoint >( m3.getConnector( "A-X" ), { 0, 0, 0 }, identity );
                connect( m3.getConnector( conn ), m4.getConnector( "A-Z" ), ori );

                auto c = float( rand() % 360 ) - 180;
                auto b = float( rand() % 120 ) - 60;
                m3.setGamma( Angle::deg( c ) );
                m3.setBeta( Angle::deg( b ) );
                REQUIRE( other.prepare() );

                auto manipulator = Manipulator( world );
                auto copy = manipulator.copy( m4.getConnector( "B-X" ).getPosition() );
                
                auto [ p, a ] = manipulator.find_angles( 1, copy );
                
                //std::cout << "ori: " << int( ori ) << " b: " << b << " c: " << c << '\n';
                //std::cout << "p: " << p.deg() << " a: " << a.deg() << "\n";
                CHECK( equals( p.deg(), b ) );
                CHECK( equals( a.deg(), c ) );
            }
        }
    }
}

TEST_CASE( "FABRIK on length 3 with Z-X connection" ){
    std::srand( 49 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-Z" ), m2.getConnector( "A-X" ), roficom::Orientation::North );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-Z" ), m5.getConnector( "A-X" ), roficom::Orientation::North );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "FABRIK on length 3 with Z+X connection" ){
    std::srand( 49 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-Z" ), m2.getConnector( "A+X" ), roficom::Orientation::North );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-Z" ), m5.getConnector( "A+X" ), roficom::Orientation::North );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "FABRIK on length 3 with X-Z connection" ){
    std::srand( 49 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-X" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-X" ), m5.getConnector( "A-Z" ), roficom::Orientation::North );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "FABRIK on length 3 with +XZ connection" ){
    std::srand( 63 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B+X" ), m2.getConnector( "A-Z" ), roficom::Orientation::North );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B+X" ), m5.getConnector( "A-Z" ), roficom::Orientation::North );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "X-X connection simple" ){
    for( auto inConn : { "B-X", "B+X" } ){
        for( auto conn : { "A-X", "A+X" } ){
            for( auto ori : { roficom::Orientation::North, roficom::Orientation::South, roficom::Orientation::East, roficom::Orientation::West } ){
                RofiWorld world;
                auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
                auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );

                connect< RigidJoint >( m1.getConnector( "A-X" ), { 0, 0, 0 }, identity );
                connect( m1.getConnector( inConn ), m2.getConnector( conn ), ori );
                REQUIRE( world.prepare() );

                auto manipulator = Manipulator( world );
                auto copy = manipulator.copy( m2.getConnector( "B-X" ).getPosition() );
                auto [ p, a ] = manipulator.find_angles( 1, copy );
                //std::cout << "p: " << p.deg() << " a: " << a.deg() << "\n";
                CHECK( equals( p.deg(), 0.0 ) );
                CHECK( equals( a.deg(), 0.0 ) );
            }
        }
    }
}

TEST_CASE( "FABRIK on length 3 with X-X connection" ){
    std::srand( 64 );
    for( int i = 0; i < 10; i++ ){

        RofiWorld world;
        auto& m1 = world.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m2 = world.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m3 = world.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );

        connect< RigidJoint >( m1.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m1.getConnector( "B-X" ), m2.getConnector( "A-X" ), roficom::Orientation::South );
        connect( m2.getConnector( "B-Z" ), m3.getConnector( "A-Z" ), roficom::Orientation::North );
        REQUIRE( world.prepare() );

        RofiWorld other;
        auto& m4 = other.insert( UniversalModule( 0, 0_deg, 0_deg, 0_deg ) );
        auto& m5 = other.insert( UniversalModule( 1, 0_deg, 0_deg, 0_deg ) );
        auto& m6 = other.insert( UniversalModule( 2, 0_deg, 0_deg, 0_deg ) );
        connect< RigidJoint >( m4.getConnector( "A-Z" ), { 0, 0, 0 }, identity );
        connect( m4.getConnector( "B-X" ), m5.getConnector( "A-X" ), roficom::Orientation::South );
        connect( m5.getConnector( "B-Z" ), m6.getConnector( "A-Z" ), roficom::Orientation::North );
        //REQUIRE( other.prepare() );

        for( auto& mod : other.modules() ){
            dynamic_cast< UniversalModule& >( mod ).setAlpha( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setBeta( Angle::deg( float( rand() % 120 - 60 ) ) );
            dynamic_cast< UniversalModule& >( mod ).setGamma( Angle::deg( float( rand() % 120 - 60 ) ) );
        }
        REQUIRE( other.prepare() );

        auto manipulator = Manipulator( world );
        bool result = manipulator.fabrik( m6.getConnector( "B-X" ).getPosition() );
        CHECK( result );
        if( !result ){
            std::cout << "target: " << serialization::toJSON( other ) << "\n\nreached: " << serialization::toJSON( manipulator.world ) << '\n';
        }
    }
}

TEST_CASE( "Collision on further joints" ){
    auto json = atoms::readInput( "data/configurations/json/shoulder4_wall.json", rofi::parsing::parseJson );
    auto world = serialization::fromJSON( *json );

    static_cast< UniversalModule* >( world.getModule( 1 ) )->setGamma( 90_deg );
    static_cast< UniversalModule* >( world.getModule( 2 ) )->setAlpha( -45_deg );
    static_cast< UniversalModule* >( world.getModule( 2 ) )->setBeta( -45_deg );
    static_cast< UniversalModule* >( world.getModule( 3 ) )->setAlpha( 45_deg );
    static_cast< UniversalModule* >( world.getModule( 3 ) )->setBeta( 45_deg );


    REQUIRE( world.prepare() );
    Manipulator manipulator( world );

    auto limits = manipulator.find_limit( 1, manipulator.joint_position( 1 ) * X );
    REQUIRE( approx_equal( limits.first.deg(), -90.0 ) );
    REQUIRE( limits.second.deg() < 1.0f );

    static_cast< UniversalModule* >( world.getModule( 1 ) )->setGamma( 45_deg );
    REQUIRE( world.prepare() );
    manipulator.world = world;

    limits = manipulator.find_limit( 1, manipulator.joint_position( 1 ) * X );
    REQUIRE( approx_equal( limits.first.deg(), -90.0 ) );
    REQUIRE( limits.second.deg() < 10.0f );

    static_cast< UniversalModule* >( world.getModule( 1 ) )->setGamma( -45_deg );
    REQUIRE( world.prepare() );
    manipulator.world = world;

    limits = manipulator.find_limit( 1, manipulator.joint_position( 1 ) * X );
    REQUIRE( approx_equal( limits.first.deg(), -90.0 ) );
    REQUIRE( approx_equal( limits.second.deg(), 90.0 ) );


    static_cast< UniversalModule* >( world.getModule( 1 ) )->setAlpha( -90_deg );
    static_cast< UniversalModule* >( world.getModule( 1 ) )->setGamma( 45_deg );

    REQUIRE( world.prepare() );
    // std::cout << serialization::toJSON( world ) << '\n';
    manipulator.world = world;

    limits = manipulator.find_limit( 1, manipulator.joint_position( 1 ) * X );
    //std::cout << "low: " << limits.first.deg() << " high: " << limits.second.deg() << '\n';
    REQUIRE( limits.first.deg() > -1.5f );
    REQUIRE( approx_equal( limits.second.deg(), 90.0 ) );
}
}