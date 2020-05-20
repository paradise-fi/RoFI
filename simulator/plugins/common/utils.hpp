#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include <gazebo/common/Events.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>


namespace gazebo
{
template < msgs::Joint::Type >
struct Precision
{
};

template <>
struct Precision< msgs::Joint::REVOLUTE >
{
    // Used for position set callback and for boundaries
    // Revolute joints: [rad]
    static constexpr double position = 1e-2;

    // Used for position set callback and for boundaries
    // Revolute joints: [rad/s]
    static constexpr double velocity = 1e-2;
};

template <>
struct Precision< msgs::Joint::PRISMATIC >
{
    // Used for position set callback and for boundaries
    // Prismatic joints: [m]
    static constexpr double position = 1e-4;

    // Used for position set callback and for boundaries
    // Prismatic joints: [m/s]
    static constexpr double velocity = 1e-4;
};

struct JointDataBase
{
    // Used for position set callback and for boundaries
    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double getPositionPrecision() const
    {
        if ( joint->GetMsgType() == msgs::Joint::PRISMATIC )
        {
            return Precision< msgs::Joint::PRISMATIC >::position;
        }
        return Precision< msgs::Joint::REVOLUTE >::position;
    }

    // Used for position set callback and for boundaries
    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getVelocityPrecision() const
    {
        if ( joint->GetMsgType() == msgs::Joint::PRISMATIC )
        {
            return Precision< msgs::Joint::PRISMATIC >::velocity;
        }
        return Precision< msgs::Joint::REVOLUTE >::velocity;
    }

    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double minPosition = std::numeric_limits< double >::lowest();
    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double maxPosition = std::numeric_limits< double >::max();
    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double minSpeed = std::numeric_limits< double >::min();
    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double maxSpeed = std::numeric_limits< double >::max();
    // Prismatic joints: [N]
    // Revolute joints: [Nm]
    double maxEffort = std::numeric_limits< double >::max();

    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double getMaxPosition() const
    {
        return maxPosition - getPositionPrecision();
    }

    // Prismatic joints: [m]
    // Revolute joints: [rad]
    double getMinPosition() const
    {
        return minPosition + getPositionPrecision();
    }

    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getMaxVelocity() const
    {
        return maxSpeed - getVelocityPrecision();
    }

    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getLowestVelocity() const
    {
        return -getMaxVelocity();
    }

    // Prismatic joints: [m/s]
    // Revolute joints: [rad/s]
    double getMinVelocity() const
    {
        return minSpeed + getVelocityPrecision();
    }

    // Prismatic joints: [N]
    // Revolute joints: [Nm]
    double getMaxEffort() const
    {
        return maxEffort;
    }

    // Prismatic joints: [N]
    // Revolute joints: [Nm]
    double getLowestEffort() const
    {
        return -getMaxEffort();
    }

    physics::JointPtr joint;

    operator bool() const
    {
        return bool( joint );
    }

protected:
    ~JointDataBase() = default;

    JointDataBase( const JointDataBase & ) = default;
    JointDataBase & operator=( const JointDataBase & ) = default;

    explicit JointDataBase( physics::JointPtr jointPtr ) : joint( std::move( jointPtr ) )
    {
        if ( !joint )
        {
            return;
        }

        auto limits = joint->GetSDF()->GetElement( "axis" )->GetElement( "limit" );
        limits->GetElement( "lower" )->GetValue()->Get( minPosition );
        limits->GetElement( "upper" )->GetValue()->Get( maxPosition );
        limits->GetElement( "velocity" )->GetValue()->Get( maxSpeed );
        limits->GetElement( "effort" )->GetValue()->Get( maxEffort );

        if ( minPosition >= maxPosition )
        {
            gzerr << "Maximal position is not larger than minimal\n";
            throw std::runtime_error( "Maximal position is not larger than minimal" );
        }

        if ( maxSpeed < 0 )
        {
            gzmsg << "No max speed in " << joint->GetScopedName() << "\n";
            maxSpeed = std::numeric_limits< double >::max();
        }
        if ( maxEffort < 0 )
        {
            gzmsg << "No max effort in " << joint->GetScopedName() << "\n";
            maxEffort = std::numeric_limits< double >::max();
        }

        assert( maxPosition - minPosition > 2 * getPositionPrecision() );
        assert( maxSpeed > getVelocityPrecision() );
    }
};

template < typename Controller >
struct JointData : public JointDataBase
{
    Controller controller;

