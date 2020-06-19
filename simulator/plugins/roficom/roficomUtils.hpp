#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gazebo/common/Events.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "jointData.hpp"
#include "utils.hpp"

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>


namespace gazebo
{
enum class RoFICoMPosition : signed char
{
    Retracted = 0,
    Retracting = 1,
    Extending = 2,
    Extended = 3,
};

inline ignition::math::Angle getAngle( const ignition::math::Vector3d & lhs,
                                       const ignition::math::Vector3d & rhs )
{
    return { std::acos( lhs.Dot( rhs ) / ( lhs.Length() * rhs.Length() ) ) };
}

physics::LinkPtr getLinkByName( physics::ModelPtr roficom, const std::string & name );
physics::JointPtr getJointByName( physics::ModelPtr roficom, const std::string & jointName );


class RoficomController
{
    JointDataBase & _jointData;
    event::ConnectionPtr _connection;

    RoFICoMPosition _position;
    double _lastForce = 0;

    std::function< void( RoFICoMPosition ) > _onPositionReached;

    static constexpr double minForce = 1e-4;
    static constexpr double forceMultiplier = 1.01;


    void onPhysicsUpdate()
    {
        assert( _jointData );
        assert( _jointData.joint );

        switch ( _position )
        {
            case RoFICoMPosition::Retracted:
            case RoFICoMPosition::Extended:
            {
                return;
            }
            case RoFICoMPosition::Retracting:
            {
                if ( _jointData.joint->Position() <= _jointData.getMinPosition() )
                {
                    positionReached( RoFICoMPosition::Retracted );
                    return;
                }

                assert( _jointData.getLowestEffort() <= -minForce );
                _lastForce = std::clamp( _lastForce * forceMultiplier,
                                         _jointData.getLowestEffort(),
                                         -minForce );
                _jointData.joint->SetForce( 0, _lastForce );
                break;
            }
            case RoFICoMPosition::Extending:
            {
                if ( _jointData.joint->Position() >= _jointData.getMaxPosition() )
                {
                    positionReached( RoFICoMPosition::Extended );
                    return;
                }

                assert( minForce <= _jointData.getMaxEffort() );
                _lastForce = std::clamp( _lastForce * forceMultiplier,
                                         minForce,
                                         _jointData.getMaxEffort() );
                _jointData.joint->SetForce( 0, _lastForce );
                break;
            }
        }
    }

    void positionReached( RoFICoMPosition position )
    {
        assert( _onPositionReached );

        _position = position;
        _onPositionReached( _position );

        fixJoint();
    }

    void fixJoint()
    {
        assert( _jointData );
        assert( _jointData.joint );

        auto currentPosition = _jointData.joint->Position();
        _jointData.joint->SetLowerLimit( 0, currentPosition );
        _jointData.joint->SetUpperLimit( 0, currentPosition );
    }

    void looseJoint()
    {
        assert( _jointData );
        assert( _jointData.joint );

        _jointData.joint->SetLowerLimit( 0, _jointData.minPosition );
        _jointData.joint->SetUpperLimit( 0, _jointData.maxPosition );
    }

public:
    RoficomController( const RoficomController & ) = delete;
    RoficomController & operator=( const RoficomController & ) = delete;

    RoficomController( JointDataBase & jointData,
                       RoFICoMPosition initPosition,
                       std::function< void( RoFICoMPosition ) > onPositionReached )
            : _jointData( jointData )
            , _position( initPosition )
            , _onPositionReached( std::move( onPositionReached ) )
    {
        assert( _jointData );
        assert( _jointData.joint );
        assert( _onPositionReached );

        _connection = event::Events::ConnectBeforePhysicsUpdate(
                std::bind( &RoficomController::onPhysicsUpdate, this ) );

        switch ( _position )
        {
            case RoFICoMPosition::Extending:
            case RoFICoMPosition::Retracting:
            {
                looseJoint();
                break;
            }
            case RoFICoMPosition::Extended:
            {
                assert( _jointData.joint->Position() >= _jointData.getMaxPosition() );
                break;
            }
            case RoFICoMPosition::Retracted:
            {
                assert( _jointData.joint->Position() <= _jointData.getMinPosition() );
                fixJoint();
                break;
            }
        }
    }

    void extend()
    {
        _position = RoFICoMPosition::Extending;
        looseJoint();
    }

    void retract()
    {
        _position = RoFICoMPosition::Retracting;
        looseJoint();
    }


    struct ControllerValues
    {
        std::string jointName;
        sdf::ElementPtr limitSdf;
        bool extend = false;
        bool positionReached = false;

        RoFICoMPosition position() const
        {
            if ( positionReached )
            {
                if ( extend )
                {
                    return RoFICoMPosition::Extended;
                }
                return RoFICoMPosition::Retracted;
            }
            else
            {
                if ( extend )
                {
                    return RoFICoMPosition::Extending;
                }
                return RoFICoMPosition::Retracting;
            }
        }
    };

    static ControllerValues loadControllerValues( sdf::ElementPtr pluginSdf )
    {
        assert( pluginSdf );

        ControllerValues controllerValues;

        checkChildrenNames( pluginSdf, { "joint", "extend", "position_reached", "limit" } );
        controllerValues.jointName =
                getOnlyChild< true >( pluginSdf, "joint" )->Get< std::string >();

        if ( !pluginSdf->HasElement( "extend" ) )
        {
            pluginSdf->InsertElement( newElemWithValue( "extend", false ) );
        }
        if ( !pluginSdf->HasElement( "position_reached" ) )
        {
            pluginSdf->InsertElement( newElemWithValue( "position_reached", false ) );
        }

        controllerValues.limitSdf = getOnlyChildOrCreate( pluginSdf, "limit" );
        controllerValues.extend = getOnlyChild< true >( pluginSdf, "extend" )->Get< bool >();
        controllerValues.positionReached =
                getOnlyChild< true >( pluginSdf, "position_reached" )->Get< bool >();

        return controllerValues;
    }
};

} // namespace gazebo
