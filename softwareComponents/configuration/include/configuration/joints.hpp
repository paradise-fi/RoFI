#pragma once

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cassert>
#include <optional>
#include <ranges>
#include <span>

#include <atoms/patterns.hpp>
#include <atoms/units.hpp>
#include "../Matrix.h"

namespace rofi::configuration {

class RigidJoint;
class RotationJoint;

using JointVisitor = atoms::Visits<
    RigidJoint,
    RotationJoint >;

/**
 * \brief Joint between two coordinate systems
 *
 * Joints represents a parametric description of positional relationship between
 * two coordinate systems (e.g., component origins). This class in abstract. See
 * below for concrete instances.
 *
 * Each joint has Joint::paramCount() parameters (real value), which are stored
 * in Joint::position. These params are, e.g., angle for rotation joint.
 */
struct Joint: public atoms::VisitableBase< Joint, JointVisitor > {
    explicit Joint( std::vector< std::pair< float, float > > jointLimits )
            : _jointLimits( std::move( jointLimits ) ), _positions( _jointLimits.size(), 0 )  {}
    virtual ~Joint() = default;

    ATOMS_CLONEABLE_BASE( Joint );

    std::span< const std::pair< float, float > > jointLimits() const {
        return _jointLimits;
    }

    std::span< const float > positions() const {
        return _positions;
    }

    void setPositions( std::span< const float > pos ) {
        assert( pos.size() == _positions.size() && "Positions have to preserve given size" );
        std::copy( pos.begin(), pos.end(), _positions.begin() );
    }

    virtual Matrix sourceToDest() const = 0;

    virtual Matrix destToSource() const {
        return arma::inv( sourceToDest() );
    };

    friend std::ostream& operator<<( std::ostream& out, Joint& j );
private:
    std::vector< std::pair< float, float > > _jointLimits;
    std::vector< float > _positions;
};

struct RigidJoint: public atoms::Visitable< Joint, RigidJoint > {
    /**
     * Construct a rigid joint based on transformation between source and dest.
     *
     * Example usage: `RigidJoint( rotate( M_PI/2, { 1, 0, 0, 1 } ) * translate( { 20, 0, 0 } ) )`
     */
    RigidJoint( const Matrix& sToDest )
        : Visitable( std::vector< std::pair< float, float > >{} ),
          _sourceToDest( sToDest ),
          _destToSource( arma::inv( sToDest ) )
    {}

    Matrix sourceToDest() const override {
        return _sourceToDest;
    }

    Matrix destToSource() const override {
        return _destToSource; // ToDo: Find out if the precomputing is effective or not
    }

    ATOMS_CLONEABLE( RigidJoint );

private:
    Matrix _sourceToDest;
    Matrix _destToSource;
};

struct RotationJoint: public atoms::Visitable< Joint, RotationJoint > {
    /**
     * Construct rotation joint by specifing the same axis and origin point in
     * source coordinate system and destination coordinate system.
     */
    RotationJoint( Vector sourceOrigin, Vector sourceAxis, Vector destOrigin,
                   Vector desAxis, Angle min, Angle max )
        : RotationJoint( translate( sourceOrigin ), desAxis - sourceAxis,
                         translate( destOrigin ), min, max )
    {}

    /**
     * Construct rotation joint by specifying transformation before rotation,
     * followed by rotation axis with origin in (0, 0, 0), followed by another
     * transformation.
     */
    RotationJoint( Matrix pre, Vector axis, Matrix post, Angle min, Angle max )
        : Visitable( std::vector{ std::pair{ min.rad(), max.rad() } } ),
          _axis( axis ),
          _pre( pre ),
          _post( post )
    {}

    Matrix sourceToDest() const override {
        return _pre * rotate( position().rad(), _axis ) * _post;
    }

    Matrix destToSource() const override {
        return arma::inv( sourceToDest() ); // ToDo: Find out if this is effective enough
    }

    Angle position() const {
        return Angle::rad( positions()[ 0 ] );
    }

    void setPosition( Angle pos ) {
        auto positions = std::array{ pos.rad() };
        return setPositions( positions );
    }

    std::pair< Angle, Angle > jointLimit() const {
        auto limits = jointLimits()[ 0 ];
        return std::pair( Angle::rad( limits.first ), Angle::rad( limits.second ) );
    }

    const Matrix pre() const  { return _pre;  }
    const Matrix post() const { return _post; }
    const Vector axis() const { return _axis; }


    ATOMS_CLONEABLE( RotationJoint );

    friend std::ostream& operator<<( std::ostream& out, Joint& j );
private:
    Vector _axis; ///< axis of rotation around with origin in point (0, 0, 0)
    Matrix _pre; ///< transformation to apply before rotating
    Matrix _post; ///< transformation to apply after rotating
};

/**
 * Provides human readable text description of the joint
 */
std::ostream& operator<<( std::ostream& out, Joint& j );

} // namespace rofi::configuration
