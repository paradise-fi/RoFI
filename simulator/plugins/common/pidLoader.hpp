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
        std::optional< ignition::math::Vector3d > pidGains;
        std::optional< double > initTarget = {};

        common::PID getPID( double minCmd, double maxCmd ) const
        {
            auto tmpPidGains = pidGains.value_or( ignition::math::Vector3d() );

            return common::PID( tmpPidGains.X(),
                                tmpPidGains.Y(),
                                tmpPidGains.Z(),
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
        std::optional< PIDValues > velocity = {};
        std::optional< PIDValues > position = {};

        PIDValues getVelocity() const
        {
            return velocity.value_or( PIDValues() );
        }
        PIDValues getPosition() const
        {
            return position.value_or( PIDValues() );
        }
    };

    static std::optional< double > loadInitTargetValue( sdf::ElementPtr pidSdf )
    {
        assert( pidSdf );

        std::optional< double > initForce;
        for ( auto child = pidSdf->GetFirstElement(); child; child = child->GetNextElement() )
        {
            if ( child->GetName() == "init_target" )
            {
                if ( initForce )
                {
                    gzerr << "Multiple occurencies of element \"init_target\".\n";
                }
                initForce = child->Get< double >();
            }
            else
            {
                gzerr << "Unrecognized element \"" << child->GetName() << "\".\n";
                continue;
            }
        }

        if ( !initForce )
        {
            gzwarn << "No element \"init_force\" found in \"" << pidSdf->GetName() << "\".\n";
        }
        return initForce;
    }

    static PIDValues loadPIDValues( sdf::ElementPtr pidSdf )
    {
        assert( pidSdf );

        PIDValues pidValues;
        for ( auto child = pidSdf->GetFirstElement(); child; child = child->GetNextElement() )
        {
            if ( child->GetName() == "pid_gains" )
            {
                if ( pidValues.pidGains )
                {
                    gzerr << "Multiple occurencies of element \"pid_gains\".\n";
                }
                pidValues.pidGains = child->Get< ignition::math::Vector3d >();
            }
            else if ( child->GetName() == "init_target" )
            {
                if ( pidValues.initTarget )
                {
                    gzerr << "Multiple occurencies of element \"init_target\".\n";
                }
                pidValues.initTarget = child->Get< double >();
            }
            else
            {
                gzerr << "Unrecognized element \"" << child->GetName() << "\".\n";
                continue;
            }
        }

        if ( !pidValues.pidGains )
        {
            gzwarn << "No element \"pid_gains\" found in \"" << pidSdf->GetName() << "\".\n";
        }
        return pidValues;
    }

    static std::vector< ControllerValues > loadControllerValues( sdf::ElementPtr pluginSdf )
    {
        if ( !pluginSdf )
        {
            return {};
        }

        std::vector< ControllerValues > loadedValues;

        for ( auto elem = pluginSdf->GetFirstElement(); elem; elem = elem->GetNextElement() )
        {
            if ( elem->GetName() != "controller" )
            {
                continue;
            }

            ControllerValues controllerValues;
            std::vector< std::string > jointNames;

            for ( auto child = elem->GetFirstElement(); child; child = child->GetNextElement() )
            {
                if ( child->GetName() == "joint" )
                {
                    jointNames.push_back( child->Get< std::string >() );
                }
                else if ( child->GetName() == "force" )
                {
                    if ( controllerValues.forceTarget )
                    {
                        gzerr << "Multiple occurencies of element \"force\".\n";
                    }
                    controllerValues.forceTarget = loadInitTargetValue( child );
                }
                else if ( child->GetName() == "velocity" )
                {
                    if ( controllerValues.velocity )
                    {
                        gzerr << "Multiple occurencies of element \"velocity\".\n";
                    }
                    controllerValues.velocity = loadPIDValues( child );
                }
                else if ( child->GetName() == "position" )
                {
                    if ( controllerValues.position )
                    {
                        gzerr << "Multiple occurencies of element \"position\".\n";
                    }
                    controllerValues.position = loadPIDValues( child );
                }
                else
                {
                    gzerr << "Unrecognized element \"" << child->GetName() << "\".\n";
                    continue;
                }
            }
            if ( jointNames.empty() )
            {
                gzerr << "No element \"joint\" found in controller.\n";
                continue;
            }
            if ( !controllerValues.velocity )
            {
                gzwarn << "No element \"velocity\" found in controller.\n";
            }
            if ( !controllerValues.position )
            {
                gzwarn << "No element \"position\" found in controller.\n";
            }

            if ( controllerValues.forceTarget )
            {
                if ( controllerValues.getVelocity().initTarget
                     || controllerValues.getPosition().initTarget )
                {
                    gzwarn << "Force target has no effect with other targets.\n";
                }
            }

            for ( auto name : jointNames )
            {
                loadedValues.push_back( controllerValues );
                loadedValues.back().jointName = std::move( name );
            }
        }

        return loadedValues;
    }
};

} // namespace gazebo
