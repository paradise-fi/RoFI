#include <isoreconfig/isomorphic.hpp>
#include <cassert>

namespace rofi::isoreconfig {

using namespace rofi::configuration;

Positions decomposeUniversalModule( 
    const Module& mod )
{
    // <mod> must be a UniversalModule
    assert( mod.type == ModuleType::Universal );

    std::span<const Component> comps = mod.components();
    Positions result;

    for ( int compIndex = 0; compIndex < 6; ++compIndex )
    {
        // First six components of UniversalModule are Roficoms
        assert( comps[ compIndex ].type == ComponentType::Roficom );

        Matrix absCompPos = comps[ compIndex ].getPosition();
        // Component position of a UniversalModule is always in the center 
        // of the shoe it belongs to; to get the "actual" (visual) position,
        // we want to move it by half a unit on -X
        result.push_back( absCompPos * matrices::translate( { -0.5, 0, 0 } ) );
    } 

    return result;
}

std::array< Positions, 2 > decomposeRofiWorld( const RofiWorld& rw )
{
    rw.isValid().get_or_throw_as< std::logic_error >();

    std::array< Positions, 2 > result;

    // Decompose modules
    for ( const auto& rModule : rw.modules() )
        for ( const Matrix& pos : decomposeUniversalModule( rModule ) )
            result[0].push_back( pos );

    // Decompose connections
    for ( const RoficomJoint& connection : rw.roficomConnections() )
    {
        Matrix pos = connection.getSourceModule( rw ).components()[connection.sourceConnector].getPosition();
        // Connection position is in the center of the connected module,
        // so it must be translated by half a unit
        result[1].push_back( pos * matrices::translate( { -0.5, 0, 0 } ) );
    }

    return std::tie( modulePoints, connectionPoints );
}

Cloud rofiWorldToCloud( const RofiWorld& rw )
{
    auto [ modulePoints, connectionPoints ] = decomposeRofiWorld( rw );

    // Merge module points and connection points into one container
    for ( const Vector& pt : connectionPoints )
        modulePoints.push_back( pt );
    
    return Cloud( modulePoints );
}

Vector centroid( const RofiWorld& rw )
{
    auto [ modulePoints, connectionPoints ] = decomposeRofiWorld( rw );
    for ( const Vector& pt : connectionPoints )
        modulePoints.push_back( pt );
    return centroid( modulePoints );
}

Vector centroid( const std::vector< Vector >& pts )
{
    assert( pts.size() >= 1 );

    Vector result = std::accumulate( ++pts.begin(), pts.end(), pts[0], 
        []( const Vector& pt1, const Vector& pt2 ){ return pt1 + pt2; } );

    for ( size_t i = 0; i < 3; ++i )
        result(i) /= double(pts.size());

    return result;
}

bool equalShape( const RofiWorld& rw1, const RofiWorld& rw2 )
{
    // Worlds with different number of modules or connections do not have same shape
    if ( rw1.modules().size() != rw2.modules().size() ||
         rw1.roficomConnections().size() != rw2.roficomConnections().size() )
        return false;

    return isometric( rofiWorldToCloud( rw1 ), rofiWorldToCloud( rw2 ) );
}

} // namespace rofi::isoreconfig
