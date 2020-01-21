#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include <cassert>
#include <type_traits>
#include <limits>

#include "utils.hpp"


namespace gazebo
{

class PIDLoader
{
public:
    struct PIDValues
    {
        std::optional< ignition::math::Vector3d > pidGains;
        std::optional< double > iMax = {};
        std::optional< double > initTarget = {};

        common::PID getPID( double maxForce ) const
        {
            auto tmpPidGains = pidGains.value_or( ignition::math::Vector3d() );
            auto tmpIMax = iMax.value_or( std::numeric_limits< double >::max() );

            return common::PID( tmpPidGains.X(), tmpPidGains.Y(), tmpPidGains.Z(),
                                tmpIMax, -tmpIMax, maxForce, -maxForce );
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

    static PIDValues loadPIDValues( sdf::ElementPtr pidSdf )
    {
        PIDValues pidValues;
        for ( auto child = pidSdf->GetFirstElement(); child; child = child->GetNextElement() )
        {
            if ( child->GetName() == "pid_gains" )
            {
                if ( pidSdf->GetName() == "force" )
                {
                    gzwarn << "Force controller does not use \"pid_gains\".\n";
                    continue;
                }
                if ( pidValues.pidGains )
                {
                    gzerr << "Multiple occurencies of element \"pid_gains\".\n";
                    continue;
                }
                pidValues.pidGains = child->Get< ignition::math::Vector3d >();
            }
            else if ( child->GetName() == "i_max" )
            {
                if ( pidSdf->GetName() == "force" )
                {
                    gzwarn << "Force controller does not use \"i_max\".\n";
                    continue;
                }
                if ( pidValues.iMax )
                {
                    gzerr << "Multiple occurencies of element \"i_max\".\n";
                    continue;
                }
                pidValues.iMax = child->Get< double >();
            }
            else if ( child->GetName() == "init_target" )
            {
                if ( pidValues.initTarget )
                {
                    gzerr << "Multiple occurencies of element \"init_target\".\n";
                    continue;
                }
                pidValues.initTarget = child->Get< double >();
            }
            else
            {
                gzerr << "Unrecognized element \"" << child->GetName() << "\".\n";
                continue;
            }
        }

        if ( pidSdf->GetName() == "force" )
        {
            if ( !pidValues.initTarget )
            {
                gzwarn << "No element \"init_force\" found in \"" << pidSdf->GetName() << "\".\n";
            }
            return pidValues;
        }

        if ( !pidValues.pidGains )
        {
            gzwarn << "No element \"pid_gains\" found in \"" << pidSdf->GetName() << "\".\n";
        }
        if ( !pidValues.iMax )
        {
            gzwarn << "No element \"i_max\" found in \"" << pidSdf->GetName() << "\".\n";
        }
        return pidValues;
    }

    static std::vector< ControllerValues > loadControllerValues( sdf::ElementPtr pluginSdf )
    {
        assert( pluginSdf );
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
                        continue;
                    }
                    controllerValues.forceTarget = loadPIDValues( child ).initTarget;
                }
                else if ( child->GetName() == "velocity" )
                {
                    if ( controllerValues.velocity )
                    {
                        gzerr << "Multiple occurencies of element \"velocity\".\n";
                        continue;
                    }
                    controllerValues.velocity = loadPIDValues( child );
                }
                else if ( child->GetName() == "position" )
                {
                    if ( controllerValues.position )
                    {
                        gzerr << "Multiple occurencies of element \"position\".\n";
                        continue;
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

            for ( auto & name : jointNames )
            {
                loadedValues.push_back( controllerValues );
                loadedValues.back().jointName = std::move( name );
            }
        }

        return loadedValues;
    }
};

} // namespace gazebo
