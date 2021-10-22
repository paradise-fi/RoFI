#pragma once

#include <array>
#include <iostream>
#include "rofibot.hpp"

namespace rofi::configuration {

class UniversalModule : public Module {
    Angle _getJointAngle( int i ) const {
        return Angle::rad( joints()[ i ].joint->position.value() );
    }

    /**
     * Build an universal module with given angles of the respective joints.
     */
    Module buildUniversalModule( int id, Angle alpha, Angle beta, Angle gamma );

public:
    virtual ~UniversalModule() = default;

    enum UmParts { UmBodyA = 7, UmBodyB = 8, UmShoeA = 6, UmShoeB = 9 };

    explicit UniversalModule( int id ) : UniversalModule( id, 0_deg, 0_deg, 0_deg ) {};
    UniversalModule( int id, Angle a, Angle b, Angle g ) : Module( buildUniversalModule( id, a, b, g ) ) {};

    Angle getAlpha() const {
        return _getJointAngle( 0 );
    }
    Angle getBeta()  const {
        return _getJointAngle( 1 );
    }
    Angle getGamma() const {
        return _getJointAngle( 2 );
    }

    Angle setAlpha( Angle a ) {
        setJointParams( 0, a.rad() );
        return getAlpha();
    }

    Angle setBeta( Angle a ) {
        setJointParams( 1, a.rad() );
        return getBeta();
    }
    Angle setGamma( Angle a ) {
        setJointParams( 2, a.rad() );
        return getGamma();
    }
};


/**
 * Given a stream read a configuration from the old "Viki" format. Unlike the
 * original parser, supports only a single configuration per file.
 */
Rofibot readOldConfigurationFormat( std::istream& s );

} // namespace rofi::configuration

