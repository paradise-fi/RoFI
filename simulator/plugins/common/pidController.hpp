#pragma once

#include <array>
#include <cassert>
#include <limits>
#include <type_traits>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "lorrisConnector.hpp"
#include "pidLoader.hpp"


constexpr size_t BufferSize = 5;

namespace gazebo
{
namespace detail
{
template < size_t N >
class Buffer
{
    std::array< double, N > _buffer = {};
    int _pos = 0;
    double _sum = 0;

public:
    void update( double value )
    {
        assert( _pos >= 0 );
        assert( static_cast< size_t >( _pos ) < _buffer.size() );

        auto old = _buffer[ _pos ];
        _buffer[ _pos ] = value;
        _sum += ( value - old ) / _buffer.size();
        _pos = ( _pos + 1 ) % _buffer.size();
    }

    double getAverage() const
    {
        return _sum;
    }
};
} // namespace detail

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
    ForceController( JointDataBase & jointData ) : _jointData( jointData )
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
        _targetForce = verboseClamp( targetForce,
                                     _jointData.getLowestEffort(),
                                     _jointData.getMaxEffort(),
                                     "targetForce" );
    }
};

class VelocityPIDController : public ForceController
{
    common::PID _velController;
    common::Time _velPrevUpdateTime;

    detail::Buffer< BufferSize > _velBuffer;
    double _targetVelocity = 0;

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
    VelocityPIDController( JointDataBase & jointData,
                           const PIDLoader::ControllerValues & pidValues )
            : ForceController( jointData )
            , _velController( pidValues.getVelocity().getPID( _jointData.getMaxEffort(),
                                                              _jointData.getLowestEffort() ) )
    {
        assert( _jointData );
        _velPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();
    }

