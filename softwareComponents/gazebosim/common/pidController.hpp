#pragma once

#include <array>
#include <cassert>
#include <chrono>
#include <limits>
#include <type_traits>

#include <gz/common/Console.hh>
#include <gz/math/PID.hh>
#include <gz/sim/System.hh>

#include "jointData.hpp"
#include "lorrisConnector.hpp"
#include "pidLoader.hpp"

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
        auto old = _buffer.at( static_cast< size_t >( _pos ) );
        _buffer.at( static_cast< size_t >( _pos ) ) = value;
        _sum += ( value - old ) / static_cast< double >( N );
        _pos = ( _pos + 1 ) % static_cast< int >( N );
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

    bool forceAtPositionBoundary( const gz::sim::EntityComponentManager & ecm ) const
    {
        if ( _targetForce == 0 )
        {
            return false;
        }
        if ( _targetForce < 0 )
        {
            return _jointData.position( ecm ) <= _jointData.getMinPosition();
        }
        return _jointData.position( ecm ) >= _jointData.getMaxPosition();
    }

public:
    explicit ForceController( JointDataBase & jointData ) : _jointData( jointData )
    {
    }

    void forcePhysicsUpdate( gz::sim::EntityComponentManager & ecm )
    {
        if ( _targetForce == 0 )
        {
            return;
        }

        if ( forceAtPositionBoundary( ecm ) )
        {
            gzmsg << "Boundary reached with force: " << _targetForce << ", setting to 0.\n";
            _targetForce = 0;
        }

        _jointData.setForce( ecm, _targetForce );
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
    gz::math::PID _velController;
    detail::Buffer< 5 > _velBuffer;
    double _targetVelocity = 0;

    bool velocityAtPositionBoundary( const gz::sim::EntityComponentManager & ecm ) const
    {
        if ( _targetVelocity == 0 )
        {
            return false;
        }
        if ( _targetVelocity < 0 )
        {
            return _jointData.position( ecm ) <= _jointData.getMinPosition();
        }
        return _jointData.position( ecm ) >= _jointData.getMaxPosition();
    }

    uint8_t _jointId = 0;
    std::optional< uint8_t > _rofiId;
    static constexpr int numOfJointBitsInId = 2;

protected:
    std::optional< uint8_t > getCombinedId() const
    {
        if ( !_rofiId )
        {
            return {};
        }
        return *_rofiId | _jointId;
    }

public:
    VelocityPIDController( JointDataBase & jointData,
                           gz::sim::EntityComponentManager &,
                           const PIDLoader::ControllerValues & pidValues,
                           uint8_t jointId )
            : ForceController( jointData )
            , _velController( pidValues.velocity.getPID( _jointData.getLowestEffort(),
                                                         _jointData.getMaxEffort() ) )
            , _jointId( static_cast< uint8_t >( jointId % ( 1 << numOfJointBitsInId ) ) )
    {
    }

    void setRofiId( int rofiId )
    {
        if ( rofiId < 0 || rofiId >= ( 1 << ( 8 - numOfJointBitsInId ) ) )
        {
            rofiId %= ( 1 << ( 8 - numOfJointBitsInId ) );
        }
        _rofiId = static_cast< uint8_t >( rofiId << numOfJointBitsInId );
    }

    template < bool Verbose = true >
    void velPhysicsUpdate( const gz::sim::UpdateInfo & info, gz::sim::EntityComponentManager & ecm )
    {
        if ( info.paused )
        {
            return;
        }

        if ( velocityAtPositionBoundary( ecm ) )
        {
            if constexpr ( Verbose )
            {
                gzmsg << "Boundary reached with velocity: " << _targetVelocity
                      << ", setting to 0.\n";
            }
            _targetVelocity = 0;
        }

        auto stepTime = std::chrono::duration< double >( info.dt );
        if ( stepTime.count() <= 0 )
        {
            return;
        }

        _velBuffer.update( _jointData.velocity( ecm ) );
        double velocity = _velBuffer.getAverage();
        double linearError = velocity - _targetVelocity;

        double force = _velController.Update( linearError, stepTime );
        _jointData.setForce( ecm, force );

        if ( auto combinedId = getCombinedId() )
        {
            lorris::Packet packet;
            packet.devId() = *combinedId;
            packet.command() = 0;
            packet.push< float >( static_cast< float >( _targetVelocity ) );
            packet.push< float >( static_cast< float >( velocity ) );
            packet.push< float >( static_cast< float >( force ) );
            packet.send();
        }
    }

    void resetVelocityPID( PidControlType lastControlType )
    {
        if ( lastControlType == PidControlType::Position
             || lastControlType == PidControlType::Velocity )
        {
            return;
        }
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
    gz::math::PID _posController;

    double _desiredPosition = 0;
    double _targetPosition = 0;
    double _maxSpeed = 0;
    std::optional< bool > _targetDirection;
    const std::function< void( double ) > _positionReachedCallback;

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
        _maxSpeed = verboseClamp( maxSpeed,
                                  _jointData.getMinVelocity(),
                                  _jointData.getMaxVelocity(),
                                  "maxSpeed" );
        _posController.SetCmdMin( -_maxSpeed );
        _posController.SetCmdMax( _maxSpeed );
        _posController.SetIMin( -_maxSpeed );
        _posController.SetIMax( _maxSpeed );
    }

public:
    template < typename PositionCallback >
    PositionPIDController( JointDataBase & jointData,
                           gz::sim::EntityComponentManager & ecm,
                           const PIDLoader::ControllerValues & pidValues,
                           PositionCallback && positionReached,
                           uint8_t jointId )
            : VelocityPIDController( jointData, ecm, pidValues, jointId )
            , _posController( pidValues.position.getPID( _jointData.getLowestVelocity(),
                                                         _jointData.getMaxVelocity() ) )
            , _positionReachedCallback( std::forward< PositionCallback >( positionReached ) )
    {
        setTargetPosition< false >( _desiredPosition );
    }

    void posPhysicsUpdate( const gz::sim::UpdateInfo & info, gz::sim::EntityComponentManager & ecm )
    {
        if ( info.paused )
        {
            return;
        }

        auto stepTime = std::chrono::duration< double >( info.dt );
        if ( stepTime.count() <= 0 )
        {
            return;
        }

        double position = _jointData.position( ecm );
        double linearError = position - _targetPosition;

        if ( _targetDirection )
        {
            const bool reachedFromPositiveDirection =
                    *_targetDirection && -linearError <= _jointData.getPositionPrecision();
            const bool reachedFromNegativeDirection =
                    !*_targetDirection && linearError <= _jointData.getPositionPrecision();

            if ( reachedFromPositiveDirection || reachedFromNegativeDirection )
            {
                _targetDirection = std::nullopt;
                if ( _positionReachedCallback )
                {
                    _positionReachedCallback( _desiredPosition );
                }
            }
        }

        auto velocity = _posController.Update( linearError, stepTime );
        VelocityPIDController::setTargetVelocity( velocity, std::nullopt );
        VelocityPIDController::velPhysicsUpdate< false >( info, ecm );

        if ( auto combinedId = getCombinedId() )
        {
            lorris::Packet packet;
            packet.devId() = *combinedId;
            packet.command() = 1;
            packet.push< float >( static_cast< float >( _targetPosition ) );
            packet.push< float >( static_cast< float >( position ) );
            packet.push< float >( static_cast< float >( velocity ) );
            packet.send();
        }
    }

    void resetPositionPID( PidControlType lastControlType )
    {
        if ( lastControlType == PidControlType::Position )
        {
            return;
        }
        _posController.Reset();
        VelocityPIDController::resetVelocityPID( lastControlType );
    }

    void setTargetPositionWithSpeed( double targetPosition,
                                     double maxSpeed,
                                     std::optional< PidControlType > lastControlType,
                                     const gz::sim::EntityComponentManager & ecm )
    {
        if ( maxSpeed <= 0 )
        {
            gzwarn << "Speed non-positive for setting position, keeping current position\n";
            setCurrentPosition( lastControlType, ecm );
            return;
        }

        if ( lastControlType )
        {
            resetPositionPID( *lastControlType );
        }

        setMaxSpeed( maxSpeed );
        setTargetPosition( targetPosition );
        _targetDirection = targetPosition > _jointData.position( ecm );
    }

    void setCurrentPosition( std::optional< PidControlType > lastControlType,
                             const gz::sim::EntityComponentManager & ecm )
    {
        if ( lastControlType )
        {
            resetPositionPID( *lastControlType );
        }

        setMaxSpeed( _jointData.getMaxVelocity() );
        setTargetPosition< false >( _jointData.position( ecm ) );
        _targetDirection = std::nullopt;
    }
};

