#include <isoreconfig/isomorphic.hpp>
#include <cassert>

namespace rofi::isoreconfig {

using namespace rofi::configuration;

Matrix pointMatrix( const Vector& pt )
{
    Matrix result( arma::eye( 4, 4 ) );
    result.col(3) = pt;
    result(3,3) = 1;
    return result;
}

std::vector< Vector > decomposeModule( const Module& rModule )
{
    std::vector< Vector > modulePoints;

    for ( const Component& comp : rModule.components() )
    {
        if ( comp.type != ComponentType::Roficom ) 
            continue;

        // Center of roficoms of a module is in the center 
        // of the shoe it belongs to; to get the "actual" (visual) position,
        // move it by half a unit in the direction the roficom is facing (X axis)
        Matrix posMat = comp.getPosition() * matrices::translate( { -0.5, 0, 0 } );
        modulePoints.push_back( posMat.col(3) );
    } 

    return modulePoints;
}

std::tuple< std::vector< Vector >, std::vector< Vector > > decomposeRofiWorld( 
    const RofiWorld& rw )
{
    rw.isValid().get_or_throw_as< std::logic_error >();

    std::vector< Vector > modulePoints;

    // Decompose modules
    for ( const auto& rModule : rw.modules() )
        for ( const auto& pos : decomposeModule( rModule ) )
            modulePoints.push_back( pos );

    std::vector< Vector > connectionPoints;

    // Decompose connections
    for ( const RoficomJoint& connection : rw.roficomConnections() )
    {
        Matrix posMat = connection.getSourceModule( rw ).components()[connection.sourceConnector].getPosition();
        // Connection position is in the center of the connected module,
        // so it must be translated by half a unit
        posMat *= matrices::translate( { -0.5, 0, 0 } );
        connectionPoints.push_back( posMat.col(3) );
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
