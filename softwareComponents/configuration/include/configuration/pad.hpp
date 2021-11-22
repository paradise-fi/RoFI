#pragma once

#include <array>
#include <iostream>
#include <configuration/rofibot.hpp>

namespace rofi::configuration {

class Pad : public Module {

    static std::vector< Component > _initializeComponents( int size ) {
        return std::vector< Component >( size, Component { ComponentType::Roficom } );
    }

    static std::vector< ComponentJoint > _initializeJoints( int n, int m ) {
        std::vector< ComponentJoint > joints;
        for ( int i = 0; i < n; i++ ) {
            for ( int j = 0; j < m; j++ ) {
                if ( i > 0 ) {
                    joints.push_back(
                        makeComponentJoint< RigidJoint >( (i - 1) * m + j, i * m + j, translate( { 0, 1, 0 } ) )
                    );
                }

                if ( j > 0 ) {
                    joints.push_back(
                        makeComponentJoint< RigidJoint >( i * m + j - 1, i * m + j, translate( { 0, 0, 1 } ) )
                    );
                }
            }
        }
        return joints;
    }

public:
    const int width;
    const int height;

    Pad( ModuleId id, int width, int height )
    : Module( ModuleType::Pad, _initializeComponents( width * height )
    , width * height, _initializeJoints( width, height ), id ), width( width ), height( height ) {
        assert( width > 0 && height > 0 && "buildPad has to get positive dimensions" );
    };

    explicit Pad( ModuleId id, int size ) : Pad( id, size, size ) {};

    ATOMS_CLONEABLE( Pad );
};


} // namespace rofi::configuration