    template < bool Verbose = true >
    void velPhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );

        if ( velocityAtPositionBoundary() )
        {
            if constexpr ( Verbose )
            {
                gzmsg << "Boundary reached with velocity: " << _targetVelocity
                      << ", setting to 0.\n";
            }
            _targetVelocity = 0;
        }

        common::Time currTime = _jointData.joint->GetWorld()->SimTime();
        common::Time stepTime = currTime - _velPrevUpdateTime;
        _velPrevUpdateTime = currTime;
        assert( stepTime >= 0 && "time went backwards" );

        _velBuffer.update( _jointData.joint->GetVelocity( 0 ) );
        double velocity = _velBuffer.getAverage();
        double linearError = velocity - _targetVelocity;

        double force = _velController.Update( linearError, stepTime );
        assert( force <= _jointData.maxEffort );
        assert( force >= -_jointData.maxEffort );

        _jointData.joint->SetForce( 0, force );

        char id = 'C';
        if ( _jointData.joint->GetName() == "shoeARev" )
        {
            id = 'A';
        }
        else if ( _jointData.joint->GetName() == "shoeBRev" )
        {
            id = 'B';
        }
        else if ( _jointData.joint->GetName() == "bodyRev" )
        {
            id = 'R';
        }
        lorris::Packet packet;
        packet.devId() = id;
        packet.command() = 0;
        packet.push< float >( _targetVelocity ); // Target
        packet.push< float >( velocity );        // Current
        packet.push< float >( force );           // Output
        packet.send();
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
    }

    void setTargetVelocity( double targetVelocity, std::optional< PidControlType > lastControlType )
    {
        _targetVelocity = verboseClamp( targetVelocity,
                                        _jointData.getLowestVelocity(),
                                        _jointData.getMaxVelocity(),
                                        "targetVelocity" );

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

    detail::Buffer< BufferSize > _posBuffer;
    double _desiredPosition = 0;
    double _targetPosition = 0;
    double _maxSpeed = 0;
    const std::function< void( double ) > _positionReachedCallback;
    bool _positionReached = true;

    template < bool Verbose = true >
    void setTargetPosition( double desiredPosition )
    {
        _desiredPosition = desiredPosition;
        if constexpr ( Verbose )
        {
            _targetPosition = verboseClamp( _desiredPosition,
                                            _jointData.getMinPosition(),
                                            _jointData.getMaxPosition(),
                                            "targetPosition" );
        }
        else
        {
            _targetPosition = clamp( _desiredPosition,
                                     _jointData.getMinPosition(),
                                     _jointData.getMaxPosition() );
        }
    }

    void setMaxSpeed( double maxSpeed )
    {
        assert( _jointData.getMinVelocity() >= _jointData.velocityPrecision );

        _maxSpeed = verboseClamp( maxSpeed,
                                  _jointData.getMinVelocity(),
                                  _jointData.getMaxVelocity(),
                                  "maxSpeed" );
        _posController.SetCmdMin( -_maxSpeed );
        _posController.SetCmdMax( _maxSpeed );
    }

public:
    template < typename Callback >
    PositionPIDController( JointDataBase & jointData,
                           const PIDLoader::ControllerValues & pidValues,
                           Callback && positionReachedCallback )
            : VelocityPIDController( jointData, pidValues )
            , _posController( pidValues.getPosition().getPID( _jointData.getLowestVelocity(),
                                                              _jointData.getMaxVelocity() ) )
            , _positionReachedCallback( std::forward< Callback >( positionReachedCallback ) )
    {
        assert( _jointData );
        assert( _jointData.getLowestVelocity() == -_jointData.getMaxVelocity() );
        _posPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();
        setTargetPosition< false >( _desiredPosition );
    }

    void posPhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );

        common::Time currTime = _jointData.joint->GetWorld()->SimTime();
        common::Time stepTime = currTime - _posPrevUpdateTime;
        _posPrevUpdateTime = currTime;
        assert( stepTime >= 0 && "time went backwards" );

        _posBuffer.update( _jointData.joint->Position( 0 ) );
        double position = _posBuffer.getAverage();
        double linearError = position - _targetPosition;

        if ( !_positionReached && std::abs( linearError ) <= _jointData.positionPrecision )
        {
            _positionReached = true;

            if ( _positionReachedCallback )
            {
                assert( _targetPosition
                        == clamp( _desiredPosition,
                                  _jointData.getMinPosition(),
                                  _jointData.getMaxPosition() ) );
                _positionReachedCallback( _desiredPosition );
            }
            else
            {
                gzwarn << "Position reached, but callback not set\n";
            }
        }

        auto velocity = _posController.Update( linearError, stepTime );
        assert( _maxSpeed >= _jointData.getMinVelocity() );
        assert( _maxSpeed <= _jointData.getMaxVelocity() );

        assert( std::abs( velocity ) <= _maxSpeed + _jointData.positionPrecision );

        VelocityPIDController::setTargetVelocity( velocity, std::nullopt );
        VelocityPIDController::velPhysicsUpdate< false >();


        char id = 'C';
        if ( _jointData.joint->GetName() == "shoeARev" )
        {
            id = 'A';
        }
        else if ( _jointData.joint->GetName() == "shoeBRev" )
        {
            id = 'B';
        }
        else if ( _jointData.joint->GetName() == "bodyRev" )
        {
            id = 'R';
        }
        lorris::Packet packet;
        packet.devId() = id;
        packet.command() = 1;
        packet.push< float >( _targetPosition ); // Target
        packet.push< float >( position );        // Current
        packet.push< float >( velocity );        // Output
        packet.send();
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

    void setTargetPositionWithSpeed( double targetPosition,
                                     double maxSpeed,
                                     std::optional< PidControlType > lastControlType )
    {
        if ( maxSpeed > 0 )
        {
            setTargetPosition( targetPosition );
            _positionReached = false;
        }
        else
        {
            maxSpeed = _jointData.getMinVelocity();
            gzwarn << "Speed non-positive for setting position, setting desired position to current position with speed "
                   << maxSpeed << "\n";
            setTargetPosition< false >( _jointData.joint->Position( 0 ) );
            _positionReached = true;
        }
        setMaxSpeed( maxSpeed );

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
    template < typename Callback,
               typename = std::enable_if_t< std::is_invocable_v< Callback, double > > >
    PIDController( JointDataBase & jointData,
                   const PIDLoader::ControllerValues & pidValues,
                   Callback && positionReachedCallback )
            : _jointData( jointData )
            , controller( jointData,
                          pidValues,
                          std::forward< Callback >( positionReachedCallback ) )
    {
        _connection = event::Events::ConnectBeforePhysicsUpdate(
                std::bind( &PIDController::onPhysicsUpdate, this ) );

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
            setTargetPositionWithSpeed( _jointData.joint->Position( 0 ),
                                        _jointData.getMaxVelocity() );
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
