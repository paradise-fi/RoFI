#pragma once

#include <optional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cmath>
#include <gz/math/Angle.hh>
#include <gz/math/Pose3.hh>
#include <gz/math/Quaternion.hh>
#include <gz/math/Vector2.hh>
#include <gz/math/Vector3.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Link.hh>
#include <gz/sim/Model.hh>

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

inline gz::math::Angle getAngle( const gz::math::Vector3d & lhs, const gz::math::Vector3d & rhs )
{
    return { std::acos( lhs.Dot( rhs ) / ( lhs.Length() * rhs.Length() ) ) };
}

std::optional< gz::sim::Entity > getLinkByName( gz::sim::Entity roficom,
                                                const gz::sim::EntityComponentManager & ecm,
                                                const std::string & name );
std::optional< gz::sim::Entity > getJointByName( gz::sim::Entity roficom,
                                                 const gz::sim::EntityComponentManager & ecm,
                                                 const std::string & jointName );

class RoficomController
{
    JointDataBase & _jointData;
    RoFICoMPosition _position;
    double _lastForce = 0;
    std::function< void( RoFICoMPosition ) > _onPositionReached;

    static constexpr double minForce = 1e-4;
    static constexpr double forceMultiplier = 1.01;

    void positionReached( gz::sim::EntityComponentManager & ecm, RoFICoMPosition position )
    {
        _position = position;
        _onPositionReached( _position );
        fixJoint( ecm );
    }

    void fixJoint( gz::sim::EntityComponentManager & ecm )
    {
        auto currentPosition = _jointData.position( ecm );
        _jointData.setPositionLimits( ecm, currentPosition, currentPosition );
    }

    void looseJoint( gz::sim::EntityComponentManager & ecm )
    {
        _jointData.setPositionLimits( ecm, _jointData.minPosition, _jointData.maxPosition );
    }

public:
    RoficomController( const RoficomController & ) = delete;
    RoficomController & operator=( const RoficomController & ) = delete;

    RoficomController( JointDataBase & jointData,
                       gz::sim::EntityComponentManager & ecm,
                       RoFICoMPosition initPosition,
                       std::function< void( RoFICoMPosition ) > onPositionReached )
            : _jointData( jointData )
            , _position( initPosition )
            , _onPositionReached( std::move( onPositionReached ) )
    {
        switch ( _position )
        {
            case RoFICoMPosition::Extending:
            case RoFICoMPosition::Retracting:
                looseJoint( ecm );
                break;
            case RoFICoMPosition::Extended:
            case RoFICoMPosition::Retracted:
                fixJoint( ecm );
                break;
        }
    }

    void update( const gz::sim::UpdateInfo & info, gz::sim::EntityComponentManager & ecm )
    {
        if ( info.paused )
        {
            return;
        }

        switch ( _position )
        {
            case RoFICoMPosition::Retracted:
            case RoFICoMPosition::Extended:
                return;
            case RoFICoMPosition::Retracting:
            {
                if ( _jointData.position( ecm ) <= _jointData.getMinPosition() )
                {
                    positionReached( ecm, RoFICoMPosition::Retracted );
                    return;
                }

                _lastForce = std::clamp( _lastForce * forceMultiplier,
                                         _jointData.getLowestEffort(),
                                         -minForce );
                _jointData.setForce( ecm, _lastForce );
                break;
            }
            case RoFICoMPosition::Extending:
            {
                if ( _jointData.position( ecm ) >= _jointData.getMaxPosition() )
                {
                    positionReached( ecm, RoFICoMPosition::Extended );
                    return;
                }

                _lastForce = std::clamp( _lastForce * forceMultiplier,
                                         minForce,
                                         _jointData.getMaxEffort() );
                _jointData.setForce( ecm, _lastForce );
                break;
            }
        }
    }

    void extend( gz::sim::EntityComponentManager & ecm )
    {
        looseJoint( ecm );
        _position = RoFICoMPosition::Extending;
        _lastForce = minForce;
    }

    void retract( gz::sim::EntityComponentManager & ecm )
    {
        looseJoint( ecm );
        _position = RoFICoMPosition::Retracting;
        _lastForce = -minForce;
    }

    struct ControllerValues
    {
        std::string jointName;
        sdf::ElementPtr limitSdf;
        bool extend = false;

        RoFICoMPosition startupState() const
        {
            return extend ? RoFICoMPosition::Extended : RoFICoMPosition::Retracted;
        }
    };

    static ControllerValues loadControllerValues( const std::shared_ptr< const sdf::Element > & pluginSdf )
    {
        assert( pluginSdf );

        ControllerValues controllerValues;
        auto sdf = std::const_pointer_cast< sdf::Element >( pluginSdf );
        checkChildrenNames( sdf, { "joint", "extend", "limit" } );
        controllerValues.jointName = getOnlyChild< true >( sdf, "joint" )->Get< std::string >();

        if ( auto limit = getOnlyChild< false >( sdf, "limit" ) )
        {
            controllerValues.limitSdf = limit;
        }
        if ( auto extend = getOnlyChild< false >( sdf, "extend" ) )
        {
            controllerValues.extend = extend->Get< bool >();
        }

        return controllerValues;
    }
};

} // namespace gazebo
