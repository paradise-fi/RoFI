#pragma once

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gazebo/common/Events.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

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

    std::function< void( RoFICoMPosition ) > _onPositionReached;


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
                _jointData.joint->SetForce( 0, _jointData.getMaxEffort() );
                break;
            }
            case RoFICoMPosition::Extending:
            {
                if ( _jointData.joint->Position() >= _jointData.getMaxPosition() )
                {
                    positionReached( RoFICoMPosition::Extended );
                    return;
                }
                _jointData.joint->SetForce( 0, _jointData.getLowestEffort() );
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
    template < typename OnPositionReachedCallback >
    RoficomController( JointDataBase & jointData, OnPositionReachedCallback && onPositionReached )
            : _jointData( jointData )
            , _onPositionReached( std::forward< OnPositionReachedCallback >( onPositionReached ) )
    {
        assert( _jointData );
        assert( _jointData.joint );
        assert( _onPositionReached );

        _connection = event::Events::ConnectBeforePhysicsUpdate(
                std::bind( &RoficomController::onPhysicsUpdate, this ) );
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
        std::string joint;
        std::optional< bool > extend;
    };

    static ControllerValues loadControllerValues( sdf::ElementPtr pluginSdf )
    {
        assert( pluginSdf );

        checkChildrenNames( pluginSdf, { "controller" } );
        auto controller = getOnlyChild< true >( pluginSdf, "controller" );
        assert( controller );

        ControllerValues controllerValues;

        checkChildrenNames( pluginSdf, { "joint", "extend" } );
        controllerValues.joint = getOnlyChild< true >( controller, "joint" )->Get< std::string >();
        auto extendElem = getOnlyChild< false >( controller, "extend" );
        if ( extendElem )
        {
            controllerValues.extend = extendElem->Get< bool >();
        }

        return controllerValues;
    }
};

} // namespace gazebo
