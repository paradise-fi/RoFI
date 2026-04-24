#pragma once

#include <deque>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <gz/sim/Model.hh>
#include <gz/sim/System.hh>

#include <rofi/gz_transport.hpp>

#include "pidController.hpp"
#include "pidLoader.hpp"
#include "utils.hpp"

#include <rofiCmd.pb.h>
#include <rofiResp.pb.h>

namespace gazebo
{
class RoFIModulePlugin : public gz::sim::System,
                         public gz::sim::ISystemConfigure,
                         public gz::sim::ISystemPreUpdate
{
public:
    static constexpr double doublePrecision = 0.001;

    void Configure( const gz::sim::Entity & entity,
                    const std::shared_ptr< const sdf::Element > & sdf,
                    gz::sim::EntityComponentManager & ecm,
                    gz::sim::EventManager & eventMgr ) override;

    void PreUpdate( const gz::sim::UpdateInfo & info,
                    gz::sim::EntityComponentManager & ecm ) override;

private:
    using RofiCmdPtr = boost::shared_ptr< const rofi::messages::RofiCmd >;
    using ConnectorRespPtr = boost::shared_ptr< const rofi::messages::ConnectorResp >;

    void initCommunication( const gz::sim::EntityComponentManager & ecm );
    void startListening();

    void addConnector( gz::sim::Entity connectorModel, const gz::sim::EntityComponentManager & ecm );
    void clearConnectors();
    void findAndInitJoints( gz::sim::EntityComponentManager & ecm );
    void findAndInitConnectors( const gz::sim::EntityComponentManager & ecm );

    rofi::messages::RofiResp getJointRofiResp( rofi::messages::JointCmd::Type cmdtype,
                                               int joint,
                                               float value ) const;
    rofi::messages::RofiResp getConnectorRofiResp(
            const rofi::messages::ConnectorResp & connectorResp ) const;

    void onRofiCmd( const RofiCmdPtr & msg );
    void onConnectorResp( const ConnectorRespPtr & msg );

    void handleRofiCmd( const rofi::messages::RofiCmd & msg,
                        const gz::sim::UpdateInfo & info,
                        gz::sim::EntityComponentManager & ecm );
    void onJointCmd( const rofi::messages::JointCmd & msg, gz::sim::EntityComponentManager & ecm );
    void onConnectorCmd( const rofi::messages::ConnectorCmd & msg );

    void processWaitCallbacks( const gz::sim::UpdateInfo & info );

    void setVelocity( int joint, double velocity );
    void setTorque( int joint, double torque );
    void setPositionWithSpeed( int joint,
                               double desiredPosition,
                               double speed,
                               const gz::sim::EntityComponentManager & ecm );

    std::optional< int > rofiId;
    gz::sim::Entity _entity = gz::sim::kNullEntity;
    gz::sim::Model _model;
    std::shared_ptr< const sdf::Element > _sdf;

    rofi::gz::NodePtr _node;
    rofi::gz::SubscriberPtr _sub;
    rofi::gz::PublisherPtr _pub;

    std::deque< JointData< PIDController > > joints;
    std::vector< std::pair< rofi::gz::PublisherPtr, rofi::gz::SubscriberPtr > > connectors;

    std::multimap< double, std::function< void() > > waitCallbacksMap;
    std::mutex waitCallbacksMapMutex;

    std::mutex _queueMutex;
    std::vector< rofi::messages::RofiCmd > _pendingCommands;
    std::vector< rofi::messages::ConnectorResp > _pendingConnectorResponses;
};

} // namespace gazebo
