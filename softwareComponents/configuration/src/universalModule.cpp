#include <configuration/universalModule.hpp>

namespace rofi::configuration {

using namespace rofi::configuration::matrices;

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

std::vector< ComponentJoint > UniversalModule::_initJoints() {
    std::vector< ComponentJoint > joints = {
        makeComponentJoint< RotationJoint >( 7, 6, // BodyA <-> ShoeA
            identity, Vector( { 1, 0, 0 } ), identity, Angle::rad( -Angle::pi / 2 ), Angle::rad( Angle::pi / 2 ) ),
        makeComponentJoint< RotationJoint >( 8, 9 // BodyB <-> ShoeB
            , identity
            , Vector( { 1, 0, 0 } )
            , identity
            , Angle::rad( -Angle::pi / 2 ), Angle::rad( Angle::pi / 2 ) ),
        makeComponentJoint< RotationJoint >( 7, 8 // BodyA <-> BodyB
            , identity
            , Vector( { 0, 0, 1 } )
            , translate( { 0, 0, 1 } ) * rotate( Angle::pi, { 0, 1, 0 } )
            , Angle::rad( -Angle::pi ), Angle::rad( Angle::pi ) ),
        makeComponentJoint< RigidJoint >( 6, 0, identity ), // A-X
        makeComponentJoint< RigidJoint >( 6, 1, rotate( Angle::pi, { 0, 1, 0 } ) ), // A+X
        makeComponentJoint< RigidJoint >( 6, 2, rotate( Angle::pi, { 0, 0, 1 } ) * rotate( Angle::pi / 2, { 0, -1, 0 } ) ), // A-Z
        makeComponentJoint< RigidJoint >( 9, 3, identity ), // B-X
        makeComponentJoint< RigidJoint >( 9, 4, rotate( Angle::pi, { 0, 1, 0 } ) ), // B+X
        makeComponentJoint< RigidJoint >( 9, 5, rotate( Angle::pi, { 0, 0, 1 } ) * rotate( Angle::pi / 2, { 0, -1, 0 } ) )  // B-Z
    };

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

int parseIntOrString( std::istringstream& line, std::vector< std::pair< std::string, int > > opts) {
    int posibleRes;
    auto pos = line.tellg();
    line >> posibleRes;
    if ( line ) {
        return posibleRes;
    }

    line.clear();
    line.seekg( pos );
    return parseStringDescription( line, opts );
}

auto parseEdge( std::istringstream& line ) {
    int id1, id2, side1, side2, dock1, dock2, orientation;
    line >> id1;

    side1 = parseIntOrString( line, { { "A", 0 }, { "B", 1 } } );
    dock1 = parseIntOrString( line, { { "+X", 0 }, { "-X", 1 }, { "-Z", 2 } } );
    orientation = parseIntOrString( line, { { "N", 0 }, { "E", 1 }, { "S", 2 }, { "W", 3 } } );
    dock2 = parseIntOrString( line, { { "+X", 0 }, { "-X", 1 }, { "-Z", 2 } } );
    side2 = parseIntOrString( line, { { "A", 0 }, { "B", 1 } } );

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
            float alpha, beta, gamma;
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

    int UniversalModule::translateComponent( const std::string& cStr ) {
        if ( cStr == "A-X" ) { return 0; }
        if ( cStr == "A+X" ) { return 1; }
        if ( cStr == "A-Z" ) { return 2; }
        if ( cStr == "B-X" ) { return 3; }
        if ( cStr == "B+X" ) { return 4; }
        if ( cStr == "B-Z" ) { return 5; }

        assert( false && "translateComponent got invalid input" );
    }

    std::string UniversalModule::translateComponent( int c ) {
        switch ( c ) {
            case 0:
                return "A-X";
            case 1:
                return "A+X";
            case 2:
                return "A-Z";
            case 3:
                return "B-X";
            case 4:
                return "B+X";
            case 5:
                return "B-Z";
        }
        assert( false && "translateComponent got invalid input" );
    }


}  // namespace rofi::configuration
