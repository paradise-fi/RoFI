#include "universalModule.h"

namespace rofi {

Module buildUniversalModule( double alpha, double beta, double gamma ) {
    std::vector< Component > components = {
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

    std::vector< ComponentJoint > joints = {
        // ToDo: Specify the joints properly
        makeJoint< RotationJoint >( 7, 6,
            identity, Vector( {1, 0, 0, 1} ), identity, -M_PI / 2, M_PI / 2 ), // Alpha
        makeJoint< RotationJoint >( 8, 9,
            identity, Vector( {1, 0, 0, 1} ), identity, -M_PI / 2, M_PI / 2 ), // Beta
        makeJoint< RotationJoint >( 7, 8,
            identity, Vector( {1, 0, 0, 1} ), identity, -M_PI / 2, M_PI / 2 ), // Gamma
        makeJoint< RigidJoint >( 6, 0, identity ),
        makeJoint< RigidJoint >( 6, 1, identity ),
        makeJoint< RigidJoint >( 6, 2, identity ),
        makeJoint< RigidJoint >( 9, 0, identity ),
        makeJoint< RigidJoint >( 9, 1, identity ),
        makeJoint< RigidJoint >( 9, 2, identity ),
    };

    joints[ 0 ].joint->positions = { alpha };
    joints[ 1 ].joint->positions = { beta };
    joints[ 2 ].joint->positions = { gamma };

    return Module( ModuleType::Universal, std::move( components ), 7,
        std::move( joints ) );
}

}