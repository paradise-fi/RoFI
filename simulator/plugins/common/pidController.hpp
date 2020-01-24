#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include <cassert>
#include <type_traits>
#include <limits>

#include "utils.hpp"
#include "pidLoader.hpp"

namespace gazebo
{

enum class PidControlType
{
    Force,
    Position,
    Velocity,
};

class ForceController
{
protected:
    JointDataBase & _jointData;

private:
    double _targetForce = 0;

    bool forceAtPositionBoundary() const
    {
        assert( _jointData );
        assert( _jointData.joint );

        if ( _targetForce == 0 )
        {
            return false;
        }
        if ( _targetForce < 0 )
        {
            return _jointData.joint->Position( 0 ) <= _jointData.getMinPosition();
        }
        return _jointData.joint->Position( 0 ) >= _jointData.getMaxPosition();
    }

public:
    ForceController( JointDataBase & jointData ) :
            _jointData( jointData )
    {
        assert( _jointData );

    }

    void forcePhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );

        if ( _targetForce == 0 )
        {
            return;
        }

        if ( forceAtPositionBoundary() )
        {
            gzmsg << "Boundary reached with force: " << _targetForce << ", setting to 0.\n";
            _targetForce = 0;
        }

        assert( _targetForce <= _jointData.maxEffort );
        assert( _targetForce >= -_jointData.maxEffort );
        _jointData.joint->SetForce( 0, _targetForce );
    }

    void setTargetForce( double targetForce )
    {
        _targetForce = verboseClamp( targetForce, _jointData.getLowestEffort(), _jointData.getMaxEffort(), "targetForce" );
    }
};

class VelocityPIDController : public ForceController
{
    common::PID _velController;
    common::Time _velPrevUpdateTime;

    double _targetVelocity = 0;
    double _lastForceApplied = 0;

    void updateLastForceApplied( double newForce )
    {
        assert( newForce <= _jointData.maxEffort );
        assert( newForce >= -_jointData.maxEffort );
        _lastForceApplied = newForce;
        _velController.SetCmdMin( _jointData.getLowestEffort() - _lastForceApplied );
        _velController.SetCmdMax( _jointData.getMaxEffort() - _lastForceApplied );
    }

    bool velocityAtPositionBoundary() const
    {
        assert( _jointData );
        assert( _jointData.joint );

        if ( _targetVelocity == 0 )
        {
            return false;
        }
        if ( _targetVelocity < 0 )
        {
            return _jointData.joint->Position( 0 ) <= _jointData.getMinPosition();
        }
        return _jointData.joint->Position( 0 ) >= _jointData.getMaxPosition();
    }

public:
    VelocityPIDController( JointDataBase & jointData, const PIDLoader::ControllerValues & pidValues ) :
            ForceController( jointData ),
            _velController( pidValues.getVelocity().getPID( _jointData.getMaxEffort(), _jointData.getLowestEffort() ) )
    {
        assert( _jointData );
        _velPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();
    }

    void velPhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );

        common::Time currTime = _jointData.joint->GetWorld()->SimTime();
        common::Time stepTime = currTime - _velPrevUpdateTime;
        _velPrevUpdateTime = currTime;
        assert( stepTime > 0 && "time went backwards" );

        if ( velocityAtPositionBoundary() )
        {
            gzmsg << "Boundary reached with velocity: " << _targetVelocity << ", setting to 0.\n";
            _targetVelocity = 0;
        }

        double linearError = _jointData.joint->GetVelocity( 0 ) - _targetVelocity;

        auto force = _lastForceApplied + _velController.Update( linearError, stepTime );
        assert( force <= _jointData.maxEffort );
        assert( force >= -_jointData.maxEffort );

        _jointData.joint->SetForce( 0, force );
        updateLastForceApplied( force );
    }

    void resetVelocityPID( PidControlType lastControlType )
    {
        if ( lastControlType == PidControlType::Velocity )
        {
            return;
        }
        if ( lastControlType == PidControlType::Position )
        {
            return;
        }

        _velPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();
        _velController.Reset();
        updateLastForceApplied( 0 );
    }

    void setTargetVelocity( double targetVelocity, std::optional< PidControlType > lastControlType )
    {
        _targetVelocity = verboseClamp( targetVelocity, _jointData.getLowestVelocity(), _jointData.getMaxVelocity(), "targetVelocity" );

        if ( lastControlType )
        {
            resetVelocityPID( *lastControlType );
        }
    }
};

class PositionPIDController : public VelocityPIDController
{
    common::PID _posController;
    common::Time _posPrevUpdateTime;

    double _desiredPosition = 0;
    double _targetPosition = 0;
    double _maxSpeed = 0;
    const std::function< void( double ) > _positionReachedCallback;
    bool _positionReached = true;

public:
    template< typename Callback >
    PositionPIDController( JointDataBase & jointData,
                           const PIDLoader::ControllerValues & pidValues,
                           Callback && positionReachedCallback ) :
            VelocityPIDController( jointData, pidValues ),
            _posController( pidValues.getPosition().getPID( _jointData.getLowestVelocity(), _jointData.getMaxVelocity() ) ),
            _positionReachedCallback( std::forward< Callback >( positionReachedCallback ) )
    {
        _posPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();
    }

