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
public:
    using ConnectorAttachInfoPtr = boost::shared_ptr< const rofi::messages::ConnectorAttachInfo >;
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using StringPair = std::pair< std::string, std::string >;
    using LinkPair = std::pair< physics::LinkPtr, physics::LinkPtr >;

    static constexpr double distancePrecision = 2e-3; // [m]
    static constexpr double connectionForce = 4;      // [N]


    AttacherPlugin() = default;

    AttacherPlugin( const AttacherPlugin & ) = delete;
    AttacherPlugin & operator=( const AttacherPlugin & ) = delete;

    void Load( physics::WorldPtr world, sdf::ElementPtr /*sdf*/ ) override;

    bool attach( StringPair modelNames, Orientation orientation );
    bool detach( StringPair modelNames );

    void sendAttachInfo( std::string modelName1,
                         std::string modelName2,
                         bool attach,
                         Orientation orientation );

    void attach_event_callback( const ConnectorAttachInfoPtr & msg );

    void onPhysicsUpdate();

    static bool applyAttractForce( LinkPair links, Orientation orientation );
    static physics::JointPtr createFixedJoint( physics::PhysicsEnginePtr physics, LinkPair links );

private:
    using ConnectedInfo = std::pair< Orientation, std::variant< LinkPair, physics::JointPtr > >;

    void sendAttachInfoToOne( std::string roficomName,
                              std::string connectedToName,
                              bool attach,
                              Orientation orientation );


    event::ConnectionPtr _onUpdate;

    transport::NodePtr _node;
    transport::SubscriberPtr _subAttachEvent;

    std::map< StringPair, ConnectedInfo > _connected;
    std::mutex _connectedMutex;

    physics::PhysicsEnginePtr _physics;
    physics::WorldPtr _world;
};

} // namespace gazebo
