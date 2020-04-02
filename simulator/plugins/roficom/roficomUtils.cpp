#include "roficomUtils.hpp"


namespace gazebo
{

physics::LinkPtr getLinkByName( physics::ModelPtr roficom, const std::string & linkName )
{
    assert( roficom );
    assert( isRoFICoM( roficom ) );

    auto linkPtr = roficom->GetLink( linkName );
    if ( !linkPtr )
    {
        linkPtr = roficom->GetLink( "RoFICoM::" + linkName );
    }
    if ( !linkPtr )
    {
        for ( auto link : roficom->GetLinks() )
        {
            auto name = link->GetName();
            int nameSize = name.size();
            int compareSize = linkName.size() + 2;

            if ( nameSize < compareSize )
            {
                continue;
            }
            if ( name.compare( nameSize - compareSize, compareSize, "::" + name ) == 0 )
            {
                if ( linkPtr )
                {
                    gzwarn << "Found two " << linkName << " links: " << linkPtr->GetName() << ", " << link->GetName() << "\n";
                    break;
                }
                linkPtr = std::move( link );
            }
        }
    }
    return linkPtr;
}

physics::JointPtr getJointByName( physics::ModelPtr roficom, const std::string & jointName )
{
    assert( roficom );
    assert( isRoFICoM( roficom ) );

    auto jointPtr = roficom->GetJoint( jointName );
    if ( !jointPtr )
    {
        jointPtr = roficom->GetJoint( "RoFICoM::" + jointName );
    }
    if ( !jointPtr )
    {
        for ( auto joint : roficom->GetJoints() )
        {
            auto name = joint->GetName();
            int nameSize = name.size();
            int compareSize = jointName.size() + 2;

            if ( nameSize < compareSize )
            {
                continue;
            }
            if ( name.compare( nameSize - compareSize, compareSize, "::" + name ) == 0 )
            {
                if ( jointPtr )
                {
                    gzwarn << "Found two " << jointName << " joints: " << jointPtr->GetName() << ", " << joint->GetName() << "\n";
                    break;
                }
                jointPtr = std::move( joint );
            }
        }
    }
    return jointPtr;
}

} // namespace gazebo
