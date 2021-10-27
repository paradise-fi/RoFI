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
    Pad( int sizeN, int sizeM )
    : Module( ModuleType::Pad, _initializeComponents( sizeN * sizeM )
    , sizeN * sizeM, _initializeJoints( sizeN, sizeM ), 0 ) {
        assert( sizeN > 0 && sizeM > 0 && "buildPad has to get positive dimensions" );
    };

    explicit Pad( int size ) : Pad( size, size ) {};

    ~Pad() override = default;

    ATOMS_CLONEABLE( Pad );
};


} // namespace rofi::configuration
