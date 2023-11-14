#include <shapeReconfig/geometry.hpp>

namespace rofi::shapereconfig {

Vector centroid( const std::vector< Vector >& pts )
{
    assert( pts.size() >= 1 );

    Vector result = std::accumulate( ++pts.begin(), pts.end(), pts[0], 
        []( const Vector& pt1, const Vector& pt2 ){ return pt1 + pt2; } );

    for ( size_t i = 0; i < 3; ++i )
        result(i) /= double(pts.size());

    return result;
}

bool isometric( const Cloud& cop1, Cloud cop2 )
{
    // Assume different number of points implies nonequal shapes
    // (even if points overlap)
    if ( cop1.size() != cop2.size() )
        return false;

    // Go through 24 orthogonal rotations in third dimension
    // (8 octants, each of which can have the axes arranged in 3 ways)
    for ( size_t hp = 0; hp < 2; ++hp ) 
    {
        for ( size_t rot = 0; rot < 4; ++rot ) 
        {
            for ( size_t permut = 0; permut < 3; ++permut )  
            {
                if ( cop1 == cop2 ) 
                    return true;
                
                cop2.permutateAxes(); // Permute axes in current octant
            }
            cop2.rotateBy90Around( 0 ); // Rotating Y and Z around X
        }
        cop2.rotateBy180Around( 2 ); // Rotate X upside down along Z (bottom octants)
    }
    
    return false;
}

Cloud canonCloud( Cloud cop )
{
    Cloud res = cop;

    for ( size_t hp = 0; hp < 2; ++hp ) 
    {
        for ( size_t rot = 0; rot < 4; ++rot ) 
        {
            for ( size_t permut = 0; permut < 3; ++permut )  
            {
                res = std::max( res, cop );
                
                cop.permutateAxes(); // Permute axes in current octant
            }
            cop.rotateBy90Around( 0 ); // Rotating Y and Z around X
        }
        cop.rotateBy180Around( 2 ); // Rotate X upside down along Z (bottom octants)
    }

    return res;
}

} // namespace rofi::shapereconfig
