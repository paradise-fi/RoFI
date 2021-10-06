#pragma once

#include <stdexcept>
#include <iostream>
#include <cassert>

#include <atoms/patterns.hpp>
#include <atoms/units.hpp>
#include "Matrix.h"

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
 * in Joint::positions. These params are, e.g., angle for rotation joint.
 */
struct Joint: public atoms::VisitableBase< Joint, JointVisitor > {
    virtual ~Joint() = default;

    using Positions = std::vector< Angle >;

    virtual int paramCount() const = 0;
    virtual Matrix sourceToDest() const = 0;
    virtual std::pair< Angle, Angle > jointLimits( int paramIdx ) const = 0;
    ATOMS_CLONEABLE_BASE( Joint );

    virtual Matrix destToSource() const {
        return arma::inv( sourceToDest() );
    };

    Matrix sourceToDest( const Positions& pos ) {
        positions = pos;
        return sourceToDest();
    }

    Matrix destToSource( const Positions& pos ) {
        positions = pos;
        return destToSource();
    }

    Positions positions;
    friend std::ostream& operator<<( std::ostream& out, Joint& j );
};

struct RigidJoint: public atoms::Visitable< Joint, RigidJoint > {
    /**
     * Construct a rigid joint based on transformation between source and dest.
     *
     * Example usage: `RigidJoint( rotate( M_PI/2, { 1, 0, 0, 1 } ) * translate( { 20, 0, 0 } ) )`
     */
    RigidJoint( const Matrix& sToDest ):
        _sourceToDest( sToDest ),
        _destToSource( arma::inv( sToDest ) )
    {}

    int paramCount() const override {
        return 0;
    }

    Matrix sourceToDest() const override {
        assert( positions.size() == 0 );
        return _sourceToDest;
    }

    Matrix destToSource() const override {
        assert( positions.size() == 0 );
        return _destToSource; // ToDo: Find out if the precomputing is effective or not
    }

    std::pair< Angle, Angle > jointLimits( int /*paramIdx*/ ) const override {
        throw std::logic_error( "Rigid joint has no parameters" );
    }

    ATOMS_CLONEABLE( RigidJoint );

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
        : _limits( { min, max } )
    {
        _pre  = translate( sourceOrigin );
        _post = translate( destOrigin );
        _axis = desAxis - sourceAxis;
    }

    /**
     * Construct rotation joint by specifying transformation before rotation,
     * followed by rotation axis with origin in (0, 0, 0), followed by another
     * transformation.
     */
    RotationJoint( Matrix pre, Vector axis, Matrix post, Angle min, Angle max )
        : _limits( { min, max } ),
          _axis( axis ),
          _pre( pre ),
          _post( post )
    {}

    int paramCount() const override {
        return 1;
    }

    Matrix sourceToDest() const override {
        assert( positions.size() == 1 );
        return _pre * rotate( positions[ 0 ].rad(), _axis ) * _post;
    }

    Matrix destToSource() const override {
        assert( positions.size() == 1 );
        return arma::inv( sourceToDest() ); // ToDo: Find out if this is effective enough
    }

    std::pair< Angle, Angle > jointLimits( int paramIdx ) const override {
        if ( paramIdx != 0 )
            throw std::logic_error( "RotationJoint joint has only parameter 0" );
        return _limits;
    }

    ATOMS_CLONEABLE( RotationJoint );

    std::pair< Angle, Angle > _limits; ///< minimal and maximal parameter limit
    Vector _axis; ///< axis of rotation around with origin in point (0, 0, 0)
    Matrix _pre; ///< transformation to apply before rotating
    Matrix _post; ///< transformation to apply after rotating
};

/**
 * Provides human readable text description of the joint
 */
std::ostream& operator<<( std::ostream& out, Joint& j );

} // namespace rofi::configuration
