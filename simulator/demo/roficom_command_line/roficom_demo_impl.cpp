#include <chrono>
#include <cstddef>
#include <functional>
#include <memory>
#include <thread>

#include <gazebo/transport/transport.hh>

#include "rofi_hal_old.hpp"

#include <rofiCmd.pb.h>

std::string getRoficomName( int i )
{
    if ( i == 0 )
        return "RoFICoM";
    return "RoFICoM_" + std::to_string( i - 1 );
}

void connectImpl( int connectorNumber, bool connect )
{
    using rofi::messages::ConnectorCmd;

    static gazebo::transport::NodePtr node;
    static std::vector< gazebo::transport::PublisherPtr > pubs;

    if ( !node )
    {
        node = boost::make_shared< gazebo::transport::Node >();
        node->Init( "default" );
    }

    bool wait = false;
    for ( int i = pubs.size(); i <= connectorNumber; i++ )
    {
        wait = true;
        pubs.push_back(
                node->Advertise< ConnectorCmd >( "~/" + getRoficomName( i ) + "/control" ) );

        ConnectorCmd msg;
        msg.set_connector( connectorNumber );
        msg.set_cmdtype( ConnectorCmd::NO_CMD );
        pubs.at( i )->Publish( std::move( msg ) );
    }

    if ( wait )
    {
        std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
    }

    ConnectorCmd msg;
    msg.set_connector( connectorNumber );
    msg.set_cmdtype( connect ? ConnectorCmd::CONNECT : ConnectorCmd::DISCONNECT );
    pubs.at( connectorNumber )->Publish( std::move( msg ) );
}

namespace rofi::hal
{
namespace detail
{
class RoFIData
{
public:
    std::vector< std::unique_ptr< ConnectorData > > connectors;
};
class JointData
{
};
class ConnectorData
{
public:
    int connectorNumber = 0;
    Connector getConnector()
    {
        return *this;
    }
};
} // namespace detail

RoFI::RoFI() : rofiData( std::make_unique< detail::RoFIData >() )
{
}

// Destructor is default, but has to be defined in implementation (for unique_ptr)
RoFI::~RoFI()
{
}

RoFI & RoFI::getLocalRoFI()
{
    static std::unique_ptr< RoFI > local;

    if ( !local )
    {
        local = std::make_unique< RoFI >( RoFI() );
    }
    return *local;
}

Joint RoFI::getJoint( int )
{
    throw std::runtime_error( "Joints are not implemented" );
}
Connector RoFI::getConnector( int index )
{
    for ( int i = rofiData->connectors.size(); i <= index; i++ )
    {
        rofiData->connectors.push_back( std::make_unique< detail::ConnectorData >() );
        rofiData->connectors.back()->connectorNumber = i;
    }
    return rofiData->connectors.at( index )->getConnector();
}

Joint::Joint( detail::JointData & )
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::maxPosition() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::minPosition() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::maxSpeed() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::minSpeed() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::maxTorque() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::getVelocity() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
void Joint::setVelocity( float )
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::getPosition() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
void Joint::setPosition( float, float, std::function< void( Joint ) > )
{
    throw std::runtime_error( "Joints are not implemented" );
}
float Joint::getTorque() const
{
    throw std::runtime_error( "Joints are not implemented" );
}
void Joint::setTorque( float )
{
    throw std::runtime_error( "Joints are not implemented" );
}

Connector::Connector( detail::ConnectorData & cdata ) : connectorData( &cdata )
{
}

void Connector::connect()
{
    connectImpl( connectorData->connectorNumber, true );
}
void Connector::disconnect()
{
    connectImpl( connectorData->connectorNumber, false );
}
} // namespace rofi::hal
