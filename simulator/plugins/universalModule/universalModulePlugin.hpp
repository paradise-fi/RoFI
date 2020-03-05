#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <deque>
#include <limits>
#include <utility>
#include <vector>

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

#include "utils.hpp"
#include "pidController.hpp"
#include "pidLoader.hpp"

namespace gazebo
{

class UniversalModulePlugin : public ModelPlugin
{
public:
    static constexpr double doublePrecision = 0.001;

    UniversalModulePlugin() = default;

    UniversalModulePlugin( const UniversalModulePlugin & ) = delete;
    UniversalModulePlugin & operator=( const UniversalModulePlugin & ) = delete;

    ~UniversalModulePlugin()
    {
        clearConnectors();
        if ( _node )
        {
            _node->Fini();
        }
    }

    virtual void Load( physics::ModelPtr model, sdf::ElementPtr sdf );

private:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;
    using ConnectorRespPtr = boost::shared_ptr< const rofi::messages::ConnectorResp >;

    void initCommunication();

    // Connectors have to be models, that have attached plugin "libroficomPlugin.so"
    // Ideally these are nested models of RoFICoM
    void addConnector( gazebo::physics::ModelPtr connectorModel );
    void clearConnectors();
    void findAndInitJoints();
    void findAndInitConnectors();

    rofi::messages::RofiResp getJointRofiResp( rofi::messages::JointCmd::Type cmdtype, int joint, float value ) const;
    rofi::messages::RofiResp getConnectorRofiResp( const rofi::messages::ConnectorResp & connectorResp ) const;

    void onRofiCmd( const RofiCmdPtr & msg );
    void onJointCmd( const rofi::messages::JointCmd & msg );
    void onConnectorCmd( const rofi::messages::ConnectorCmd & msg );

    void onConnectorResp( const ConnectorRespPtr & msg );

    void setVelocity( int joint, double velocity );
    void setTorque( int joint, double torque );
    void setPositionWithSpeed( int joint, double desiredPosition, double speed );


    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::SubscriberPtr _sub;
    transport::PublisherPtr _pub;

    std::deque< JointData< PIDController > > joints;
    std::vector< std::pair< transport::PublisherPtr, transport::SubscriberPtr > > connectors;
};

} // namespace gazebo