    void posPhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );

        common::Time currTime = _jointData.joint->GetWorld()->SimTime();
        common::Time stepTime = currTime - _posPrevUpdateTime;
        _posPrevUpdateTime = currTime;

        double linearError = _jointData.joint->Position( 0 ) - _targetPosition;

        if ( !_positionReached && std::abs( linearError ) <= _jointData.positionPrecision )
        {
            _positionReached = true;

            if ( _positionReachedCallback )
            {
                assert( _targetPosition ==
                        clamp( _desiredPosition, _jointData.getMinPosition(), _jointData.getMaxPosition() ) );
                _positionReachedCallback( _desiredPosition );
            }
            else
            {
                gzwarn << "Position reached callback not set\n";
            }
        }

        // TODO account for last velocity and update cmd bounds
        auto vel = _posController.Update( linearError, stepTime );
        assert( _maxSpeed >= 0 );
        assert( _maxSpeed <= _jointData.maxSpeed );
        assert( vel <= -_maxSpeed );
        assert( vel >= _maxSpeed );
        VelocityPIDController::setTargetVelocity( vel, std::nullopt );
        VelocityPIDController::velPhysicsUpdate();
    }

    void resetPositionPID( PidControlType lastControlType )
    {
        if ( lastControlType == PidControlType::Position )
        {
            return;
        }

        _posPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();
        _posController.Reset();

        VelocityPIDController::resetVelocityPID( lastControlType );
    }

    void setTargetPositionWithSpeed( double targetPosition, double maxSpeed, std::optional< PidControlType > lastControlType )
    {
        _desiredPosition = targetPosition;
        _targetPosition = verboseClamp( targetPosition, _jointData.getMinPosition(), _jointData.getMaxPosition(), "targetPosition" );
        _positionReached = false;

        if ( maxSpeed <= 0 )
        {
            gzwarn << "Speed non-positive for setting position, setting desired position to actual position\n";
            _targetPosition = _jointData.joint->Position( 0 );
            _positionReached = true;
        }
        _maxSpeed = verboseClamp( maxSpeed, _jointData.getMinVelocity(), _jointData.getMaxVelocity(), "speed" );
        _posController.SetCmdMin( -_maxSpeed );
        _posController.SetCmdMax( _maxSpeed );

        if ( lastControlType )
        {
            resetPositionPID( *lastControlType );
        }
    }
};

class PIDController
{
    event::ConnectionPtr _connection;

    JointDataBase & _jointData;
    PositionPIDController controller;

    PidControlType activeController = PidControlType::Force;

    void onPhysicsUpdate()
    {
        switch ( activeController )
        {
            case PidControlType::Force:
                controller.forcePhysicsUpdate();
                return;
            case PidControlType::Position:
                controller.posPhysicsUpdate();
                return;
            case PidControlType::Velocity:
                controller.velPhysicsUpdate();
                return;
        }
        assert( false && "unrecognized pid control type value" );
    }


public:
    template< typename Callback, typename = std::enable_if_t< std::is_invocable_v< Callback, double > > >
    PIDController( JointDataBase & jointData, const PIDLoader::ControllerValues & pidValues, Callback && positionReachedCallback )
        : _jointData( jointData ),
          controller( jointData, pidValues, std::forward< Callback >( positionReachedCallback ) )
    {
        _connection = event::Events::ConnectBeforePhysicsUpdate( std::bind( &PIDController::onPhysicsUpdate, this ) );

        if ( pidValues.getPosition().initTarget )
        {
            auto position = *pidValues.getPosition().initTarget;
            auto speed = pidValues.getVelocity().initTarget.value_or( _jointData.getMaxVelocity() );
            setTargetPositionWithSpeed( position, speed );
        }
        else if ( pidValues.getVelocity().initTarget )
        {
            setTargetVelocity( *pidValues.getVelocity().initTarget );
        }
        else if ( pidValues.forceTarget )
        {
            setTargetForce( *pidValues.forceTarget );
        }
        else
        {
            setTargetPositionWithSpeed( _jointData.joint->Position( 0 ), _jointData.getMaxVelocity() );
        }
    }

    void setTargetForce( double force )
    {
        controller.setTargetForce( force );
        activeController = PidControlType::Force;
    }

    void setTargetPositionWithSpeed( double position, double speed )
    {
        controller.setTargetPositionWithSpeed( position, speed, activeController );
        activeController = PidControlType::Position;
    }

    void setTargetVelocity( double velocity )
    {
        controller.setTargetVelocity( velocity, activeController );
        activeController = PidControlType::Velocity;
    }
};

} // namespace gazebo
