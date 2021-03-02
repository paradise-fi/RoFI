#pragma once

#include <deque>
#include <limits>
#include <utility>
#include <vector>

#include <gazebo/common/Events.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "pidController.hpp"
#include "pidLoader.hpp"
#include "utils.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

namespace gazebo
{
class RoFIModulePlugin : public ModelPlugin
{
public:
    static constexpr double doublePrecision = 0.001;

    RoFIModulePlugin() = default;

    RoFIModulePlugin( const RoFIModulePlugin & ) = delete;
    RoFIModulePlugin & operator=( const RoFIModulePlugin & ) = delete;

    ~RoFIModulePlugin()
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
    void startListening();

    // Connectors have to be models, that have attached plugin "libroficomPlugin.so"
    // Ideally these are nested models of RoFICoM
    void addConnector( gazebo::physics::ModelPtr connectorModel );
    void clearConnectors();
    void findAndInitJoints();
    void findAndInitConnectors();

    rofi::messages::RofiResp getJointRofiResp( rofi::messages::JointCmd::Type cmdtype,
                                               int joint,
                                               float value ) const;
    rofi::messages::RofiResp getConnectorRofiResp(
            const rofi::messages::ConnectorResp & connectorResp ) const;

    void onRofiCmd( const RofiCmdPtr & msg );
    void onJointCmd( const rofi::messages::JointCmd & msg );
    void onConnectorCmd( const rofi::messages::ConnectorCmd & msg );

    void onConnectorResp( const ConnectorRespPtr & msg );

    void onUpdate();

    void setVelocity( int joint, double velocity );
    void setTorque( int joint, double torque );
    void setPositionWithSpeed( int joint, double desiredPosition, double speed );


    std::optional< int > rofiId;
    physics::ModelPtr _model;
    sdf::ElementPtr _sdf;

    transport::NodePtr _node;
    transport::SubscriberPtr _sub;
    transport::PublisherPtr _pub;

    event::ConnectionPtr onUpdateConnection;

    std::deque< JointData< PIDController > > joints;
    std::vector< std::pair< transport::PublisherPtr, transport::SubscriberPtr > > connectors;

    std::multimap< common::Time, std::function< void() > > waitCallbacksMap;
    std::mutex waitCallbacksMapMutex;
};

} // namespace gazebo
