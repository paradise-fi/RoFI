#include "database.hpp"


using namespace rofi::simplesim;


std::optional< Database::RofiDescription > Database::getDescription( RofiId rofiId ) const
{
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return {};
    }

    auto description = std::make_optional< RofiDescription >();
    description->set_jointcount( rofi->second.jointsSize() );
    description->set_connectorcount( rofi->second.connectorsSize() );
    return description;
}


std::optional< float > Database::getVelocity( RofiId rofiId, int joint ) const
{
    if ( joint < 0 )
    {
        return {};
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return {};
    }
    if ( size_t( joint ) >= rofi->second.jointsSize() )
    {
        return {};
    }

    return rofi->second.getJoint( joint ).velocity;
}

bool Database::setPositionWithSpeed( RofiId rofiId, int joint, float position, float speed )
{
    if ( joint < 0 )
    {
        return false;
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return false;
    }
    if ( size_t( joint ) >= rofi->second.jointsSize() )
    {
        return false;
    }

    rofi->second.getJoint( joint ).velocity = speed;
    rofi->second.getJoint( joint ).desiredPosition = position;
    return true;
}

bool Database::setVelocity( RofiId rofiId, int joint, float velocity )
{
    if ( joint < 0 )
    {
        return false;
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return false;
    }
    if ( size_t( joint ) >= rofi->second.jointsSize() )
    {
        return false;
    }

    rofi->second.getJoint( joint ).velocity = velocity;
    rofi->second.getJoint( joint ).desiredPosition.reset();
    return true;
}

std::optional< Database::ConnectorState > Database::getConnectorState( RofiId rofiId,
                                                                       int connector ) const
{
    if ( connector < 0 )
    {
        return {};
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return {};
    }
    if ( size_t( connector ) >= rofi->second.connectorsSize() )
    {
        return {};
    }

    return rofi->second.getConnector( connector ).getConnectorState();
}

Database::ConnectedTo Database::getConnectedTo( RofiId rofiId, int connector ) const
{
    if ( connector < 0 )
    {
        return {};
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return {};
    }
    if ( size_t( connector ) >= rofi->second.connectorsSize() )
    {
        return {};
    }

    return rofi->second.getConnector( connector ).connectedTo;
}

bool Database::extendConnector( RofiId rofiId, int connector )
{
    if ( connector < 0 )
    {
        return false;
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return false;
    }
    if ( size_t( connector ) >= rofi->second.connectorsSize() )
    {
        return false;
    }

    rofi->second.getConnector( connector ).position = true;
    return true;
}

bool Database::retractConnector( RofiId rofiId, int connector )
{
    if ( connector < 0 )
    {
        return false;
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return false;
    }
    if ( size_t( connector ) >= rofi->second.connectorsSize() )
    {
        return false;
    }

    rofi->second.getConnector( connector ).retract();
    return true;
}

bool Database::setConnectorPower( RofiId rofiId, int connector, ConnectorLine line, bool connect )
{
    using ConnectorCmd = rofi::messages::ConnectorCmd;

    if ( connector < 0 )
    {
        return false;
    }
    auto rofi = _internalStates.find( rofiId );
    if ( rofi == _internalStates.end() )
    {
        return false;
    }
    if ( size_t( connector ) >= rofi->second.connectorsSize() )
    {
        return false;
    }

    switch ( line )
    {
        case ConnectorCmd::INT_LINE:
        {
            rofi->second.getConnector( connector ).internal = connect;
            break;
        }
        case ConnectorCmd::EXT_LINE:
        {
            rofi->second.getConnector( connector ).external = connect;
            break;
        }
        default:
        {
            return false;
        }
    }
    return true;
}
