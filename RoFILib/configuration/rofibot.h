#pragma once

#include <memory>
#include <vector>

#include <containers.hpp>

#include "joints.h"
#include "Matrix.h"

namespace rofi {

/// ModuleId
using ModuleId = int;

enum class ComponentType {
    UmShoe, UmBody,
    Roficom
};

enum class ModuleType {
    Unknown,
    Universal,
    Cube,
};

/**
 * \brief Joint between two components of the same module
 */
struct ComponentJoint {
    std::unique_ptr< Joint > joint;
    const ModuleId sourceComponent, destinationComponent;
};

/**
 * \brief Joint between two modules.
 *
 * The joint is created between two components of two modules specified by
 * module ids and corresponding component index.
 */
struct ModuleJoint {
    std::unique_ptr< Joint > joint;
    ModuleId sourceModule;
    ModuleId destModule;
    int sourceComponent;
    int destComponent;
};

/**
 * \brief Joint between a fixed point in space and a module
 */
struct SpaceJoint {
    std::unique_ptr< Joint > joint;
    const Vector refPoint;
    const ModuleId destModule;
    const int destComponent;
};

struct Rofibot;
struct Module;

/**
 * \brief Single body of a module
 */
struct Component {
    ComponentType type;
    Module *parent;

    const std::vector< int > joints; ///< indices of joints referencing this component
};

/**
 * \brief RoFI module
 */
struct Module {
    ModuleId id; ///< integral identifier unique within a context of a single rofibot
    ModuleType type;
    const std::vector< Component > components;
    const std::vector< std::unique_ptr< Joint > > joints;
    Rofibot* parent;
};

/**
 * \brief
 */
class Rofibot {
public:
    atoms::IdSet< Module > _modules;
    std::vector< std::unique_ptr< ModuleJoint > > _moduleJoints;
    std::vector< std::unique_ptr< SpaceJoint > > _spaceJoints;

    std::unordered_map< ModuleId, std::vector< ModuleJoint* > > _jointMap;
};


// Module buildUniversalModule( double alpha, double beta, double gamma ) {
//     Module m;
//     m.components = {
//         Component( &m, ComponentType::UmShoe ),
//         Component( &m, ComponentType::UmBody ),
//         Component( &m, ComponentType::UmBody ),
//         Component( &m, ComponentType::UmShoe ),
//         Component( &m, ComponentType::Roficom ),
//         Component( &m, ComponentType::Roficom ),
//         Component( &m, ComponentType::Roficom ),
//         Component( &m, ComponentType::Roficom ),
//         Component( &m, ComponentType::Roficom ),
//         Component( &m, ComponentType::Roficom )
//     };
//     m.joints = {
//         IntraModuleJoint(
//             make_unique< RotationalJoint >( t1, ax1, axOr1, -90, 90 ),
//             0, 1 ),
//         IntraModuleJoint(
//             make_unique< RotationalJoint >( t2, ax2, axOr2, -360, 360 ),
//             1, 2),
//         IntraModuleJoint(
//             make_unique< RotationalJoint >( t3, ax3, axOr3, -90, 90 ),
//             2, 3),
//         IntraModuleJoint(
//             make_unique< ShoeConJoint >( XMinus ),
//             0, 4
//         ),
//         IntraModuleJoint(
//             make_unique< ShoeConJoint >( XMinus ),
//             0, 4
//         ),
//         IntraModuleJoint(
//             make_unique< ShoeConJoint >( XMinus ),
//             0, 4
//         ),
//         IntraModuleJoint(
//             make_unique< ShoeConJoint >( XMinus ),
//             0, 4
//         ),
//         IntraModuleJoint(
//             make_unique< ShoeConJoint >( XMinus ),
//             0, 4
//         ),
//         IntraModuleJoint(
//             make_unique< ShoeConJoint >( XMinus ),
//             0, 4
//         )
//     }
//     m.joints[ 0 ].positions = { alpha };
//     m.joints[ 1 ].positions = { beta };
//     m.joints[ 2 ].positions = { gamma };
//     return m;
// }



} // namespace rofi