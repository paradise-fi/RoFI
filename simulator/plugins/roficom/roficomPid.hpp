#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include <cassert>

#include "../common/utils.hpp"


namespace gazebo
{
namespace roficom
{

enum class Position : signed char
{
    Retracted = 0,
    Retracting = 1,
    Extending = 2,
    Extended = 3,
};

class PID
{
    static constexpr double linear_p = 600.0; // TODO
    static constexpr double linear_i = 300.0; // TODO
    static constexpr double linear_d = 0.0; // TODO
    static constexpr double linear_imax = 123456789.0; // TODO

    static constexpr double positionPrecision = 1e-6; // [m]
    static constexpr double openedPosition = 7e-3; // [m]


    JointDataBase & _jointData;
    common::PID _controller;
    common::Time _prevUpdateTime;
    event::ConnectionPtr _connection;

    bool _targetPosition = false;
    Position _currentPosition = Position::Retracted;

    double getMinPosition() const
    {
        return 0.0 + positionPrecision;
    }

    double getMaxPosition() const
    {
        return openedPosition - positionPrecision;
    }

    double getTargetPosition() const
    {
        return _targetPosition ? getMaxPosition() : getMinPosition();
    }

    void onPhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );
        common::Time currTime = _jointData.joint->GetWorld()->SimTime();
        common::Time stepTime = currTime - _prevUpdateTime;
        _prevUpdateTime = currTime;

        assert( stepTime > 0 && "time went backwards" );

        double linearError = _jointData.joint->Position( 0 ) - getTargetPosition();
        if ( _targetPosition )
            gzmsg << "Pos: " << _jointData.joint->Position( 0 ) << ", target: " << getTargetPosition() << ", error: " << linearError << "\n";
        if ( linearError <= positionPrecision )
        {
            _currentPosition = _targetPosition ? Position::Extended : Position::Retracted;
        }

        auto force = _controller.Update( linearError, stepTime );
        if ( _targetPosition )
            gzmsg << "New force: " << force << ", current: " << _jointData.joint->GetForce( 0 ) << "\n";
        _jointData.joint->SetForce( 0, force );
    }

public:
    PID( JointDataBase & jointData ) :
            _jointData( jointData ),
            _controller( linear_p, linear_i, linear_d,
                         linear_imax, -linear_imax,
                         _jointData.maxEffort, -_jointData.maxEffort )
    {
        assert( jointData );
        gzmsg << "Max effort: " << _jointData.maxEffort << "\n";
        _prevUpdateTime = _jointData.joint->GetWorld()->SimTime();
        _connection = event::Events::ConnectBeforePhysicsUpdate( std::bind( &PID::onPhysicsUpdate, this ) );
    }

    void setTargetPosition( bool position )
    {
        assert( _jointData );

        if ( position == _targetPosition )
        {
            gzmsg << "Joint already in the same position\n";
            return;
        }
        _targetPosition = position;
        _currentPosition = _targetPosition ? Position::Extending : Position::Retracting;
    }

    Position getCurrentPosition() const
    {
        return _currentPosition;
    }
};

} // namespace roficom
} // namespace gazebo
