#pragma once

#include <cassert>
#include <limits>
#include <type_traits>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "utils.hpp"


namespace gazebo
{
class PIDLoader
{
public:
    struct PIDValues
    {
        ignition::math::Vector3d pidGains;
        std::optional< double > initTarget = {};

        common::PID getPID( double minCmd, double maxCmd ) const
        {
            return common::PID( pidGains.X(),
                                pidGains.Y(),
                                pidGains.Z(),
                                maxCmd,
                                minCmd,
                                maxCmd,
                                minCmd );
        }
    };
    struct ControllerValues
    {
        std::string jointName;
        std::optional< double > forceTarget = {};
        PIDValues velocity = {};
        PIDValues position = {};
    };

    static PIDValues loadPIDValues( sdf::ElementPtr pidSdf )
    {
        assert( pidSdf );

        PIDValues pidValues;

        checkChildrenNames( pidSdf, { "pid_gains", "init_target" } );

        pidValues.pidGains =
                getOnlyChild< true >( pidSdf, "pid_gains" )->Get< ignition::math::Vector3d >();
        auto initTarget = getOnlyChild< false >( pidSdf, "init_target" );
        if ( initTarget )
        {
            pidValues.initTarget = initTarget->Get< double >();
        }

        return pidValues;
    }

    static std::vector< ControllerValues > loadControllerValues( sdf::ElementPtr pluginSdf )
    {
        assert( pluginSdf );

        std::vector< ControllerValues > loadedValues;

        for ( auto elem : getChildren( pluginSdf, "controller" ) )
        {
            ControllerValues controllerValues;

            checkChildrenNames( elem, { "joint", "force", "velocity", "position" } );

            auto jointNames = getChildren( elem, "joint" );
            auto force = getOnlyChild< false >( elem, "force" );
            if ( force )
            {
                checkChildrenNames( force, { "init_target" } );

                auto initTarget = getOnlyChild< false >( force, "init_target" );
                if ( initTarget )
                {
                    controllerValues.forceTarget = initTarget->Get< double >();
                }
            }
            controllerValues.velocity = loadPIDValues( getOnlyChild< true >( elem, "velocity" ) );
            controllerValues.position = loadPIDValues( getOnlyChild< true >( elem, "position" ) );

            if ( jointNames.empty() )
            {
                gzerr << "No element 'joint' found in 'controller'\n";
                continue;
            }
            if ( controllerValues.forceTarget )
            {
                if ( controllerValues.velocity.initTarget || controllerValues.position.initTarget )
                {
                    gzwarn << "Force target has no effect with other targets\n";
                }
            }

            for ( auto joint : jointNames )
            {
                loadedValues.push_back( controllerValues );
                loadedValues.back().jointName = joint->Get< std::string >();
            }
        }

        return loadedValues;
    }
};

} // namespace gazebo
