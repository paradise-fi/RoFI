/*
 * Desc: Gazebo link attacher plugin.
 * Author: Sammy Pfeiffer (sam.pfeiffer@pal-robotics.com)
 * Date: 05/04/2016
 */

#pragma once

#include <iostream>
#include <map>
#include <utility>
#include <variant>

#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>
#include <gazebo/transport/transport.hh>
#include <sdf/sdf.hh>

#include <connectorAttachInfo.pb.h>

namespace gazebo
{
std::pair< std::string, std::string > sortNames( std::pair< std::string, std::string > names );

class AttacherPlugin : public WorldPlugin
{
    static constexpr double distancePrecision = 1e-4; // [m]
    static constexpr double connectionForce = 2;      // [N]

public:
    using ConnectorAttachInfoPtr = boost::shared_ptr< const rofi::messages::ConnectorAttachInfo >;
    using StringPair = std::pair< std::string, std::string >;
    using LinkPair = std::pair< physics::LinkPtr, physics::LinkPtr >;

    void Load( physics::WorldPtr world, sdf::ElementPtr /*sdf*/ ) override;

    bool attach( StringPair modelNames );
    bool detach( StringPair modelNames );

    // TODO deprecated in the future
    void moveToOther( std::string thisRoficomName,
                      std::string otherRoficomName,
                      rofi::messages::ConnectorState::Orientation orientation );

    void sendAttachInfo( std::string modelName1,
                         std::string modelName2,
                         bool attach,
                         rofi::messages::ConnectorState::Orientation orientation );

    void attach_event_callback( const ConnectorAttachInfoPtr & msg );

    void onPhysicsUpdate();

    static physics::JointPtr createFixedJoint( physics::PhysicsEnginePtr physics, LinkPair links );

private:
    void sendAttachInfoToOne( std::string roficomName,
                              std::string connectedToName,
                              bool attach,
                              rofi::messages::ConnectorState::Orientation orientation );


    event::ConnectionPtr _onUpdate;

    transport::NodePtr _node;
    transport::SubscriberPtr _subAttachEvent;

    std::map< StringPair, std::variant< LinkPair, physics::JointPtr > > _connected;
    std::mutex _connectedMutex;

    physics::PhysicsEnginePtr _physics;
    physics::WorldPtr _world;
};

} // namespace gazebo
