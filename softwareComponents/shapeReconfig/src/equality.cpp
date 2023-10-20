#include <shapeReconfig/equality.hpp>

namespace rofi::shapereconfig {

using namespace rofi::configuration;
 
// Used for joint parameters, must be within tolerance (1/100 to recognize one degree at distance of 0.5)
bool isClose( float f1, float f2 )
{
    return fabs( f1 - f2 ) < 0.01f;
}

// Used for limits, for equality they should be almost equal in value
bool isClosePair( const std::pair< float, float >& p1, const std::pair< float, float >& p2 )
{
    return fabs( p1.first - p2.first ) < 0.0001f && fabs( p1.second - p2.second ) < 0.0001f;

}

bool equalJoint( const Joint& j1, const Joint& j2 )
{
    return std::ranges::equal( j1.jointLimits(), j2.jointLimits(), isClosePair )
        && std::ranges::equal( j1.positions(), j2.positions(), isClose );
}

bool equalRoficomJoint( const RoficomJoint& rj1, const RoficomJoint& rj2 )
{
    return rj1.orientation == rj2.orientation
        && rj1.sourceModule == rj2.sourceModule
        && rj1.destModule == rj2.destModule
        && rj1.sourceConnector == rj2.sourceConnector
        && rj1.destConnector == rj2.destConnector
        && std::ranges::equal( rj1.jointLimits(), rj2.jointLimits(), isClosePair )
        && std::ranges::equal( rj1.positions(), rj2.positions(), isClose );
}

/* bool equalSpaceJoints( const SpaceJoint& sj1, const SpaceJoint& sj2 )
{
    // ignores reference points
    return equalJoints( *sj1.joint, *sj2.joint )
        && sj1.destModule == sj2.destModule 
        && sj1.destComponent == sj2.destComponent;
} */

bool equalComponentJoint( const ComponentJoint& cj1, const ComponentJoint& cj2 )
{
    return equalJoint( *cj1.joint, *cj2.joint )
        && cj1.sourceComponent == cj2.sourceComponent
        && cj1.destinationComponent == cj2.destinationComponent;
}

bool equalModule( const Module& rModule1, const Module& rModule2 )
{
    // ignores components
    return rModule1.type == rModule2.type
        && rModule1.getId() == rModule2.getId()
        && std::ranges::equal( rModule1.joints(), rModule2.joints(), equalComponentJoint );   
}

bool equalConfiguration( const RofiWorld& rw1, const RofiWorld& rw2 )
{
    // ignores _spaceJoints and referencePoints
    return std::ranges::equal( rw1.modules(), rw2.modules(), equalModule )
        && std::ranges::equal( rw1.roficomConnections(), rw2.roficomConnections(), equalRoficomJoint );
}

} // namespace rofi::shapereconfig
