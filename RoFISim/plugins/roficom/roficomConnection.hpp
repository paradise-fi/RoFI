#pragma once

#include <array>
#include <optional>

#include <gazebo/common/Events.hh>
#include <gazebo/gazebo.hh>
#include <gazebo/physics/physics.hh>

#include "roficomUtils.hpp"

#include <connectorAttachInfo.pb.h>
#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>


namespace gazebo
{
class RoFICoMPlugin;

class RoficomConnection
{
public:
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using PacketPtr = boost::shared_ptr< const rofi::messages::Packet >;
    using AttachInfoPtr = boost::shared_ptr< const rofi::messages::ConnectorAttachInfo >;

    void load( RoFICoMPlugin & roficomPlugin, physics::ModelPtr model, transport::NodePtr node );


    bool isConnected() const;
    std::optional< Orientation > getOrientation() const
    {
        return _orientation;
    }

    void connectToNearbyRequest();
    void connectRequest( physics::ModelPtr otherRoficom,
                         RoficomConnection::Orientation orientation );
    void disconnectRequest();

    void sendPacket( const rofi::messages::Packet & packet );
    void onPacket( const PacketPtr & packet );

    void onAttachEvent( const AttachInfoPtr & attachInfo );
    void onConnectEvent( const std::string & otherRoficomName, Orientation orientation );
    void onDisconnectEvent( const std::string & otherRoficomName );

private:
    std::optional< Orientation > canBeConnected( physics::ModelPtr roficom ) const;

    void startCommunication( physics::ModelPtr otherRoficom );

    void disconnect();

    std::optional< Orientation > _orientation;

    RoFICoMPlugin * _roficomPlugin = nullptr;
    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubToOther;
    transport::SubscriberPtr _subToOther;

    transport::PublisherPtr _pubAttachEvent;
    transport::SubscriberPtr _subAttachEvent;

    std::string _connectedToName;
    physics::ModelPtr _connectedTo;
};

} // namespace gazebo
