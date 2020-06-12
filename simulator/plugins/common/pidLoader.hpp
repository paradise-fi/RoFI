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
    struct InitTargets
    {
        std::string jointName;
        std::optional< double > force;
        std::optional< double > velocity;
        std::optional< double > position;

        InitTargets( std::string jointName,
                     std::optional< double > force,
                     std::optional< double > velocity,
                     std::optional< double > position )
                : jointName( std::move( jointName ) )
                , force( force )
                , velocity( velocity )
                , position( position )
        {
        }

        static InitTargets newForce( std::string jointName, double force )
        {
            return InitTargets( std::move( jointName ), force, {}, {} );
        }
        static InitTargets newVelocity( std::string jointName, double velocity )
        {
            return InitTargets( std::move( jointName ), {}, velocity, {} );
        }
        static InitTargets newPosition( std::string jointName, double position, double velocity )
        {
            return InitTargets( std::move( jointName ), {}, velocity, position );
        }
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

    static sdf::ElementPtr createPIDValues( const std::string & name, const PIDValues & values )
    {
        sdf::ElementPtr elem = newElement( name );
        assert( elem );

        elem->InsertElement( newElemWithValue( "pid_gains", values.pidGains ) );
        if ( values.initTarget )
        {
            elem->InsertElement( newElemWithValue( "init_target", *values.initTarget ) );
        }

        return elem;
    }

    static sdf::ElementPtr createController( const ControllerValues & values )
    {
        sdf::ElementPtr controller = newElement( "controller" );
        assert( controller );

        controller->InsertElement( newElemWithValue( "joint", values.jointName ) );

        auto force = newElement( "force" );
        if ( values.forceTarget )
        {
            force->InsertElement( newElemWithValue( "init_target", *values.forceTarget ) );
        }
        controller->InsertElement( force );

        controller->InsertElement( createPIDValues( "velocity", values.velocity ) );
        controller->InsertElement( createPIDValues( "position", values.position ) );

        return controller;
    }

    template < typename T >
    static void updateOptionalValue( sdf::ElementPtr parentSdf,
                                     const std::string & name,
                                     const std::optional< T > & value )
    {
        if ( value )
        {
            setValue( getOnlyChildOrCreate( parentSdf, name ), *value );
        }
        else
        {
            auto child = getOnlyChild< false >( parentSdf, name );
            if ( child )
            {
                child->RemoveFromParent();
            }
        }
    }

    static void updateInitTargetsSdf( sdf::ElementPtr pluginSdf, const InitTargets & targets )
    {
        for ( auto controller : getChildren( pluginSdf, "controller" ) )
        {
            if ( getOnlyChild< true >( controller, "joint" )->Get< std::string >()
                 != targets.jointName )
            {
                continue;
            }

            auto force = getOnlyChild< true >( controller, "force" );
            updateOptionalValue( force, "init_target", targets.force );

            auto velocity = getOnlyChild< true >( controller, "velocity" );
            updateOptionalValue( velocity, "init_target", targets.velocity );

            auto position = getOnlyChild< true >( controller, "position" );
            updateOptionalValue( position, "init_target", targets.position );
        }
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

        for ( auto elem : getChildren( pluginSdf, "controller" ) )
        {
            elem->RemoveFromParent();
        }
        for ( const auto & values : loadedValues )
        {
            pluginSdf->InsertElement( createController( values ) );
        }

        return loadedValues;
    }
};

} // namespace gazebo
