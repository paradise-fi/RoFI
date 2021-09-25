#include "pad.hpp"

namespace rofi {

Module buildPad( int size ) {
    return buildPad( size, size );
}

Module buildPad( int sizeN, int sizeM ) {
    assert( sizeN > 0 && sizeM > 0 && "buildPad has to get positive dimensions" );
    std::vector< Component > components( sizeN * sizeM, Component { ComponentType::Roficom } );

    std::vector< ComponentJoint > joints;
    
    for ( int i = 0; i < sizeN; i++ ) {
        for ( int j = 0; j < sizeM; j++ ) {
            if ( i > 0 ) {
                joints.push_back(
                    makeJoint< RigidJoint >( (i - 1) * sizeM + j, i * sizeM + j, translate( { 0, 1, 0 } ) )
                );
            }

            if ( j > 0 ) {
                joints.push_back(
                    makeJoint< RigidJoint >( i * sizeM + j - 1, i * sizeM + j, translate( { 0, 0, 1 } ) )
                );
            }
        }
    }

    return Module( ModuleType::Pad, std::move( components ), sizeN * sizeM,
        std::move( joints ), 0 );
}

} // namespace rofi
