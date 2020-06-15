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

inline std::string getElemPath( gazebo::physics::BasePtr elem,
                                const std::string & delim = "/",
                                bool prependWorldName = true )
{
    assert( elem );

    std::vector< std::string > names;

    while ( elem )
    {
        if ( !prependWorldName && !elem->GetParent() )
        {
            break;
        }
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
        names.push_back( getAttribute< std::string >( elem, "name" ) );
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

inline std::string GetScopedName( sdf::ElementPtr elem, bool prependWorldName = false )
{
    return getElemPath( elem, "::", prependWorldName );
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
    return getPluginSdf( modelSdf, "libroficomPlugin.so" );
}

inline sdf::ElementPtr getRoFIModulePluginSdf( sdf::ElementPtr modelSdf )
{
    return getPluginSdf( modelSdf, "libuniversalModulePlugin.so" );
}

inline sdf::ElementPtr getAttacherPluginSdf( sdf::ElementPtr worldSdf )
{
    return getPluginSdf( worldSdf, "libattacherPlugin.so" );
}

inline sdf::ElementPtr getDistributorPluginSdf( sdf::ElementPtr worldSdf )
{
    return getPluginSdf( worldSdf, "libdistributorPlugin.so" );
}

inline bool isRoFIModule( sdf::ElementPtr modelSdf )
{
    return modelSdf && getRoFIModulePluginSdf( modelSdf ) != nullptr;
}

inline bool isRoFIModule( physics::ModelPtr model )
{
    return model && isRoFIModule( model->GetSDF() );
}

inline bool isRoFICoM( sdf::ElementPtr modelSdf )
{
    return modelSdf && getRoFICoMPluginSdf( modelSdf ) != nullptr;
}

inline bool isRoFICoM( physics::ModelPtr model )
{
    return model && isRoFICoM( model->GetSDF() );
}

inline bool hasAttacherPlugin( physics::WorldPtr world )
{
    return world && getAttacherPluginSdf( world->SDF() ) != nullptr;
}

} // namespace gazebo
