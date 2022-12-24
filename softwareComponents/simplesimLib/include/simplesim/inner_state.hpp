#pragma once

#include <chrono>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "atoms/unreachable.hpp"
#include "atoms/util.hpp"
#include "configuration/rofiworld.hpp"

#include <connectorState.pb.h>
#include <rofiResp.pb.h>


namespace rofi::simplesim
{
using ModuleId = rofi::configuration::ModuleId;

struct Joint {
    ModuleId moduleId;
    int jointIdx;

    bool operator==( const Joint & ) const = default;
    rofi::messages::RofiResp getRofiResp( rofi::messages::JointCmd::Type type,
                                          std::optional< float > value = {} ) const
    {
        rofi::messages::RofiResp rofiResp;
        rofiResp.set_rofiid( this->moduleId );
        rofiResp.set_resptype( rofi::messages::RofiCmd::JOINT_CMD );
        auto & jointResp = *rofiResp.mutable_jointresp();
        jointResp.set_joint( this->jointIdx );
        jointResp.set_resptype( type );
        if ( value ) {
            jointResp.set_value( *value );
        }
        return rofiResp;
    }
};
struct Connector {
    ModuleId moduleId;
    int connIdx;

    bool operator==( const Connector & ) const = default;
    rofi::messages::RofiResp getRofiResp( rofi::messages::ConnectorCmd::Type type ) const
    {
        rofi::messages::RofiResp rofiResp;
        rofiResp.set_rofiid( this->moduleId );
        rofiResp.set_resptype( rofi::messages::RofiCmd::CONNECTOR_CMD );
        auto & connectorResp = *rofiResp.mutable_connectorresp();
        connectorResp.set_connector( this->connIdx );
        connectorResp.set_resptype( type );
        return rofiResp;
    }
};

class JointInnerState {
public:
    struct PositionControl {
    public:
        float position = 0.f;
        float velocity = 0.f;
    };
    struct VelocityControl {
    public:
        float velocity = 0.f;
    };
    using Control = std::variant< VelocityControl, PositionControl >;

    Control control() const
    {
        return _control;
    }

    float velocity() const
    {
        return std::visit( overload{ []( const VelocityControl & velControl ) {
                                        return velControl.velocity;
                                    },
                                     []( const PositionControl & posControl ) {
                                         return posControl.velocity;
                                     } },
                           _control );
    }

    /// Returns if position is reached and new position
    std::pair< bool, float > computeNewPosition( float currentPosition,
                                                 std::chrono::duration< float > duration ) const
    {
        auto getNewPosition = [ &currentPosition, &duration ]( float velocity ) {
            return currentPosition + velocity * duration.count();
        };
        return std::visit( overload{ [ &getNewPosition ]( VelocityControl velControl ) {
                                        return std::pair( false,
                                                          getNewPosition( velControl.velocity ) );
                                    },
                                     [ &getNewPosition ]( PositionControl posControl ) {
                                         auto newPosition = getNewPosition( posControl.velocity );

                                         if ( ( posControl.velocity >= 0 )
                                              == ( posControl.position >= newPosition ) ) {
                                             return std::pair( false, newPosition );
                                         }
                                         return std::pair( true, posControl.position );
                                     } },
                           _control );
    }

    void setVelocityControl( VelocityControl velocityControl )
    {
        _control = { velocityControl };
    }
    void setPositionControl( PositionControl positionControl )
    {
        _control = { positionControl };
    }
    void holdCurrentPosition()
    {
        setVelocityControl( VelocityControl{ .velocity = 0.f } );
    }

private:
    Control _control = { VelocityControl{ .velocity = 0.f } };
};

class ConnectorInnerState {
public:
    using Orientation = rofi::configuration::roficom::Orientation;

    enum class Position
    {
        Retracted,  ///< Connector is in retracted state.
        Retracting, ///< Connector is retracting, but can be still connected in configuration.
        Extending,  ///< Connector is extending and can connect to a near connector.
        Extended,   ///< Connector is in extended state.
    };

    struct OtherConnector {
        Connector connector;
        Orientation orientation;

