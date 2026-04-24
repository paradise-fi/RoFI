#include "roficomUtils.hpp"


namespace gazebo
{
namespace
{
bool nameMatches( const std::string & actual, const std::string & desired )
{
    if ( actual == desired )
    {
        return true;
    }
    if ( actual.size() > desired.size() + 2
         && actual.compare( actual.size() - desired.size(), desired.size(), desired ) == 0
         && actual.compare( actual.size() - desired.size() - 2, 2, "::" ) == 0 )
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