    template < typename... Args >
    explicit JointData( physics::JointPtr joint, Args &&... args )
            : JointDataBase( std::move( joint ) )
            , controller( *this, std::forward< Args >( args )... )
    {
    }

    ~JointData() = default;
};

inline double verboseClamp( double value, double min, double max, std::string debugName )
{
    assert( min <= max );
    if ( value < min )
    {
        gzwarn << "Value of " << debugName << " clamped from " << value << " to " << min << "\n";
        return min;
    }
    if ( value > max )
    {
        gzwarn << "Value of " << debugName << " clamped from " << value << " to " << max << "\n";
        return max;
    }
    return value;
}

inline double clamp( double value, double min, double max )
{
    assert( min <= max );
    return std::clamp( value, min, max );
}

inline bool equal( double first, double second, double precision )
{
    return std::abs( first - second ) <= precision;
}

inline std::string getElemPath( gazebo::physics::BasePtr elem, const std::string & delim = "/" )
{
    assert( elem );

    std::vector< std::string > names;

    while ( elem )
    {
        names.push_back( elem->GetName() );
        elem = elem->GetParent();
    }

    assert( !names.empty() );
    auto it = names.rbegin();
    std::string elemPath = *it++;
    while ( it != names.rend() )
    {
        elemPath += delim + *it++;
    }

    return elemPath;
}

inline sdf::ElementPtr getPluginSdf( sdf::ElementPtr modelSdf, const std::string & pluginName )
{
    assert( modelSdf );

    if ( !modelSdf->HasElement( "plugin" ) )
    {
        return {};
    }

    for ( auto child = modelSdf->GetElement( "plugin" ); child;
          child = child->GetNextElement( "plugin" ) )
    {
        if ( !child->HasAttribute( "filename" ) )
        {
            continue;
        }

        auto value = child->Get< std::string >( "filename", "" );
        if ( value.second && value.first == pluginName )
        {
            return child;
        }
    }
    return {};
}

inline std::vector< sdf::ElementPtr > getChildren( sdf::ElementPtr sdf, const std::string & name )
{
    assert( sdf );

    if ( !sdf->HasElement( name ) )
    {
        return {};
    }

    std::vector< sdf::ElementPtr > children;
    for ( auto child = sdf->GetElement( name ); child; child = child->GetNextElement( name ) )
    {
        children.push_back( child );
    }
    return children;
}

template < bool Required >
sdf::ElementPtr getOnlyChild( sdf::ElementPtr sdf, const std::string & name )
{
    assert( sdf );

    if ( !sdf->HasElement( name ) )
    {
        if constexpr ( Required )
        {
            gzerr << "Expected element '" << name << "' in '" << sdf->GetName() << "'\n";
            throw std::runtime_error( "Expected element '" + name + "' in '" + sdf->GetName()
                                      + "'" );
        }
        else
        {
            return {};
        }
    }

    sdf::ElementPtr child = sdf->GetElement( name );
    if ( child->GetNextElement( name ) )
    {
        gzerr << "Expected only one element '" << name << "' in '" << sdf->GetName() << "'\n";
        throw std::runtime_error( "Expected only one element '" + name + "' in '" + sdf->GetName()
                                  + "'" );
    }
    return child;
}

inline void checkChildrenNames( sdf::ElementPtr sdf, const std::vector< std::string > & names )
{
    assert( sdf );

    for ( auto child = sdf->GetFirstElement(); child; child = child->GetNextElement() )
    {
        if ( std::find( names.begin(), names.end(), child->GetName() ) == names.end() )
        {
            gzwarn << "Unknown element '" << child->GetName() << "' in '" << sdf->GetName()
                   << "'\n";
        }
    }
}

inline bool isRoFICoM( physics::ModelPtr model )
{
    return model && getPluginSdf( model->GetSDF(), "libroficomPlugin.so" ) != nullptr;
}

inline bool hasAttacherPlugin( physics::WorldPtr world )
{
    return world && getPluginSdf( world->SDF(), "libattacherPlugin.so" ) != nullptr;
}

} // namespace gazebo
