#pragma once

#include <cmath>
#include <limits>

#include <gz/math/Vector2.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Joint.hh>
#include <sdf/Joint.hh>

#include "utils.hpp"

namespace gazebo
{
template < sdf::JointType >
struct Precision
{
};

template <>
struct Precision< sdf::JointType::REVOLUTE >
{
    static constexpr double position = 1e-2;
    static constexpr double velocity = 1e-2;
};

template <>
struct Precision< sdf::JointType::PRISMATIC >
{
    static constexpr double position = 1e-4;
    static constexpr double velocity = 1e-4;
};

struct JointDataBase
{
    double getPositionPrecision() const
    {
        if ( jointType == sdf::JointType::PRISMATIC )
        {
            return Precision< sdf::JointType::PRISMATIC >::position;
        }
        return Precision< sdf::JointType::REVOLUTE >::position;
    }

    double getVelocityPrecision() const
    {
        if ( jointType == sdf::JointType::PRISMATIC )
        {
            return Precision< sdf::JointType::PRISMATIC >::velocity;
        }
        return Precision< sdf::JointType::REVOLUTE >::velocity;
    }

    double getMaxPosition() const
    {
        return maxPosition - getPositionPrecision();
    }

    double getMinPosition() const
    {
        return minPosition + getPositionPrecision();
    }

    double getMaxVelocity() const
    {
        return maxSpeed - getVelocityPrecision();
    }

    double getLowestVelocity() const
    {
        return -getMaxVelocity();
    }

    double getMinVelocity() const
    {
        return minSpeed + getVelocityPrecision();
    }

    double getMaxEffort() const
    {
        return maxEffort;
    }

    double getLowestEffort() const
    {
        return -getMaxEffort();
    }

    double position( const gz::sim::EntityComponentManager & ecm ) const
    {
        auto current = joint.Position( ecm );
        if ( !current || current->empty() )
        {
            return 0;
        }
        return current->front();
    }

    double velocity( const gz::sim::EntityComponentManager & ecm ) const
    {
        auto current = joint.Velocity( ecm );
        if ( !current || current->empty() )
        {
            return 0;
        }
        return current->front();
    }

    void setForce( gz::sim::EntityComponentManager & ecm, double force )
    {
        joint.SetForce( ecm, { force } );
    }

    void setPositionLimits( gz::sim::EntityComponentManager & ecm, double lower, double upper )
    {
        joint.SetPositionLimits( ecm, { gz::math::Vector2d( lower, upper ) } );
    }

    gz::sim::Joint joint;
    gz::sim::Entity jointEntity = gz::sim::kNullEntity;
    sdf::JointType jointType = sdf::JointType::REVOLUTE;

    double minPosition = std::numeric_limits< double >::lowest();
    double maxPosition = std::numeric_limits< double >::max();
    double minSpeed = 0;
    double maxSpeed = std::numeric_limits< double >::max();
    double maxEffort = std::numeric_limits< double >::max();

    operator bool() const
    {
        return jointEntity != gz::sim::kNullEntity;
    }

protected:
    ~JointDataBase() = default;

    JointDataBase( const JointDataBase & ) = default;
    JointDataBase & operator=( const JointDataBase & ) = default;

    explicit JointDataBase( gz::sim::Entity jointEntity_,
                            sdf::ElementPtr limitSdf,
                            gz::sim::EntityComponentManager & ecm )
            : joint( jointEntity_ )
            , jointEntity( jointEntity_ )
    {
        if ( jointEntity == gz::sim::kNullEntity || !joint.Valid( ecm ) )
        {
            jointEntity = gz::sim::kNullEntity;
            return;
        }

        joint.EnablePositionCheck( ecm );
        joint.EnableVelocityCheck( ecm );
        joint.EnableTransmittedWrenchCheck( ecm );

        auto type = joint.Type( ecm );
        if ( type )
        {
            jointType = *type;
        }

        auto axes = joint.Axis( ecm );
        if ( !axes || axes->empty() )
        {
            throw std::runtime_error( "Joint has no axis information" );
        }

        const auto & axis = axes->front();
        minPosition = axis.Lower();
        maxPosition = axis.Upper();
        maxSpeed = axis.MaxVelocity();
        maxEffort = axis.Effort();

        if ( limitSdf )
        {
            if ( auto lowerSdf = getOnlyChild< false >( limitSdf, "lower" ) )
            {
                minPosition = lowerSdf->Get< double >();
            }
            if ( auto upperSdf = getOnlyChild< false >( limitSdf, "upper" ) )
            {
                maxPosition = upperSdf->Get< double >();
            }
            if ( auto velocitySdf = getOnlyChild< false >( limitSdf, "velocity" ) )
            {
                maxSpeed = velocitySdf->Get< double >();
            }
            if ( auto effortSdf = getOnlyChild< false >( limitSdf, "effort" ) )
            {
                maxEffort = effortSdf->Get< double >();
            }
        }

        if ( minPosition > maxPosition )
        {
            gzerr << "Maximal position is not larger than minimal\n";
            throw std::runtime_error( "Maximal position is not larger than minimal" );
        }

        if ( !std::isfinite( maxSpeed ) || maxSpeed < 0 )
        {
            gzwarn << "No max speed on joint entity " << jointEntity << "\n";
            maxSpeed = std::numeric_limits< double >::max();
        }
        if ( !std::isfinite( maxEffort ) || maxEffort < 0 )
        {
            gzwarn << "No max effort on joint entity " << jointEntity << "\n";
            maxEffort = std::numeric_limits< double >::max();
        }

        assert( maxPosition >= minPosition );
        assert( maxSpeed > getVelocityPrecision() );
        assert( maxEffort > 0 );
    }
};

template < typename Controller >
struct JointData : public JointDataBase
{
    Controller controller;

    template < typename... Args >
    explicit JointData( gz::sim::Entity jointEntity,
                        sdf::ElementPtr limitSdf,
                        gz::sim::EntityComponentManager & ecm,
                        Args &&... args )
            : JointDataBase( jointEntity, std::move( limitSdf ), ecm )
            , controller( *this, ecm, std::forward< Args >( args )... )
    {
    }
};

} // namespace gazebo
