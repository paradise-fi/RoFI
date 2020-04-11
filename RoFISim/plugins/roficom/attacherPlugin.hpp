/*
 * Desc: Gazebo link attacher plugin.
 * Author: Sammy Pfeiffer (sam.pfeiffer@pal-robotics.com)
 * Date: 05/04/2016
 */

#pragma once

#include <iostream>
#include <map>

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
public:
    using ConnectorAttachInfoPtr = boost::shared_ptr< const rofi::messages::ConnectorAttachInfo >;

    void Load( physics::WorldPtr world, sdf::ElementPtr /*sdf*/ ) override;

    bool attach( std::pair< std::string, std::string > modelNames );
    bool detach( std::pair< std::string, std::string > modelNames );

    // TODO deprecated in the future
    void moveToOther( std::string thisRoficomName,
                      std::string otherRoficomName,
                      rofi::messages::ConnectorState::Orientation orientation );

    void sendAttachInfo( std::string modelName1,
                         std::string modelName2,
                         bool attach,
                         rofi::messages::ConnectorState::Orientation orientation );

    void attach_event_callback( const ConnectorAttachInfoPtr & msg );

private:
    void sendAttachInfoToOne( std::string roficomName,
                              std::string connectedToName,
                              bool attach,
                              rofi::messages::ConnectorState::Orientation orientation );


    transport::NodePtr _node;
    transport::SubscriberPtr _subAttachEvent;

    std::map< std::pair< std::string, std::string >, physics::JointPtr > _joints;

    physics::PhysicsEnginePtr _physics;
    physics::WorldPtr _world;
};

} // namespace gazebo
