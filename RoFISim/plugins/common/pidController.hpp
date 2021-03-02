#pragma once

#include <array>
#include <cassert>
#include <limits>
#include <type_traits>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

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
            return _jointData.joint->Position() <= _jointData.getMinPosition();
        }
        return _jointData.joint->Position() >= _jointData.getMaxPosition();
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

    detail::Buffer< 5 > _velBuffer;
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
            return _jointData.joint->Position() <= _jointData.getMinPosition();
        }
        return _jointData.joint->Position() >= _jointData.getMaxPosition();
    }

    unsigned char _jointId = 0;
    std::optional< unsigned char > _rofiId;
    static constexpr int numOfJointBitsInId = 2;
    static_assert( numOfJointBitsInId > 0 && numOfJointBitsInId < 8 );

protected:
    std::optional< unsigned char > getCombinedId() const
    {
        if ( !_rofiId )
        {
            return {};
        }

        [[maybe_unused]] constexpr auto jointBitsMask = ( 1 << numOfJointBitsInId ) - 1;
        assert( ( *_rofiId & jointBitsMask ) == 0 );
        assert( ( _jointId & ~jointBitsMask ) == 0 );

        return *_rofiId | _jointId;
    }

public:
    VelocityPIDController( JointDataBase & jointData,
                           const PIDLoader::ControllerValues & pidValues,
                           int jointId )
            : ForceController( jointData )
            , _velController( pidValues.velocity.getPID( _jointData.getLowestEffort(),
                                                         _jointData.getMaxEffort() ) )
            , _jointId( jointId % ( 1 << numOfJointBitsInId ) )
    {
        assert( _jointData );
        _velPrevUpdateTime = _jointData.joint->GetWorld()->SimTime();

        if ( static_cast< int >( _jointId ) != jointId )
        {
            gzwarn << "Lorris will use joint id '" << _jointId << "' instead of '" << jointId
                   << "'\n";
        }
    }

    void setRofiId( int rofiId )
    {
        gzwarn << "Setting RoFI Id to '" << rofiId << "'\n";
        if ( _rofiId )
        {
            gzwarn << "RoFI Id in controller already set\n";
        }

        static_assert( numOfJointBitsInId > 0 && numOfJointBitsInId < 8 );

        if ( rofiId < 0 || rofiId >= ( 1 << ( 8 - numOfJointBitsInId ) ) )
        {
            int newRofiId = rofiId % ( 1 << ( 8 - numOfJointBitsInId ) );
            assert( newRofiId >= 0 );

            gzwarn << "For lorris, RoFI Id '" << rofiId << "' is changed to '" << newRofiId
                   << "'\n";
            rofiId = newRofiId;
        }

        {
            using IdLimits = std::numeric_limits< decltype( _rofiId )::value_type >;
            assert( rofiId >= IdLimits::min() );
            assert( rofiId <= IdLimits::max() );
            assert( rofiId << numOfJointBitsInId >= IdLimits::min() );
            assert( rofiId << numOfJointBitsInId <= IdLimits::max() );
        }

        _rofiId = rofiId << numOfJointBitsInId;
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

        auto combinedId = getCombinedId();
        if ( combinedId )
        {
            lorris::Packet packet;
            packet.devId() = *combinedId;
            packet.command() = 0;
            packet.push< float >( _targetVelocity ); // Target
            packet.push< float >( velocity );        // Current
            packet.push< float >( force );           // Output
            packet.send();
        }
    }

    void resetVelocityPID( PidControlType lastControlType )
    {
        switch ( lastControlType )
        {
            case PidControlType::Position:
            case PidControlType::Velocity:
                return;
            case PidControlType::Force:
                break;
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

    double _desiredPosition = 0;
    double _targetPosition = 0;
    double _maxSpeed = 0;
    std::optional< bool > _targetDirection; // If _targetDirection is nullopt, position is reached
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
        assert( _jointData.getMinVelocity() >= _jointData.getVelocityPrecision() );

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
                           const PIDLoader::ControllerValues & pidValues,
                           PositionCallback && positionReached,
                           int jointId )
            : VelocityPIDController( jointData, pidValues, jointId )
            , _posController( pidValues.position.getPID( _jointData.getLowestVelocity(),
                                                         _jointData.getMaxVelocity() ) )
            , _positionReachedCallback( std::forward< PositionCallback >( positionReached ) )
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

        double position = _jointData.joint->Position();
        double linearError = position - _targetPosition;

        if ( _targetDirection )
        {
            if ( ( *_targetDirection && -linearError <= _jointData.getPositionPrecision() )
                 || ( !*_targetDirection && linearError <= _jointData.getPositionPrecision() ) )
            {
                _targetDirection = std::nullopt;

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
        }

        auto velocity = _posController.Update( linearError, stepTime );
        assert( _maxSpeed >= _jointData.getMinVelocity() );
        assert( _maxSpeed <= _jointData.getMaxVelocity() );

        assert( std::abs( velocity ) <= _maxSpeed + _jointData.getPositionPrecision() );

        VelocityPIDController::setTargetVelocity( velocity, std::nullopt );
        VelocityPIDController::velPhysicsUpdate< false >();

        auto combinedId = getCombinedId();
        if ( combinedId )
        {
            lorris::Packet packet;
            packet.devId() = *combinedId;
            packet.command() = 1;
            packet.push< float >( _targetPosition ); // Target
            packet.push< float >( position );        // Current
            packet.push< float >( velocity );        // Output
            packet.send();
        }
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
        if ( maxSpeed <= 0 )
        {
            gzwarn << "Speed non-positive for setting position, setting desired position to current position\n";
            setCurrentPosition( lastControlType );
            return;
        }

        if ( lastControlType )
        {
            resetPositionPID( *lastControlType );
        }

        setMaxSpeed( maxSpeed );
        setTargetPosition( targetPosition );
        _targetDirection = targetPosition > _jointData.joint->Position();
    }

    void setCurrentPosition( std::optional< PidControlType > lastControlType )
    {
        if ( lastControlType )
        {
            resetPositionPID( *lastControlType );
        }

        setMaxSpeed( _jointData.getMaxVelocity() );
        setTargetPosition< false >( _jointData.joint->Position() );
        _targetDirection = std::nullopt;
    }
};

