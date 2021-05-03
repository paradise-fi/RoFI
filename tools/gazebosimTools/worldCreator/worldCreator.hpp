#pragma once
#include <istream>
#include <optional>

#include <gazebo/gazebo.hh>
#include <sdf/sdf.hh>

#include "Configuration.h"
#include "utils.hpp"


inline bool equalPose( const ignition::math::Pose3d & lhs, const ignition::math::Pose3d & rhs )
{
    return lhs.Pos() == rhs.Pos() && ( lhs.Rot() == rhs.Rot() || lhs.Rot() == -rhs.Rot() );
}

struct ConfigWithPose
{
    Configuration config;
    ignition::math::Pose3d pose;

    static std::optional< ConfigWithPose > tryReadConfiguration( std::istream & input );

    static ConfigWithPose readConfiguration( std::istream & input )
    {
        auto config = tryReadConfiguration( input );
        if ( !config )
        {
            throw std::runtime_error( "No configuration" );
        }
        return *config;
    }

    static std::vector< ConfigWithPose > readConfigurations( std::istream & input )
    {
        std::vector< ConfigWithPose > configs;

        std::optional< ConfigWithPose > config;
        while ( config = tryReadConfiguration( input ), config )
        {
            configs.push_back( *config );
        }

        return configs;
    }
};


sdf::SDFPtr createWorld( const std::string & worldPath,
                         const std::vector< ConfigWithPose > & configs );

void addConfigurationToWorld( sdf::ElementPtr world, const ConfigWithPose & config );

sdf::SDFPtr loadFromFile( const std::string & filename );

void addModuleToDistributor( sdf::ElementPtr distributorSdf, ID rofiId );
void setModulePosition( sdf::ElementPtr moduleStateSdf,
                        const Module & module,
                        const std::array< Matrix, 2 > & matrices,
                        const ignition::math::Pose3d & beginPose );
void setModulePIDPositionController( sdf::ElementPtr modulePluginSdf, const Module & module );
void setRoficomExtendedPlugin( sdf::ElementPtr pluginSdf, bool extended );
sdf::ElementPtr createRoficomState( sdf::ElementPtr roficomSdf,
                                    bool extended,
                                    const ignition::math::Pose3d & parentPose );
void setAttached( sdf::ElementPtr worldSdf, const Configuration & config );

sdf::ElementPtr_V getModules( sdf::ElementPtr worldSdf );

sdf::ElementPtr newRoFIUniversalModule( ID rofiId );

std::string getRoFICoMInnerName( sdf::ElementPtr roficomSdf );

template < bool Required >
sdf::ElementPtr getElemByName( sdf::ElementPtr sdf,
                               const std::string & elemName,
                               const std::string & nameValue )
{
    using namespace gazebo;

    assert( sdf );

    for ( auto child : getChildren( sdf, elemName ) )
    {
        if ( getAttribute< std::string >( child, "name" ) != nameValue )
        {
            continue;
        }

        return child;
    }

    if constexpr ( Required )
    {
        std::cerr << "Expected '" << elemName << "' element with name '" << nameValue << "' in '"
                  << sdf->GetName() << "'\n";
        throw std::runtime_error( "Expected '" + elemName + "' element with name '" + nameValue
                                  + "' in '" + sdf->GetName() + "'" );
    }
    else
    {
        return {};
    }
}

inline sdf::ElementPtr getElemByNameOrCreate( sdf::ElementPtr sdf,
                                              const std::string & elemName,
                                              const std::string & nameValue )
{
    using namespace gazebo;

    auto elem = getElemByName< false >( sdf, elemName, nameValue );
    if ( !elem )
    {
        elem = newElement( elemName );
        setAttribute( elem, "name", nameValue );
        insertElement( sdf, elem );
    }
    assert( elem );
    return elem;
}

ignition::math::Pose3d matrixToPose( const Matrix & matrix );

inline void setPose( sdf::ElementPtr elem, const ignition::math::Pose3d & pose )
{
    using namespace gazebo;

    assert( elem );
    setValue( getOnlyChildOrCreate( elem, "pose" ), pose );
}

inline ignition::math::Pose3d moveShoeToCenter( ShoeId shoeId )
{
    using namespace ignition::math;

    if ( shoeId == A )
    {
        return { 0, 0, 0.05, Angle::HalfPi(), 0, 0 };
    }

    assert( shoeId == B );
    return { 0, 0, 0.05, -Angle::HalfPi(), 0, Angle::Pi() };
}

inline std::string rofiName( ID rofiId )
{
    return "rofi_" + std::to_string( rofiId );
}

inline std::string roficomInnerName( sdf::ElementPtr moduleSdf, ShoeId shoeId, ConnectorId connId )
{
    using namespace gazebo;

    int i = 0;
    for ( auto roficomSdf : getChildren( moduleSdf, "model" ) )
    {
        if ( !isRoFICoM( roficomSdf ) )
        {
            continue;
        }
        if ( i != shoeId * 3 + connId )
        {
            i++;
            continue;
        }
        auto result = GetScopedName( roficomSdf );
        return result;
    }
    throw std::runtime_error( "Could not find roficom" );
}
