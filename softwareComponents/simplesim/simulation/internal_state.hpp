#pragma once

#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

#include "connectorState.pb.h"
#include "rofiCmd.pb.h"


namespace rofi::simplesim
{
class InternalJointState
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );

    float velocity = {};
    std::optional< float > desiredPosition;
};

class InternalConnectorState
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    using ConnectorState = rofi::messages::ConnectorState;

    class Connector
    {
    public:
        Connector( RofiId rofiId, int connector, ConnectorState::Orientation orientation )
                : rofiId( rofiId )
                , connector( connector )
                , orientation( orientation )
        {
        }

        RofiId rofiId = {};
        int connector = {};
        ConnectorState::Orientation orientation = {};
    };


    bool position = false;
    bool internal = false;
    bool external = false;
    std::optional< Connector > connectedTo;

    void retract()
    {
        position = false;
        connectedTo.reset();
    }
    void setConnectedTo( RofiId rofiId, int connector, ConnectorState::Orientation orientation )
    {
        connectedTo = { rofiId, connector, orientation };
    }

    ConnectorState getConnectorState() const
    {
        ConnectorState state;
        state.set_position( position );
        state.set_internal( internal );
        state.set_external( external );
        state.set_connected( connectedTo.has_value() );
        if ( connectedTo.has_value() )
        {
            state.set_orientation( connectedTo->orientation );
        }
        return state;
    }
};

struct InternalState
{
public:
    using RofiId = decltype( rofi::messages::RofiCmd().rofiid() );
    static_assert( std::is_same_v< RofiId, InternalJointState::RofiId > );
    static_assert( std::is_same_v< RofiId, InternalConnectorState::RofiId > );

    InternalState( int joints, int connectors ) : _joints( joints ), _connectors( connectors )
    {
    }

    InternalState( const InternalState & ) = default;
    InternalState & operator=( const InternalState & ) = default;

    auto jointsSize() const
    {
        return _joints.size();
    }
    auto connectorsSize() const
    {
        return _connectors.size();
    }

    InternalJointState & getJoint( int joint )
    {
        return _joints.at( joint );
    }
    const InternalJointState & getJoint( int joint ) const
    {
        return _joints.at( joint );
    }

    InternalConnectorState & getConnector( int connector )
    {
        return _connectors.at( connector );
    }
    const InternalConnectorState & getConnector( int connector ) const
    {
        return _connectors.at( connector );
    }

private:
    std::vector< InternalJointState > _joints;
    std::vector< InternalConnectorState > _connectors;
};

} // namespace rofi::simplesim
