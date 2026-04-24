#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gz/common/Console.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Types.hh>
#include <gz/sim/Util.hh>
#include <gz/sim/components/Model.hh>
#include <gz/sim/components/Name.hh>
#include <gz/sim/components/World.hh>
#include <sdf/Element.hh>
#include <sdf/Model.hh>
#include <sdf/World.hh>

namespace gazebo
{
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

inline std::string normalizePluginFilename( std::string filename )
{
    if ( filename.ends_with( ".so" ) )
    {
        filename.resize( filename.size() - 3 );
    }
    if ( filename.starts_with( "lib" ) )
    {
        filename.erase( 0, 3 );
    }
    return filename;
}

inline std::string getElemPath( gz::sim::Entity elem,
                                const gz::sim::EntityComponentManager & ecm,
                                const std::string & delim = "/",
                                bool prependWorldName = true )
{
    assert( elem != gz::sim::kNullEntity );

    std::vector< std::string > names;
    auto current = elem;
    while ( current != gz::sim::kNullEntity )
    {
        const auto * name = ecm.Component< gz::sim::components::Name >( current );
        if ( !name )
        {
            break;
        }

        const auto isWorld = ecm.Component< gz::sim::components::World >( current ) != nullptr;
        if ( isWorld && !prependWorldName )
        {
            break;
        }

        names.push_back( name->Data() );

        if ( isWorld )
        {
            break;
        }
        current = ecm.ParentEntity( current );
    }

    assert( !names.empty() );
    std::reverse( names.begin(), names.end() );

    std::string elemPath = names.front();
    for ( size_t i = 1; i < names.size(); ++i )
    {
        elemPath += delim + names[ i ];
    }
    return elemPath;
}

inline std::string getElemPath( sdf::ElementPtr elem,
                                const std::string & delim = "/",
                                bool prependWorldName = true )
{
    assert( elem );

    std::vector< std::string > names;

    while ( elem )
    {
        if ( !prependWorldName && elem->GetName() == "world" )
        {
            break;
        }
        names.push_back( elem->Get< std::string >( "name", "" ).first );
        if ( elem->GetName() == "world" )
        {
            break;
        }
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

inline std::string GetScopedName( gz::sim::Entity elem,
                                  const gz::sim::EntityComponentManager & ecm,
                                  bool prependWorldName = true )
{
    return getElemPath( elem, ecm, "::", prependWorldName );
}

inline std::string GetScopedName( sdf::ElementPtr elem, bool prependWorldName = false )
{
    return getElemPath( elem, "::", prependWorldName );
}

inline std::string scopedNameToPath( std::string scopedName )
{
    size_t start = 0;
    while ( ( start = scopedName.find( "::", start ) ) != std::string::npos )
    {
        scopedName.replace( start, 2, "/" );
        ++start;
    }
    return scopedName;
}

inline sdf::ElementPtr getPluginSdf( sdf::ElementPtr modelSdf, const std::string & pluginName )
{
    assert( modelSdf );

    if ( !modelSdf->HasElement( "plugin" ) )
    {
        return {};
    }

    const auto normalizedName = normalizePluginFilename( pluginName );
    for ( auto child = modelSdf->GetElement( "plugin" ); child;
          child = child->GetNextElement( "plugin" ) )
    {
        if ( !child->HasAttribute( "filename" ) )
        {
            continue;
        }

        auto value = child->Get< std::string >( "filename", "" );
        if ( value.second && normalizePluginFilename( value.first ) == normalizedName )
        {
            return child;
        }
    }
    return {};
}

inline sdf::ElementPtr getPluginSdf( const sdf::Model & modelSdf, const std::string & pluginName )
{
    return modelSdf.Element() ? getPluginSdf( modelSdf.Element(), pluginName ) : sdf::ElementPtr();
}

inline sdf::ElementPtr getPluginSdf( const sdf::World & worldSdf, const std::string & pluginName )
{
    return worldSdf.Element() ? getPluginSdf( worldSdf.Element(), pluginName ) : sdf::ElementPtr();
}

inline void insertElement( sdf::ElementPtr parent, sdf::ElementPtr child )
{
    assert( parent );
    assert( child );

    parent->InsertElement( child );
    child->SetParent( parent );
}

inline sdf::ElementPtr newElement( const std::string & name )
{
    auto elem = std::make_shared< sdf::Element >();
    elem->SetName( name );
    return elem;
}

template < typename T >
sdf::ElementPtr newElemWithValue( const std::string & name, const T & value )
{
    auto elem = newElement( name );
    elem->AddValue( "string", "", false );
    elem->Set( value );
    return elem;
}

template < typename T >
void setValue( sdf::ElementPtr elem, const T & value )
{
    assert( elem );

    if ( !elem->GetValue() )
    {
        elem->AddValue( "string", "", false );
    }
    elem->Set( value );
}

template < typename T >
void setAttribute( sdf::ElementPtr elem, const std::string & key, const T & value )
{
    assert( elem );

    if ( !elem->HasAttribute( key ) )
    {
        elem->AddAttribute( key, "string", "", false );
    }

    assert( elem->GetAttribute( key ) );
    elem->GetAttribute( key )->Set( value );
}

template < typename T >
T getAttribute( sdf::ElementPtr elem, const std::string & key )
{
    assert( elem );

    auto attribute = elem->GetAttribute( key );
    if ( !attribute )
    {
        throw std::runtime_error( "No attribute with key '" + key + "'" );
    }

    T value;
    if ( !attribute->Get( value ) )
    {
        throw std::runtime_error( "Could not get attribute with key '" + key + "'" );
    }
    return value;
}


template < typename T >
T getAttribute( sdf::ElementPtr elem, const std::string & key, const T & defaultValue )
{
    assert( elem );

    auto attribute = elem->GetAttribute( key );
    if ( !attribute )
    {
        return defaultValue;
    }

    T value = defaultValue;
    if ( !attribute->Get( value ) )
    {
        return defaultValue;
    }
    return value;
}

inline sdf::ElementPtr_V getChildren( sdf::ElementPtr sdf, const std::string & name )
{
    assert( sdf );

    if ( !sdf->HasElement( name ) )
    {
        return {};
    }

    sdf::ElementPtr_V children;
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

inline sdf::ElementPtr getOnlyChildOrCreate( sdf::ElementPtr sdf, const std::string & name )
{
    assert( sdf );

    if ( !sdf->HasElement( name ) )
    {
        insertElement( sdf, newElement( name ) );
    }

    return getOnlyChild< true >( sdf, name );
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

inline sdf::ElementPtr getRoFICoMPluginSdf( sdf::ElementPtr modelSdf )
{
    if ( !modelSdf )
    {
        return {};
    }

    if ( auto pluginSdf = getPluginSdf( modelSdf, "roficomPlugin" ) )
    {
        return pluginSdf;
    }

    for ( auto childModelSdf : getChildren( modelSdf, "model" ) )
    {
        if ( auto pluginSdf = getRoFICoMPluginSdf( childModelSdf ) )
        {
            return pluginSdf;
        }
    }

    return {};
}

inline sdf::ElementPtr getRoFICoMPluginSdf( const sdf::Model & modelSdf )
{
    return getPluginSdf( modelSdf, "roficomPlugin" );
}

inline sdf::ElementPtr getRoFIModulePluginSdf( sdf::ElementPtr modelSdf )
{
    return getPluginSdf( modelSdf, "rofiModulePlugin" );
}

inline sdf::ElementPtr getRoFIModulePluginSdf( const sdf::Model & modelSdf )
{
    return getPluginSdf( modelSdf, "rofiModulePlugin" );
}

inline sdf::ElementPtr getAttacherPluginSdf( sdf::ElementPtr worldSdf )
{
    return getPluginSdf( worldSdf, "attacherPlugin" );
}

inline sdf::ElementPtr getAttacherPluginSdf( const sdf::World & worldSdf )
{
    return getPluginSdf( worldSdf, "attacherPlugin" );
}

inline sdf::ElementPtr getDistributorPluginSdf( sdf::ElementPtr worldSdf )
{
    return getPluginSdf( worldSdf, "distributorPlugin" );
}

inline sdf::ElementPtr getDistributorPluginSdf( const sdf::World & worldSdf )
{
    return getPluginSdf( worldSdf, "distributorPlugin" );
}

inline bool isRoFIModule( sdf::ElementPtr modelSdf )
{
    return modelSdf && getRoFIModulePluginSdf( modelSdf ) != nullptr;
}

inline bool isRoFIModule( const sdf::Model & modelSdf )
{
    return getRoFIModulePluginSdf( modelSdf ) != nullptr;
}

inline bool isRoFICoM( sdf::ElementPtr modelSdf )
{
    return modelSdf && getRoFICoMPluginSdf( modelSdf ) != nullptr;
}

inline bool isRoFICoM( const sdf::Model & modelSdf )
{
    return getRoFICoMPluginSdf( modelSdf ) != nullptr;
}

inline bool isRoFIModule( gz::sim::Entity model, const gz::sim::EntityComponentManager & ecm )
{
    const auto * modelSdf = ecm.Component< gz::sim::components::ModelSdf >( model );
    return modelSdf && isRoFIModule( modelSdf->Data() );
}

inline bool isRoFICoM( gz::sim::Entity model, const gz::sim::EntityComponentManager & ecm )
{
    const auto * modelSdf = ecm.Component< gz::sim::components::ModelSdf >( model );
    return modelSdf && isRoFICoM( modelSdf->Data() );
}

inline bool hasAttacherPlugin( gz::sim::Entity world, const gz::sim::EntityComponentManager & ecm )
{
    const auto * worldSdf = ecm.Component< gz::sim::components::WorldSdf >( world );
    return worldSdf && getAttacherPluginSdf( worldSdf->Data() ) != nullptr;
}

inline std::optional< gz::sim::Entity > findModelEntityByScopedName(
        const std::string & scopedName,
        const gz::sim::EntityComponentManager & ecm,
        gz::sim::Entity relativeTo = gz::sim::kNullEntity )
{
    for ( auto entity : gz::sim::entitiesFromScopedName( scopedName, ecm, relativeTo, "::" ) )
    {
        if ( ecm.Component< gz::sim::components::Model >( entity ) != nullptr )
        {
            return entity;
        }
    }
    return std::nullopt;
}

} // namespace gazebo
