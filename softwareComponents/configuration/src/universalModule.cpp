#include <configuration/universalModule.hpp>

namespace rofi::configuration {

std::vector< Component > UniversalModule::_initComponents() {
    return std::vector< Component > {
        Component{ ComponentType::Roficom },
        Component{ ComponentType::Roficom },
        Component{ ComponentType::Roficom },
        Component{ ComponentType::Roficom },
        Component{ ComponentType::Roficom },
        Component{ ComponentType::Roficom },
        Component{ ComponentType::UmShoe },
        Component{ ComponentType::UmBody },
        Component{ ComponentType::UmBody },
        Component{ ComponentType::UmShoe }
    };
}

std::vector< ComponentJoint > UniversalModule::_initJoints( Angle alpha, Angle beta, Angle gamma ) {
    std::vector< ComponentJoint > joints = {
        makeComponentJoint< RotationJoint >( 7, 6, // BodyA <-> ShoeA
            identity, Vector( { 1, 0, 0 } ), identity, Angle::rad( - M_PI_2 ), Angle::rad( M_PI_2 ) ),
        makeComponentJoint< RotationJoint >( 8, 9 // BodyB <-> ShoeB
            , identity
            , Vector( { 1, 0, 0 } )
            , identity
            , Angle::rad( - M_PI_2 ), Angle::rad( M_PI_2 ) ),
        makeComponentJoint< RotationJoint >( 7, 8 // BodyA <-> BodyB
            , identity
            , Vector( { 0, 0, 1 } )
            , translate( { 0, 0, 1 } ) * rotate( M_PI, { 0, 1, 0 } )
            , Angle::rad( - M_PI ), Angle::rad( M_PI ) ),
        makeComponentJoint< RigidJoint >( 6, 0, identity ), // A-X
        makeComponentJoint< RigidJoint >( 6, 1, rotate( M_PI, { 0, 1, 0 } ) ), // A+X
        makeComponentJoint< RigidJoint >( 6, 2, rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI_2, { 0, -1, 0 } ) ), // A-Z
        makeComponentJoint< RigidJoint >( 9, 3, identity ), // B-X
        makeComponentJoint< RigidJoint >( 9, 4, rotate( M_PI, { 0, 1, 0 } ) ), // B+X
        makeComponentJoint< RigidJoint >( 9, 5, rotate( M_PI, { 0, 0, 1 } ) * rotate( M_PI_2, { 0, -1, 0 } ) )  // B-Z
    };

    joints[ 0 ].joint->positions = { alpha.rad() };
    joints[ 1 ].joint->positions = { beta.rad() };
    joints[ 2 ].joint->positions = { gamma.rad() };

    return joints;
}


bool checkOldConfigurationEInput( int id1, int id2, int side1, int side2, int dock1, int dock2
                                    , int orientation, const std::set< ModuleId >& knownModules ) {
    bool ok = true;
    if ( knownModules.count( id1 ) == 0)
        std::cerr << "Unknown module id " << id1 << " skipping edge\n", ok = false;
    if ( knownModules.count( id2 ) == 0 )
        std::cerr << "Unknown module id " << id2 << " skipping edge\n", ok = false;
    if ( orientation > 3 || orientation < 0 )
        std::cerr << "Invalid orientation: " << orientation << "\n", ok = false;
    if ( side1 > 2 || side1 < 0 )
        std::cerr << "Invalid side1 specification, got: " << side1 << "\n", ok = false;
    if ( side2 > 2 || side2 < 0 )
        std::cerr << "Invalid side2 specification, got: " << side2 << "\n", ok = false;
    if ( dock1 > 3 || dock1 < 0 )
        std::cerr << "Invalid dock1 specification, got: " << dock1 << "\n", ok = false;
    if ( dock2 > 3 || dock2 < 0 )
        std::cerr << "Invalid dock2 specification, got: " << dock2 << "\n", ok = false;

    return ok;
}

int parseStringDescription( std::istringstream& line
                          , std::vector< std::pair< std::string, int > > opts ) {
    std::string part;
    line >> part;
    for ( auto& [ str, value ] : opts ) {
        if ( str == part )
            return value;
    }
    std::string expected = "| ";
    for ( auto& [ str, _ ] : opts )
        expected.append( str ).append( " | " );
    throw std::runtime_error( "Unexpected value |" + part + "| expected one of " + expected );
}

auto parseEdge( std::istringstream& line ) {
    int id1, id2, side1, side2, dock1, dock2, orientation;
    line >> id1 >> side1;
    if ( line ) {
        line >> dock1 >> orientation >> dock2 >> side2;
    } else { // config file has the letter notation
        line.clear(); // reset the bad state
        side1 = parseStringDescription( line, { { "A", 0 }, { "B", 1 } } );
        dock1 = parseStringDescription( line, { { "+X", 0 }, { "-X", 1 }, { "-Z", 2 } } );
        orientation = parseStringDescription( line, { { "N", 0 }, { "E", 1 }, { "S", 2 }, { "W", 3 } } );
        dock2 = parseStringDescription( line, { { "+X", 0 }, { "-X", 1 }, { "-Z", 2 } } );
        side2 = parseStringDescription( line, { { "A", 0 }, { "B", 1 } } );
    }
    line >> id2;

    return std::make_tuple( id1, id2, side1, side2, dock1, dock2, orientation );
}

Rofibot readOldConfigurationFormat( std::istream& s ) {
    std::string line;
    Rofibot rofibot;
    std::set< ModuleId > knownModules;
    while ( std::getline( s, line ) ) {
        std::istringstream lineStr( line );
        std::string type;

        lineStr >> type;
        if ( type.empty() || type[0] == '#' )
            continue; // Empty line or a comment

        if ( type == "C" )
            continue; // Initial configuration found
        if ( type == "M" ) {
            double alpha, beta, gamma;
            int id;
            lineStr >> id >> alpha >> beta >> gamma;
            auto rModule = UniversalModule( id, Angle::deg( alpha ), Angle::deg( beta ), Angle::deg( gamma ) );
            rofibot.insert( std::move( rModule ) );
            knownModules.insert( id );
            continue;
        }
        if ( type == "E" ) {
            auto [ id1, id2, side1, side2, dock1, dock2, orientation ] = parseEdge( lineStr );
            if ( !checkOldConfigurationEInput( id1, id2, side1, side2, dock1, dock2, orientation, knownModules ) )
                throw std::runtime_error( "Invalid edge specification" );
            auto& component1 = rofibot.getModule( id1 )->connectors()[ side1 * 3 + dock1 ];
            auto& component2 = rofibot.getModule( id2 )->connectors()[ side2 * 3 + dock2 ];
            connect( component1, component2, static_cast< roficom::Orientation >( orientation ) );
            continue;
        }
        throw std::runtime_error("Expected a module (M) or edge (E), got " + type + ".");
    }
    return rofibot;
}

}  // namespace rofi::configuration
