#include <shapeReconfig/hashing.hpp>

namespace rofi::shapereconfig {

size_t combineHash( size_t h1, size_t h2 ) 
{ 
    return h1 ^ (h2 << 1);
}

// If converted to degrees and rounded, equal degree has same hash
size_t hashRad( float rads )
{
    return std::hash<int>{}( static_cast<int>( round( rads * 100 ) ) );
}

size_t hashJoint( const rofi::configuration::Joint& j )
{
    auto jointLims = j.jointLimits();
    size_t h1 = std::accumulate( jointLims.begin(), jointLims.end(), size_t{}, 
        []( size_t acc, const std::pair< float, float >& lims ){ 
            size_t h = combineHash( hashRad( lims.first ), hashRad( lims.second ) );
            return combineHash( acc, h );
        } );

    auto jointPositions = j.positions();
    size_t h2 = std::accumulate( jointPositions.begin(), jointPositions.end(), size_t{}, 
        []( size_t acc, const float& pos ){ 
            return combineHash( acc, hashRad( pos ) ); 
        } );

    return combineHash( h1, h2 );
}

size_t hashRoficomJoint( const rofi::configuration::RoficomJoint& rj )
{
    size_t res = hashJoint( rj );
    res = combineHash( res, std::hash<size_t>{}( static_cast<size_t>( rj.orientation ) ) );
    res = combineHash( res, std::hash<size_t>{}( static_cast<size_t>( rj.sourceModule ) ) );
    res = combineHash( res, std::hash<size_t>{}( static_cast<size_t>( rj.destModule ) ) );
    res = combineHash( res, std::hash<int>{}( rj.sourceConnector ) );
    res = combineHash( res, std::hash<int>{}( rj.destConnector ) );
    return res;
}

size_t hashComponentJoint( const rofi::configuration::ComponentJoint& cj )
{
    size_t res = hashJoint( *cj.joint );
    res = combineHash( res, std::hash<int>{}( cj.sourceComponent ));
    res = combineHash( res, std::hash<int>{}( cj.destinationComponent ));
    return res;
}

size_t hashModule( const rofi::configuration::Module& mod )
{
    size_t res = std::hash<size_t>{}(static_cast<size_t>(mod.type));
    res = combineHash( res, std::hash<int>{}( mod.getId() ) );
    auto modJoints = mod.joints();
    return std::accumulate( modJoints.begin(), modJoints.end(), res, 
        []( size_t acc, const rofi::configuration::ComponentJoint& cj ){ 
            return combineHash( acc, hashComponentJoint( cj ) ); 
        } );
}

size_t RofiWorldHash::operator()( const rofi::configuration::RofiWorld& rw ) const
{
    auto mods = rw.modules();
    size_t h1 = std::accumulate( mods.begin(), mods.end(), size_t{}, 
        []( size_t res, const rofi::configuration::Module& rModule ){ 
            return combineHash( res, hashModule( rModule ) ); });
    auto conns = rw.roficomConnections();
    size_t h2 = std::accumulate( conns.begin(), conns.end(), size_t{}, 
        []( size_t res, const rofi::configuration::RoficomJoint& rj ){ 
            return combineHash( res, hashRoficomJoint( rj ) ); });
    return combineHash( h1, h2 );
}

constexpr auto hashPoint = HashArray< int, 3 >();

size_t hashSphere( const std::pair< size_t, std::vector< std::array< int, 3 > > >& sphere )
{
    std::vector< size_t > transSpheres( sphere.second.size() );
    std::ranges::transform( sphere.second, transSpheres.begin(), hashPoint );
    size_t radiusHash = std::hash<size_t>{}( sphere.first );
    return std::accumulate( transSpheres.begin(), transSpheres.end(), radiusHash, combineHash );
}

size_t HashCloud::operator()( const Cloud& cop ) const
{
    std::vector< size_t > transCop( cop.size() );
    std::ranges::transform( cop, transCop.begin(), hashSphere );
    return std::accumulate( transCop.begin(), transCop.end(), size_t{}, combineHash );
}

} // namespace rofi::shapereconfig
