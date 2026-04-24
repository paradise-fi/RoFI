#pragma once

#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <gz/math/Quaternion.hh>
#include <gz/sim/System.hh>

#include <rofi/gz_transport.hpp>

#include "roficomConnect.hpp"
#include "roficomUtils.hpp"

#include <connectorAttachInfo.pb.h>

namespace gazebo
{
std::pair< std::string, std::string > sortNames( std::pair< std::string, std::string > names );
rofi::messages::ConnectorState::Orientation readOrientation( const std::string & str );
gz::math::Quaterniond rotation( rofi::messages::ConnectorState::Orientation orientation );

class AttacherPlugin : public gz::sim::System,
                       public gz::sim::ISystemConfigure,
                       public gz::sim::ISystemPreUpdate
{
public:
    using ConnectorAttachInfoPtr = boost::shared_ptr< const rofi::messages::ConnectorAttachInfo >;
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using StringPair = std::pair< std::string, std::string >;

    void Configure( const gz::sim::Entity & entity,
                    const std::shared_ptr< const sdf::Element > & sdf,
                    gz::sim::EntityComponentManager & ecm,
                    gz::sim::EventManager & eventMgr ) override;

    void PreUpdate( const gz::sim::UpdateInfo & info,
                    gz::sim::EntityComponentManager & ecm ) override;

    void loadConnectionsFromSdf();
    bool attach( StringPair modelNames,
                 std::optional< Orientation > orientation,
                 gz::sim::EntityComponentManager & ecm );
    std::optional< Orientation > detach( StringPair modelNames, gz::sim::EntityComponentManager & ecm );

    void sendAttachInfo( const std::string & modelName1,
                         const std::string & modelName2,
                         bool attach,
                         Orientation orientation );
    void attach_event_callback( const ConnectorAttachInfoPtr & msg );

    static gz::sim::Entity createFixedJoint( gz::sim::Entity parentLink,
                                             gz::sim::Entity childLink,
                                             const std::string & name,
                                             gz::sim::EntityComponentManager & ecm );

private:
    struct ConnectedInfo
    {
        Orientation orientation;
        gz::sim::Entity jointEntity = gz::sim::kNullEntity;
    };

    void sendAttachInfoToOne( const std::string & roficomName,
                              const std::string & connectedToName,
                              bool attach,
                              Orientation orientation );

    gz::sim::Entity _worldEntity = gz::sim::kNullEntity;
    std::string _worldName;
    std::shared_ptr< const sdf::Element > _sdf;

    rofi::gz::NodePtr _node;
    rofi::gz::SubscriberPtr _subAttachEvent;

    std::map< StringPair, ConnectedInfo > _connected;
    std::mutex _connectedMutex;

    std::mutex _queueMutex;
    std::vector< rofi::messages::ConnectorAttachInfo > _pendingAttachRequests;
};

} // namespace gazebo
