#pragma once

#include <gazebo/gazebo.hh>
#include <gazebo/common/Events.hh>
#include <gazebo/physics/physics.hh>

#include <array>
#include <optional>

#include "roficomUtils.hpp"

#include <connectorCmd.pb.h>
#include <connectorResp.pb.h>


namespace gazebo
{

class RoficomConnectionJoint
{
public:
    using Orientation = rofi::messages::ConnectorState::Orientation;

    void load( physics::ModelPtr model );

    void connectToOther( physics::ModelPtr otherRoficom, Orientation orientation );
    void disconnect();

private:
    physics::ModelPtr _model;
    physics::JointPtr _joint;
};


class RoFICoMPlugin;

class RoficomConnection
{
public:
    using Orientation = rofi::messages::ConnectorState::Orientation;
    using PacketPtr = boost::shared_ptr< const rofi::messages::Packet >;
    
    void load( RoFICoMPlugin & roficomPlugin, physics::ModelPtr model, transport::NodePtr node );


    bool isConnected() const;
    std::optional< Orientation > getOrientation() const
    {
        return _orientation;
    }

    void connectToNearby();
    void disconnect();

    void sendPacket( const rofi::messages::Packet & packet );
    void onPacket( const PacketPtr & packet );

private:
    void connect( physics::ModelPtr otherRoficom, RoficomConnection::Orientation orientation );

    std::optional< Orientation > canBeConnected( physics::ModelPtr roficom ) const;

    void startCommunication( physics::ModelPtr otherRoficom );
    void checkConnection( RoFICoMPosition position );

    std::optional< Orientation > _orientation;

    RoFICoMPlugin * _roficomPlugin = nullptr;
    physics::ModelPtr _model;

    transport::NodePtr _node;
    transport::PublisherPtr _pubToOther;
    transport::SubscriberPtr _subToOther;

    physics::ModelPtr _connectedTo;
};

} // namespace gazebo
