#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "utils.hpp"

namespace gazebo
{
template < msgs::Joint::Type >
struct Precision
{
};

template <>
struct Precision< msgs::Joint::REVOLUTE >
{
    // Used for position set callback and for boundaries
    // Revolute joints: [rad]
    static constexpr double position = 1e-2;

    // Used for position set callback and for boundaries
    // Revolute joints: [rad/s]
    static constexpr double velocity = 1e-2;
};

template <>
struct Precision< msgs::Joint::PRISMATIC >
{
    // Used for position set callback and for boundaries
    // Prismatic joints: [m]
    static constexpr double position = 1e-4;

    // Used for position set callback and for boundaries
    // Prismatic joints: [m/s]
    static constexpr double velocity = 1e-4;
};

struct JointDataBase
{
    // Used for position set callback and for boundaries
    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double getPositionPrecision() const
    {
        if ( joint->GetMsgType() == msgs::Joint::PRISMATIC )
        {
            return Precision< msgs::Joint::PRISMATIC >::position;
        }
        return Precision< msgs::Joint::REVOLUTE >::position;
    }

    // Used for position set callback and for boundaries
    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getVelocityPrecision() const
    {
        if ( joint->GetMsgType() == msgs::Joint::PRISMATIC )
        {
            return Precision< msgs::Joint::PRISMATIC >::velocity;
        }
        return Precision< msgs::Joint::REVOLUTE >::velocity;
    }

    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double minPosition = std::numeric_limits< double >::lowest();
    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double maxPosition = std::numeric_limits< double >::max();
    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double minSpeed = std::numeric_limits< double >::min();
    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double maxSpeed = std::numeric_limits< double >::max();
    // Prismatic joints: [N]
    // Revolute joints: [Nm]
    double maxEffort = std::numeric_limits< double >::max();

    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double getMaxPosition() const
    {
        return maxPosition - getPositionPrecision();
    }

    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double getMinPosition() const
    {
        return minPosition + getPositionPrecision();
    }

    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getMaxVelocity() const
    {
        return maxSpeed - getVelocityPrecision();
    }

    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getLowestVelocity() const
    {
        return -getMaxVelocity();
    }

    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getMinVelocity() const
    {
        return minSpeed + getVelocityPrecision();
    }

    // Prismatic joints: [N]
    // Revolute joints: [Nm]
    double getMaxEffort() const
    {
        return maxEffort;
    }

    // Prismatic joints: [N]
    // Revolute joints: [Nm]
    double getLowestEffort() const
    {
        return -getMaxEffort();
    }

    physics::JointPtr joint;

    operator bool() const
    {
        return bool( joint );
    }

protected:
    ~JointDataBase() = default;

    JointDataBase( const JointDataBase & ) = default;
    JointDataBase & operator=( const JointDataBase & ) = default;

    explicit JointDataBase( physics::JointPtr jointPtr, sdf::ElementPtr limitSdf )
            : joint( std::move( jointPtr ) )
    {
        if ( !joint )
        {
            return;
        }

        auto limits = joint->GetSDF()->GetElement( "axis" )->GetElement( "limit" );
        limits->GetElement( "lower" )->GetValue()->Get( minPosition );
        limits->GetElement( "upper" )->GetValue()->Get( maxPosition );
        limits->GetElement( "velocity" )->GetValue()->Get( maxSpeed );
        limits->GetElement( "effort" )->GetValue()->Get( maxEffort );

        if ( limitSdf )
        {
            auto lowerSdf = getOnlyChildOrCreate( limitSdf, "lower" );
            if ( !lowerSdf->GetValue() )
            {
                setValue( lowerSdf, minPosition );
            }
            minPosition = lowerSdf->Get< double >();

            auto upperSdf = getOnlyChildOrCreate( limitSdf, "upper" );
            if ( !upperSdf->GetValue() )
            {
                setValue( upperSdf, maxPosition );
            }
            maxPosition = upperSdf->Get< double >();

            auto velocitySdf = getOnlyChildOrCreate( limitSdf, "velocity" );
            if ( !velocitySdf->GetValue() )
            {
                setValue( velocitySdf, maxSpeed );
            }
            maxSpeed = velocitySdf->Get< double >();

            auto effortSdf = getOnlyChildOrCreate( limitSdf, "effort" );
            if ( !effortSdf->GetValue() )
            {
                setValue( effortSdf, maxEffort );
            }
            maxEffort = effortSdf->Get< double >();
        }

        if ( minPosition > maxPosition )
        {
            gzerr << "Maximal position is not larger than minimal\n";
            throw std::runtime_error( "Maximal position is not larger than minimal" );
        }

        if ( maxSpeed < 0 )
        {
            gzmsg << "No max speed in " << joint->GetScopedName() << "\n";
            maxSpeed = std::numeric_limits< double >::max();
        }
        if ( maxEffort < 0 )
        {
            gzmsg << "No max effort in " << joint->GetScopedName() << "\n";
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
    explicit JointData( physics::JointPtr joint, sdf::ElementPtr limitSdf, Args &&... args )
            : JointDataBase( std::move( joint ), std::move( limitSdf ) )
            , controller( *this, std::forward< Args >( args )... )
    {
    }

    ~JointData() = default;
};

} // namespace gazebo