        bool operator==( const OtherConnector & ) const = default;
    };

public:
    void setRetracting()
    {
        switch ( _position ) {
            case Position::Extending:
            case Position::Extended:
                _position = Position::Retracting;
                return;
            case Position::Retracted:
            case Position::Retracting:
                return;
        }
        assert( false );
    }
    // Sets position to extending if the connector was retracted.
    // To set position from `Extended` to `Extending`, set it first to `Retracting`.
    void setExtending()
    {
        switch ( _position ) {
            case Position::Retracted:
            case Position::Retracting:
                _position = Position::Extending;
                return;
            case Position::Extending:
            case Position::Extended:
                return;
        }
        assert( false );
    }
    Position position() const
    {
        return _position;
    }
    void finalizePosition()
    {
        switch ( _position ) {
            case Position::Retracted:
            case Position::Extended:
                return;
            case Position::Retracting:
                _position = Position::Retracted;
                return;
            case Position::Extending:
                _position = Position::Extended;
                return;
        }
        assert( false );
    }

    void setConnectedTo( Connector connector, Orientation orientation )
    {
        assert( _connectedTo == std::nullopt );
        _connectedTo = { .connector = connector, .orientation = orientation };
    }
    std::optional< OtherConnector > resetConnectedTo()
    {
        auto prevConnectedTo = std::move( _connectedTo );
        _connectedTo.reset();
        return prevConnectedTo;
    }
    const std::optional< OtherConnector > & connectedTo() const
    {
        return _connectedTo;
    }

    bool internal() const
    {
        return _internal;
    }
    bool & internal()
    {
        return _internal;
    }
    bool external() const
    {
        return _external;
    }
    bool & external()
    {
        return _external;
    }

    rofi::messages::ConnectorState connectorState() const
    {
        auto posToBool = []( Position pos ) -> bool {
            switch ( pos ) {
                case Position::Retracted:
                case Position::Retracting:
                    return false;
                case Position::Extending:
                case Position::Extended:
                    return true;
            }
            ROFI_UNREACHABLE( "Unknown connector position" );
        };
        auto toMsgOrientation = []( Orientation o ) {
            switch ( o ) {
                case Orientation::North:
                    return messages::ConnectorState::NORTH;
                case Orientation::East:
                    return messages::ConnectorState::EAST;
                case Orientation::South:
                    return messages::ConnectorState::SOUTH;
                case Orientation::West:
                    return messages::ConnectorState::WEST;
            }
            ROFI_UNREACHABLE( "Unknown connector orientation" );
        };

        messages::ConnectorState state;
        state.set_position( posToBool( position() ) );
        state.set_internal( internal() );
        state.set_external( external() );
        state.set_connected( connectedTo().has_value() );
        if ( connectedTo().has_value() ) {
            state.set_orientation( toMsgOrientation( connectedTo()->orientation ) );
        }
        return state;
    }

private:
    Position _position = Position::Extended;
    bool _internal = false;
    bool _external = false;
    std::optional< OtherConnector > _connectedTo;
};

class ModuleInnerState {
public:
    ModuleInnerState( int jointsCount, int connectorsCount )
            : _joints( jointsCount ), _connectors( connectorsCount )
    {
        assert( jointsCount >= 0 );
        assert( connectorsCount >= 0 );
    }

    ModuleInnerState( const ModuleInnerState & ) = default;
    ModuleInnerState & operator=( const ModuleInnerState & ) = default;
    ModuleInnerState( ModuleInnerState && ) = default;
    ModuleInnerState & operator=( ModuleInnerState && ) = default;

    std::span< const JointInnerState > joints() const
    {
        return _joints;
    }
    std::span< JointInnerState > joints()
    {
        return _joints;
    }

    std::span< const ConnectorInnerState > connectors() const
    {
        return _connectors;
    }
    std::span< ConnectorInnerState > connectors()
    {
        return _connectors;
    }

private:
    std::vector< JointInnerState > _joints;
    std::vector< ConnectorInnerState > _connectors;
};

} // namespace rofi::simplesim
