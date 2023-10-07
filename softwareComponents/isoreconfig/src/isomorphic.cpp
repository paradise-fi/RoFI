#include <cassert>
#include <isoreconfig/isomorphic.hpp>

namespace rofi::isoreconfig {

using namespace rofi::configuration;

Matrix pointMatrix( const Vector& pt )
{
    Matrix result( arma::eye( 4, 4 ) );
    result.col(3) = pt;
    result(3,3) = 1;
    return result;
}

std::vector< Vector > decomposeModule( const Module& mod )
{
    std::vector< Vector > result;

    for ( const Component& comp : mod.components() )
    {
        if ( comp.type != ComponentType::Roficom ) 
            continue;

        Matrix posMat = comp.getPosition() * matrices::translate( { -0.5, 0, 0 } );
        // Center of roficoms of a module is in the center 
        // of the shoe it belongs to; to get the "actual" (visual) position,
        // move it by half a unit in the direction the roficom is facing (X axis)
        result.push_back( arma::round( posMat.col(3) / ERROR_MARGIN ) * ERROR_MARGIN );
    } 

    return result;
}

std::tuple< std::vector< Vector >, std::vector< Vector > > decomposeRofiWorld( const RofiWorld& rw )
{
    rw.isValid().get_or_throw_as< std::logic_error >();

    std::vector< Vector > modulePoints;

    // Decompose modules
    for ( const Module& rModule : rw.modules() )
        for ( const Vector& pos : decomposeModule( rModule ) )
            modulePoints.push_back( pos );

    std::vector< Vector > connectionPoints;

    // Decompose connections
    for ( const RoficomJoint& connection : rw.roficomConnections() )
    {
        Matrix posMat = connection.getSourceModule( rw ).components()[connection.sourceConnector].getPosition();
        // Connection position is in the center of the connected module,
        // so it must be translated by half a unit
        posMat *= matrices::translate( { -0.5, 0, 0 } );
        connectionPoints.push_back( arma::round( posMat.col(3) / ERROR_MARGIN ) * ERROR_MARGIN );
    }

    return std::tie( modulePoints, connectionPoints );
}

std::vector< std::vector< Vector > > decomposeRofiWorldModules( 
    const rofi::configuration::RofiWorld& rw )
{
    std::vector< std::vector< Vector > > decomposedModules;

    for ( const Module& rModule : rw.modules() )
        decomposedModules.push_back( decomposeModule( rModule ) );

    return decomposedModules;
}

Cloud rofiWorldToCloud( const RofiWorld& rw )
{
    auto [ modulePoints, connectionPoints ] = decomposeRofiWorld( rw );

    // Merge module points and connection points into one container
    for ( const Vector& pt : connectionPoints )
        modulePoints.push_back( pt );
    
    return Cloud( modulePoints );
}

Cloud rofiWorldToShape( const RofiWorld& rw )
{
    return canonCloud( rofiWorldToCloud( rw ) );
}

std::array< int, 4 > rofiWorldToEigenValues( const RofiWorld& rw )
{
    auto [ modulePoints, connectionPoints ] = decomposeRofiWorld( rw );

    // Merge module points and connection points into one container
    for ( const Vector& pt : connectionPoints )
        modulePoints.push_back( pt );

    arma::mat data;
    data.set_size( modulePoints.size(), 3 );

    for ( size_t i = 0; i < modulePoints.size(); ++i )
        for ( size_t j = 0; j < 3; ++j )
            data(i, j) = modulePoints[i](j);

    arma::mat coeff;
    arma::mat score;
    arma::vec latent;
    arma::vec tsquared;

    princomp( coeff, score, latent, tsquared, data );

    int signum = 1;
    if ( det( coeff ) < 0 )
        signum = -1;

    return { static_cast<int>( round( latent(0) / 0.0001 ) ), 
        static_cast<int>( round( latent(1) / 0.0001 ) ),
        static_cast<int>( round( latent(2) / 0.0001 ) ),
        signum };
}

Vector centroid( const RofiWorld& rw )
{
    auto [ modulePoints, connectionPoints ] = decomposeRofiWorld( rw );
    for ( const Vector& pt : connectionPoints )
        modulePoints.push_back( pt );
    return centroid( modulePoints );
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