class PIDController
{
    JointDataBase & _jointData;
    PositionPIDController _controller;
    PidControlType activeController = PidControlType::Force;

public:
    PIDController( const PIDController & ) = delete;
    PIDController & operator=( const PIDController & ) = delete;

    template < typename PositionCallback,
               typename = std::enable_if_t< std::is_invocable_v< PositionCallback, double > > >
    PIDController( JointDataBase & jointData,
                   gz::sim::EntityComponentManager & ecm,
                   const PIDLoader::ControllerValues & pidValues,
                   PositionCallback && positionReached,
                   uint8_t jointId )
            : _jointData( jointData )
            , _controller( jointData,
                           ecm,
                           pidValues,
                           std::forward< PositionCallback >( positionReached ),
                           jointId )
    {
        if ( pidValues.position.initTarget )
        {
            auto position = *pidValues.position.initTarget;
            auto speed = pidValues.velocity.initTarget.value_or( _jointData.getMaxVelocity() );
            setTargetPositionWithSpeed( position, speed, ecm );
        }
        else if ( pidValues.velocity.initTarget )
        {
            setTargetVelocity( *pidValues.velocity.initTarget );
        }
        else if ( pidValues.forceTarget )
        {
            setTargetForce( *pidValues.forceTarget );
        }
        else
        {
            _controller.setCurrentPosition( PidControlType::Force, ecm );
            activeController = PidControlType::Position;
        }
    }

    void update( const gz::sim::UpdateInfo & info, gz::sim::EntityComponentManager & ecm )
    {
        switch ( activeController )
        {
            case PidControlType::Force:
                _controller.forcePhysicsUpdate( ecm );
                break;
            case PidControlType::Position:
                _controller.posPhysicsUpdate( info, ecm );
                break;
            case PidControlType::Velocity:
                _controller.velPhysicsUpdate( info, ecm );
                break;
        }
    }

    void setTargetForce( double force )
    {
        _controller.setTargetForce( force );
        activeController = PidControlType::Force;
    }

    void setTargetVelocity( double velocity )
    {
        _controller.setTargetVelocity( velocity, activeController );
        activeController = PidControlType::Velocity;
    }

    void setTargetPositionWithSpeed( double position,
                                     double speed,
                                     const gz::sim::EntityComponentManager & ecm )
    {
        _controller.setTargetPositionWithSpeed( position, speed, activeController, ecm );
        activeController = PidControlType::Position;
    }

    void setRofiId( int rofiId )
    {
        _controller.setRofiId( rofiId );
    }
};

} // namespace gazebo
