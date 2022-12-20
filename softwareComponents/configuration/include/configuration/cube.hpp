#pragma once

#include <configuration/rofiworld.hpp>

namespace rofi::configuration {

using namespace rofi::configuration::matrices;

class Cube : public Module {

    static std::vector< Component > _initComponents(){
        auto components = std::vector< Component >( 6, Component { ComponentType::Roficom, {}, {}, nullptr } );
        components.push_back( Component{ ComponentType::CubeBody, {}, {}, nullptr } );
        return components;
    }

    static std::vector< ComponentJoint > _initJoints(){
        std::vector< ComponentJoint > joints = {
            makeComponentJoint< RigidJoint >( 6, 0, identity ),
            makeComponentJoint< RigidJoint >( 6, 1, rotate( Angle::pi, { 0, 1, 0 } ) ),
            makeComponentJoint< RigidJoint >( 6, 2, rotate( Angle::pi / 2, { 0, 1, 0 } ) ),
            makeComponentJoint< RigidJoint >( 6, 3, rotate( Angle::pi / 2, { 0, -1, 0 } ) ),
            makeComponentJoint< RigidJoint >( 6, 4, rotate( Angle::pi / 2, { 0, 0, 1 } ) ),
            makeComponentJoint< RigidJoint >( 6, 5, rotate( Angle::pi / 2, { 0, 0, -1 } ) ),
        };
        return joints;
    }

  public:
    Cube( int id ) : Module( ModuleType::Cube, _initComponents(), 6, _initJoints(), id ){}

    ATOMS_CLONEABLE( Cube );
};


} // namespace rofi::configuration
