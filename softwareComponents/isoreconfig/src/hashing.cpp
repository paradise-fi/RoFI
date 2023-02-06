#include <isoreconfig/hashing.hpp>

namespace rofi::isoreconfig {

using namespace rofi::configuration;

size_t combineHash( size_t h1, size_t h2 ) 
{ 
    return h1 ^ (h2 << 1);
}

// If converted to degrees and rounded, equal degree has same hash
size_t hashRad( float rads )
{
    return std::hash<int>{}( static_cast<int>( round( rads * 100 ) ) );
}

size_t hashJoint( const Joint& j )
{
    size_t h1 = std::accumulate( j.jointLimits().begin(), j.jointLimits().end(), 0, 
        []( size_t acc, const std::pair< float, float >& lims ){ 
            size_t h = combineHash( hashRad( lims.first ), hashRad( lims.second ) );
            return combineHash( acc, h );
        } );

    size_t h2 = std::accumulate( j.positions().begin(), j.positions().end(), 0, 
        []( size_t acc, const float& pos ){ 
            return combineHash( acc, hashRad( pos ) ); 
        } );

    return combineHash( h1, h2 );
}

size_t hashRoficomJoint( const RoficomJoint& rj )
{
    size_t res = hashJoint( rj );
    res = combineHash( res, std::hash<size_t>{}( static_cast<size_t>( rj.orientation ) ) );
    res = combineHash( res, std::hash<size_t>{}( static_cast<size_t>( rj.sourceModule ) ) );
    res = combineHash( res, std::hash<size_t>{}( static_cast<size_t>( rj.destModule ) ) );
    res = combineHash( res, std::hash<int>{}( rj.sourceConnector ) );
    res = combineHash( res, std::hash<int>{}( rj.destConnector ) );
    return res;
}

size_t hashComponentJoint( const ComponentJoint& cj )
{
    size_t res = hashJoint(*cj.joint);
    res = combineHash( res, std::hash<int>{}( cj.sourceComponent ));
    res = combineHash( res, std::hash<int>{}( cj.destinationComponent ));
    return res;
}

size_t hashModule( const Module& mod )
{
    size_t res = std::hash<size_t>{}(static_cast<size_t>(mod.type));
    res = combineHash( res, std::hash<int>{}(mod.getId()) );
    size_t h = std::accumulate( mod.joints().begin(), mod.joints().end(), 0, 
        []( size_t acc, const ComponentJoint& cj ){ 
            return combineHash( acc, hashComponentJoint( cj ) ); 
        } );
    return combineHash( res, h );
}

size_t RofiWorldHash::operator()( const RofiWorld& rw ) const
{
    size_t h1 = std::accumulate( rw.modules().begin(), rw.modules().end(), 0, 
        []( size_t res, const Module& rModule ){ 
            return combineHash( res, hashModule( rModule ) ); });
    size_t h2 = std::accumulate( rw.roficomConnections().begin(), rw.roficomConnections().end(), 0, 
        []( size_t res, const RoficomJoint& rj ){ 
            return combineHash( res, hashRoficomJoint( rj ) ); });
    return combineHash( h1, h2 );
}

size_t hashPoint( const std::array< int, 3 >& pt )
{
    size_t h = combineHash( std::hash<int>{}( pt[0] ), std::hash<int>{}( pt[1] ) );
    return combineHash( h, std::hash<int>{}( pt[2] ) );
}

size_t hashSphere( const std::pair< size_t, std::vector< std::array< int, 3 > > >& sphere )
{
    size_t radiusHash = std::hash<size_t>{}( sphere.first );
    return std::accumulate( sphere.second.begin(), sphere.second.end(), radiusHash,
        []( size_t res, const std::array< int, 3 >& pt ){
            return combineHash( res, hashPoint( pt ) );
        } );
}

size_t HashCloud::operator()( const Cloud& cop ) const
{
    return std::accumulate( cop.begin(), cop.end(), 0,
        []( size_t res, const std::pair< size_t, std::vector< std::array< int, 3 > > >& sphere ){
            return combineHash( res, hashSphere( sphere ) );
        } );
}

} // namespace rofi::isoreconfig
