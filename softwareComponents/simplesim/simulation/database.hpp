#pragma once

#include <map>
#include <optional>

#include "internal_state.hpp"

#include "rofiDescription.pb.h"


namespace rofi::simplesim
{
class Database
{
public:
    using RofiId = InternalState::RofiId;
    using RofiDescription = rofi::messages::RofiDescription;
    using ConnectorState = InternalConnectorState::ConnectorState;
    using ConnectorLine = rofi::messages::ConnectorCmd::Line;
    using ConnectedTo = std::optional< InternalConnectorState::Connector >;

    bool hasRofi( RofiId rofiId ) const
    {
        return _internalStates.find( rofiId ) != _internalStates.end();
    }

    void addRofi( RofiId rofiId, int joints, int connectors )
    {
        _internalStates.insert_or_assign( rofiId, InternalState( joints, connectors ) );
    }

    // Returns rofi description if module with rofiId exists
    std::optional< RofiDescription > getDescription( RofiId rofiId ) const;


    // Returns velocity if module with rofiId exists and has given joint
    std::optional< float > getVelocity( RofiId rofiId, int joint ) const;

    // Sets position and returns true if module with rofiId exists and has given joint
    bool setPositionWithSpeed( RofiId rofiId, int joint, float position, float speed );
    // Sets velocity and returns true if module with rofiId exists and has given joint
    bool setVelocity( RofiId rofiId, int joint, float velocity );


    // Returns connector state if module with rofiId exists and has given connector
    std::optional< ConnectorState > getConnectorState( RofiId rofiId, int connector ) const;
    // Returns the connectedTo connector if such exists
    ConnectedTo getConnectedTo( RofiId rofiId, int connector ) const;
    // Extends connector and returns true if module with rofiId exists and has given connector
    bool extendConnector( RofiId rofiId, int connector );
    // Retracts connector and returns true if module with rofiId exists and has given connector
    bool retractConnector( RofiId rofiId, int connector );
    // Sets selected power line and returns true if module with rofiId exists and has given
    // connector
    bool setConnectorPower( RofiId rofiId, int connector, ConnectorLine line, bool connect );

private:
    std::map< RofiId, InternalState > _internalStates;
};

} // namespace rofi::simplesim
