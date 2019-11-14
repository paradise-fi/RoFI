#include "connectorPlugin.hpp"


namespace gazebo
{
std::string getElemPath( gazebo::physics::BasePtr elem )
{
    std::vector< std::string > names;

    while ( elem )
    {
        names.push_back( elem->GetName() );
        elem = elem->GetParent();
    }

    std::string elemPath;
    for ( auto it = names.rbegin(); it != names.rend(); it++ )
    {
        elemPath += "/" + *it;
    }

    return elemPath;
}

// Gets path delimeted with '::' and returns path delimeted by '/'
std::string replaceDelimeter( std::string_view sensorPath )
{
    std::vector< std::string_view > splitPath;
    int last = 0;
    for ( int i = 0; i < sensorPath.size() - 1; i++ )
    {
        if ( sensorPath[ i ] == ':' && sensorPath[ i + 1 ] == ':' )
        {
            splitPath.push_back( sensorPath.substr( last, i - last ) );
            last = i + 2;
        }
    }
    splitPath.push_back( sensorPath.substr( last ) );

    std::string topicName;
    for ( auto name : splitPath )
    {
        topicName += "/";
        topicName += name;
    }

    return topicName;
}


void ConnectorPlugin::Load( physics::ModelPtr model, sdf::ElementPtr /*sdf*/ )
{
    gzmsg << "The Connector plugin is attached to model ["
            << model->GetName() << "]\n";
    _model = model;

    initCommunication();
    initSensorCommunication();

    gzmsg << "Connector plugin ready\n";
}

void ConnectorPlugin::connect()
{
    gzmsg << "Connecting connector " << connectorNumber << "\n";
    // TODO
}

void ConnectorPlugin::disconnect()
{
    gzmsg << "Disconnecting connector " << connectorNumber << "\n";
    // TODO
}

void ConnectorPlugin::sendPacket( const rofi::messages::Packet & packet )
{
    gzmsg << "Sending packet from RoFI in connector " << connectorNumber << "\n";

    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = packet;
    _pubOutside->Publish( std::move( msg ) );
}

void ConnectorPlugin::onPacket( const ConnectorPlugin::PacketPtr & packet )
{
    gzmsg << "Received packet in connector " << connectorNumber << "\nSending to RoFI\n";

    auto msg = getConnectorResp( rofi::messages::ConnectorCmd::PACKET );
    *msg.mutable_packet() = *packet;
    _pubRofi->Publish( std::move( msg ) );
}

void ConnectorPlugin::initCommunication()
{
    if ( !_model )
    {
        gzerr << "Model has to be set before initializing sensor communication\n";
        return;
    }

    if ( _node )
    {
        _node->Fini();
    }

    _node = boost::make_shared< transport::Node >();
    _node->Init( getElemPath( _model ) );

    _pubRofi = _node->Advertise< rofi::messages::ConnectorResp >( "~/response" );
    _subRofi = _node->Subscribe( "~/control", & ConnectorPlugin::onConnectorCmd, this );
}

void ConnectorPlugin::initSensorCommunication()
{
    if ( !_node->IsInitialized() )
    {
        gzerr << "Initialize communication before initializing sensor communication\n";
        return;
    }

    auto sensors = _model->SensorScopedName( "roficonnector-sensor" );

    if ( sensors.size() != 1 )
    {
        gzerr << "Connector plugin expects exactly 1 nested sensor named 'roficonnector-sensor' (found " << sensors.size() << ")\n";
        return;
    }

    auto topicName = "/gazebo" + replaceDelimeter( sensors.front() ) + "/contacts";
    _subSensor = _node->Subscribe( std::move( topicName ), & ConnectorPlugin::onSensorMessage, this );
}

void ConnectorPlugin::onConnectorCmd( const ConnectorCmdPtr & msg )
{
    using rofi::messages::ConnectorCmd;

    connectorNumber = msg->connector();

    switch ( msg->cmdtype() )
    {
    case ConnectorCmd::NO_CMD:
    {
        _pubRofi->Publish( getConnectorResp( ConnectorCmd::NO_CMD ) );
        gzmsg << "Sending response to NO_CMD message\n";
        break;
    }
    case ConnectorCmd::GET_STATE:
    {
        // TODO
        gzmsg << "Getting state is not implemented\n";
        break;
    }
    case ConnectorCmd::CONNECT:
    {
        connect();
        break;
    }
    case ConnectorCmd::DISCONNECT:
    {
        disconnect();
        break;
    }
    case ConnectorCmd::PACKET:
    {
        sendPacket( msg->packet() );
        break;
    }
    case ConnectorCmd::CONNECT_POWER:
    {
        gzmsg << "Connecting power line is not implemented\n";
        break;
    }
    case ConnectorCmd::DISCONNECT_POWER:
    {
        gzmsg << "Disconnecting power line is not implemented\n";
        break;
    }
    case ConnectorCmd::POWER_STATE:
    {
        gzmsg << "Getting power state is not implemented\n";
        _pubRofi->Publish( getConnectorResp( ConnectorCmd::POWER_STATE ) );
        break;
    }
    default:
        gzwarn << "Unknown command type: " << msg->cmdtype() << " of connector " << connectorNumber << "\n";
        break;
    }
}

void ConnectorPlugin::onSensorMessage( const ContactsMsgPtr & contacts )
{
    // TODO

    for ( int i = 0; i < contacts->contact_size(); i++ )
    {
        auto & contact = contacts->contact( i );

        std::cout << "Collision between[" << contact.collision1()
                << "] and [" << contact.collision2() << "]\n";

        for ( int j = 0; j < contact.position_size(); j++ )
        {
            std::cout << j << "  Position:"
                    << contact.position( j ).x() << " "
                    << contact.position( j ).y() << " "
                    << contact.position( j ).z() << "\n";
            std::cout << "   Normal:"
                    << contact.normal( j ).x() << " "
                    << contact.normal( j ).y() << " "
                    << contact.normal( j ).z() << "\n";
            std::cout << "   Depth:" << contact.depth( j ) << "\n";
        }
    }
}

rofi::messages::ConnectorResp ConnectorPlugin::getConnectorResp( rofi::messages::ConnectorCmd::Type cmdtype ) const
{
    rofi::messages::ConnectorResp resp;
    resp.set_connector( connectorNumber );
    resp.set_cmdtype( cmdtype );
    return resp;
}


GZ_REGISTER_MODEL_PLUGIN( ConnectorPlugin )

} // namespace gazebo