class PIDController
{
    JointDataBase & _jointData;
    PositionPIDController _controller;

    PidControlType activeController = PidControlType::Force;

    event::ConnectionPtr _connection;

    void onPhysicsUpdate()
    {
        switch ( activeController )
        {
            case PidControlType::Force:
                _controller.forcePhysicsUpdate();
                return;
            case PidControlType::Position:
                _controller.posPhysicsUpdate();
                return;
            case PidControlType::Velocity:
                _controller.velPhysicsUpdate();
                return;
        }
        assert( false && "unrecognized pid control type value" );
    }

public:
    PIDController( const PIDController & ) = delete;
    PIDController & operator=( const PIDController & ) = delete;

    template < typename PositionCallback,
               typename = std::enable_if_t< std::is_invocable_v< PositionCallback, double > > >
    PIDController( JointDataBase & jointData,
                   const PIDLoader::ControllerValues & pidValues,
                   PositionCallback && positionReached,
                   int jointId )
            : _jointData( jointData )
            , _controller( jointData,
                           pidValues,
                           std::forward< PositionCallback >( positionReached ),
                           jointId )
    {
        _connection = event::Events::ConnectBeforePhysicsUpdate(
                std::bind( &PIDController::onPhysicsUpdate, this ) );

        if ( pidValues.position.initTarget )
        {
            auto position = *pidValues.position.initTarget;
            auto speed = pidValues.velocity.initTarget.value_or( _jointData.getMaxVelocity() );
            setTargetPositionWithSpeed( position, speed );
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
            _controller.setCurrentPosition( PidControlType::Force );
            activeController = PidControlType::Position;
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

    void setTargetPositionWithSpeed( double position, double speed )
    {
        _controller.setTargetPositionWithSpeed( position, speed, activeController );
        activeController = PidControlType::Position;
    }

    void setRofiId( int rofiId )
    {
        _controller.setRofiId( rofiId );
    }
};

} // namespace gazebo
