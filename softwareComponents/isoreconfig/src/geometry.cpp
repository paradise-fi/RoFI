#include <isoreconfig/geometry.hpp>

namespace rofi::isoreconfig {

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
        cop2.rotateBy180Around( 2 ); // Rotate X upside down along Z
    }
    
    return false;
}

} // namespace rofi::isoreconfig
