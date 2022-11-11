#include <isoreconfig/isomorphic.hpp>
#include <isoreconfig/geometry.hpp>
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
    auto [ ok, err ] = rw.isValid();
    if ( !ok ) throw std::logic_error( "RofiWorld to decompose is not prepared" );

    std::array< Positions, 2 > result;

    // Decompose modules
    for ( const auto& /*RofiWorld::ModuleInfo*/ modInf : rw.modules() )
        for ( const Matrix& pos : decomposeUniversalModule( *modInf.module ) )
            result[0].push_back( pos );

    // Decompose connections
    for ( const RoficomJoint& connection : rw.roficomConnections() )
    {
        Matrix pos = connection.getSourceModule( rw ).components()[connection.sourceConnector].getPosition();
        // Connection position is in the center of the connected module,
        // so it must be translated by half a unit
        result[1].push_back( pos * matrices::translate( { -0.5, 0, 0 } ) );
    }

    return result;
}

Matrix centroid( const RofiWorld& rw )
{
    return pointToPos( centroid( decomposeRofiWorld( rw )[0] ));
}

} // namespace rofi::isoreconfig
