#include "roficomUtils.hpp"


namespace gazebo
{
namespace
{
bool hasDelimitedSuffix( std::string_view actual,
                         std::string_view desired,
                         std::string_view delimiter )
{
    return actual.size() > desired.size() + delimiter.size()
        && actual.compare( actual.size() - desired.size(), desired.size(), desired ) == 0
        && actual.compare( actual.size() - desired.size() - delimiter.size(),
                           delimiter.size(),
                           delimiter ) == 0;
}

bool hasGazeboDuplicateSuffix( std::string_view actual, std::string_view desired )
{
    if ( actual.size() <= desired.size() + 2 || !actual.starts_with( desired ) )
    {
        return false;
    }

    auto suffix = actual.substr( desired.size() );
    if ( suffix.front() != '(' || suffix.back() != ')' )
    {
        return false;
    }

    suffix.remove_prefix( 1 );
    suffix.remove_suffix( 1 );
    return !suffix.empty()
        && std::all_of( suffix.begin(), suffix.end(), []( unsigned char c ) {
               return std::isdigit( c ) != 0;
           } );
}

bool nameMatches( const std::string & actual, const std::string & desired )
{
    auto leafName = std::string_view( actual );
    if ( auto scopePos = leafName.rfind( "::" ); scopePos != std::string_view::npos )
    {
        leafName.remove_prefix( scopePos + 2 );
    }

    if ( leafName == desired || hasDelimitedSuffix( leafName, desired, "__" )
         || hasGazeboDuplicateSuffix( leafName, desired ) )
    {
        return true;
    }
    return false;
}
} // namespace

std::optional< gz::sim::Entity > getLinkByName( gz::sim::Entity roficom,
                                                const gz::sim::EntityComponentManager & ecm,
                                                const std::string & linkName )
{
    assert( roficom != gz::sim::kNullEntity );
    assert( isRoFICoM( roficom, ecm ) );

    gz::sim::Model model( roficom );
    if ( auto entity = model.LinkByName( ecm, linkName ); entity != gz::sim::kNullEntity )
    {
        return entity;
    }

    for ( auto linkEntity : model.Links( ecm ) )
    {
        gz::sim::Link link( linkEntity );
        if ( auto name = link.Name( ecm ); name && nameMatches( *name, linkName ) )
        {
            return linkEntity;
        }
    }
    return std::nullopt;
}

std::optional< gz::sim::Entity > getJointByName( gz::sim::Entity roficom,
                                                 const gz::sim::EntityComponentManager & ecm,
                                                 const std::string & jointName )
{
    assert( roficom != gz::sim::kNullEntity );
    assert( isRoFICoM( roficom, ecm ) );

    gz::sim::Model model( roficom );
    if ( auto entity = model.JointByName( ecm, jointName ); entity != gz::sim::kNullEntity )
    {
        return entity;
    }

    for ( auto jointEntity : model.Joints( ecm ) )
    {
        gz::sim::Joint joint( jointEntity );
        if ( auto name = joint.Name( ecm ); name && nameMatches( *name, jointName ) )
        {
            return jointEntity;
        }
    }
    return std::nullopt;
}

} // namespace gazebo
