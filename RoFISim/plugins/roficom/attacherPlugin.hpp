#pragma once

#include <iostream>
#include <map>
#include <optional>
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
rofi::messages::ConnectorState::Orientation readOrientation( const std::string & str );
ignition::math::Quaterniond rotation( rofi::messages::ConnectorState::Orientation orientation );

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

    void Load( physics::WorldPtr world, sdf::ElementPtr sdf ) override;

    void loadConnectionsFromSdf();
    void addConnectionToSdf( StringPair modelNames, std::optional< Orientation > orientation );
    void removeConnectionFromSdf( StringPair modelNames );

    bool attach( StringPair modelNames, std::optional< Orientation > orientation );
    std::optional< Orientation > detach( StringPair modelNames );

    void sendAttachInfo( std::string modelName1,
                         std::string modelName2,
                         bool attach,
                         Orientation orientation );

    void attach_event_callback( const ConnectorAttachInfoPtr & msg );

    void onPhysicsUpdate();

    static bool applyAttractForce( LinkPair links, Orientation orientation );
    static physics::JointPtr createFixedJoint( physics::PhysicsEnginePtr physics, LinkPair links );

private:
    struct ConnectedInfo
    {
        Orientation orientation;
        std::variant< LinkPair, physics::JointPtr > info;

        ConnectedInfo( Orientation orientation, std::variant< LinkPair, physics::JointPtr > info )
                : orientation( orientation )
                , info( info )
        {
        }
    };

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
    sdf::ElementPtr _sdf;
};

} // namespace gazebo
